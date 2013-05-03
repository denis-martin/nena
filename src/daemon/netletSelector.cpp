/**
 * netletSelector.cpp
 *
 * @brief Netlet selection implementation.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Feb 12, 2009
 *      Author: denis
 */

#include "netletSelector.h"
#include "nena.h"
#include "netAdaptBroker.h"

#include "properties.h"

#include "debug.h"

#include "xmlNode/xmlNode.h"

#include <cstdlib>
#include <string>
#include <sstream>
#include <vector>

#include <boost/lexical_cast.hpp>

#define NETLETSELECTOR_SYSTEMPOLICIESFILE	"nad_systemPolicies.xml"		///< File containing the system policies for Netlet selection
using namespace std;

using boost::shared_ptr;
using boost::shared_mutex;
using boost::shared_lock;
using boost::unique_lock;
using boost::lexical_cast;

const string selectorId = "internalservice://nena/netletSelector";

/**
 * @brief Constructor
 */
CNetletSelector::CNetletSelector(CNena* nodeA, IMessageScheduler *sched) :
		IMessageProcessor(sched), nena(nodeA)
{
	className += "::CNetletSelector";
	setId(selectorId);

	systemPolicies = NULL;

	refreshSystemPolicies();

	registerEvent(EVENT_NETLETSELECTOR_NETLETCHANGED);
	registerEvent(EVENT_NETLETSELECTOR_NOSUITABLENETLET);

	registerEvent(EVENT_NETLETSELECTOR_APPCONNECT);
	registerEvent(EVENT_NETLETSELECTOR_APPDISCONNECT);

	nena->registerInternalService(getId(), this);

	management = nena->lookupInternalService("internalservice://management");
	if (management == NULL) {
		DBG_WARNING(FMT("%1%: node management service not found, namespace nena:// unhandled") % getId());

	}
}

/**
 * @brief Destructor
 */
CNetletSelector::~CNetletSelector()
{
	if (systemPolicies != NULL)
		delete systemPolicies;

	nena->unregisterInternalService(getId(), this);
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CNetletSelector::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CNetletSelector::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage();
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void CNetletSelector::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	if (msg->getType() != IMessage::t_outgoing)
		throw EUnhandledMessage("Message type is not IMessage::t_outgoing");

	shared_ptr<CFlowState> flowState = msg->getFlowState();

	if (flowState == NULL)
		throw EUnhandledMessage("Outgoing message w/o flow state");

//	if (flowState->getOperationalState() == CFlowState::s_stale)
//		throw EUnhandledMessage("Message with stale flow state");

	// TODO associate netlet to flow state
	IAppConnector* appc = (IAppConnector*) msg->getFrom();

	msg->setFrom(this);

	// get destination ID
	boost::shared_ptr<CMorphableValue> mv_destId;
	if (!msg->hasProperty(IMessage::p_destId, mv_destId))
		throw EUnhandledMessage("Cannot determine destination ID");
	string destId = mv_destId->cast<CStringValue>()->value();

	// set source ID
	// TODO what if the architecture's interpretation of an ID is not a node ID?
	boost::shared_ptr<CMorphableValue> mv_srcId(new CStringValue(nena->getNodeName()));
	msg->setProperty(IMessage::p_srcId, mv_srcId);

	if (destId.compare(0, 5, "nena:") == 0) {
		// management request
		if (management) {
			msg->setTo(management);

		} else {
			throw EUnhandledMessage((FMT("%1%: namespace 'nena:' unhandled") % getId()).str());

		}

	} else {
		// determine Netlet (if not already done)
		if (appc->getNetlet() == NULL)
			selectNetlet(appc);

		if (appc->getNetlet() == NULL)
			throw EUnhandledMessage((FMT("No Netlet associated to %1%") % appc->getId()).str());

		msg->setTo(appc->getNetlet());

	}

//	DBG_DEBUG(FMT("Message from app %1% to %2%")
//		% appc->getIdentifier() % msg->getTo()->getId());
	sendMessage(msg);
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void CNetletSelector::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<CFlowState> flowState = msg->getFlowState();
	if (flowState == NULL)
		throw EUnhandledMessage("Incoming message w/o flow state");

	if (flowState->getOperationalState() == CFlowState::s_stale)
		throw EUnhandledMessage("Dropping incoming message from stale flow state");

	boost::shared_lock<boost::shared_mutex> appConnLock(appConnMutex);
	map<CFlowState::FlowId, IAppConnector*>::const_iterator it = appConns.find(flowState->getFlowId());
	if (it == appConns.end()) {
		if (flowState->getOperationalState() == CFlowState::s_valid) {
			// check if we have a server socket for the requested service
			CFlowState::FlowId serverFlow = lookupAppService(flowState->getServiceId());
			if (serverFlow == 0)
				throw EUnhandledMessage((FMT("%1%: Incoming message for unknown service %2%") % getId() % flowState->getServiceId()).str());

			IAppConnector* appConn = lookupFlowId(serverFlow);
			if (appConn == NULL)
				throw EUnhandledMessage((FMT("%1%: Incoming message for non-existent app connector (service %2%, flowId %3%)") %
						getId() % flowState->getServiceId() % flowState->getFlowId()).str());

			assert(appConn->getMethod() == IAppConnector::method_bind);

			// yes, so create an event
			string id = (FMT("%1%/incoming/%2%") % appConn->getRemoteURI() % flowState->getFlowId()).str();

			map<string, list<shared_ptr<IMessage> > >::iterator pr_it;
			pr_it = pendingRequests.find(id);
			if (pr_it == pendingRequests.end()) {
				// new request
				parentAppConns[id] = appConn;
				pendingRequests[id].push_back(msg);
				DBG_DEBUG(FMT("%1%: Creating new incoming event for service %2%(%3%)") %
						getId() % flowState->getServiceId() % flowState->getFlowId());
				appConn->sendEvent(IAppConnector::event_incoming, id);

			} else {
				DBG_DEBUG(FMT("%1%: Adding pending request for service %2%(%3%)") % getId() %
						flowState->getServiceId() % flowState->getFlowId());
				pendingRequests[id].push_back(msg);

			}

		} else {
			throw EUnhandledMessage((FMT("Dropping incoming message for non-valid flow %1% (state %2%)") %
					flowState->getFlowId() % flowState->getOperationalState()).str());

		}

	} else {
		IAppConnector* appConn = it->second;

		// check if the app connector is associated to the sending netlet
		if (appConn->getNetlet() == (INetlet*) msg->getFrom()) {
			msg->setFrom(this);
			msg->setTo(appConn);
			sendMessage(msg);

		} else if ((appConn->getNetlet() == NULL) && (msg->getFrom() == management)) {
			// make an exception for management requests
			msg->setFrom(this);
			msg->setTo(appConn);
			sendMessage(msg);

		} else {
			throw EUnhandledMessage("Flow ID is valid, but not associated to the sending Netlet!");

		}

	}
}

/**
 * @brief	Register an app connector. Usually called by wrapper
 * 			implementations. A new flow state will be assigned.
 *
 * @param appConn	Application connector to register.
 */
void CNetletSelector::registerAppConnector(IAppConnector* appConn)
{
	assert(appConn != NULL);
	bool sendTrigger = false;

	appConn->setNext(this);
	appConn->setRegistered(true);

	boost::unique_lock<boost::shared_mutex> appConnLock(appConnMutex);

	switch (appConn->getMethod()) {
	case IAppConnector::method_bind: {
		registerAppService(appConn->getRemoteURI(), appConn);
//		sendTrigger = true;
		break;
	}
	case IAppConnector::method_connect: {
		sendTrigger = true;
		break;
	}
	case IAppConnector::method_get: {
		map<string, list<boost::shared_ptr<IMessage> > >::iterator it;
		it = pendingRequests.find(appConn->getRemoteURI());
		if (it != pendingRequests.end()) { // pending packets for this incoming request
			shared_ptr<CFlowState> flowState = it->second.front()->getFlowState();
			IAppConnector* parentAppConn = parentAppConns[appConn->getRemoteURI()];
			assert(parentAppConn != NULL);
			assert(parentAppConn->getFlowState()->getChildFlowState(flowState->getRemoteId(), flowState->getRemoteFlowId()));
			appConn->setParent(parentAppConn);
			appConn->setFlowState(flowState);
			appConn->setNetlet(static_cast<INetlet *>(it->second.front()->getFrom())); // TODO: associate Netlet to flowState?

			DBG_DEBUG(FMT("%1%: new child app connection for [%2%(%3%)], remote flow [%4%(%5%)]") %
					getId() % flowState->getServiceId() % flowState->getFlowId() %
					flowState->getRemoteId() % flowState->getRemoteFlowId());

			list<boost::shared_ptr<IMessage> >::iterator mit;
			for (mit = it->second.begin(); mit != it->second.end(); mit++) {
				shared_ptr<IMessage> msg = *mit;
				msg->setFrom(this);
				msg->setTo(appConn);
				sendMessage(msg);

//				DBG_DEBUG(FMT("%1%: sending pending message to new app connector(%2%)") % getId() % flowState->getFlowId());
			}

			pendingRequests.erase(it);

		} else {
			sendTrigger = true;
		}
		break;
	}
	case IAppConnector::method_put: {
		sendTrigger = true;
		break;
	}
	default: {
		DBG_DEBUG(FMT("%1%: unknown method %2%") % getId() % appConn->getMethod());
		break;
	}
	}

	shared_ptr<CFlowState> flowState = appConn->getFlowState();
	if (flowState.get() == NULL) {
		flowState = nena->createFlowState(appConn);
		flowState->setLocalId(nena->getNodeName());
		flowState->setServiceId(appConn->getRemoteURI());
		flowState->setMethod(appConn->getMethod());
		appConn->setFlowState(flowState);

		DBG_DEBUG(FMT("%1%: new app connection [%2%(%3%)] from %4%") %
				getId() % flowState->getServiceId() % flowState->getFlowId() % appConn->getIdentifier());

	}

	if (sendTrigger)
		selectNetlet(appConn);

	shared_ptr<Event_AppConnReady> appConnReadyEvent(new Event_AppConnReady(flowState, this, appConn));
	sendMessage(appConnReadyEvent);

	appConns[flowState->getFlowId()] = appConn;

	if (sendTrigger) {
		if (flowState->getRemoteId().empty()) {
			flowState->setRemoteId(appConn->getRemoteURI());
		}

		string destId = flowState->getRemoteId();

		IMessageProcessor* mp = appConn->getNetlet();
		if (mp == NULL)
			if (destId.compare(0, 5, "nena:") == 0)
				mp = management;

		if (mp) {
			shared_ptr<IMessage> trigger(new Event_AppConnect(flowState, appConn->getMethod(), appConn->getRemoteURI()));
			trigger->setFrom(this);
			trigger->setTo(mp);
			sendMessage(trigger);

		} else {
			DBG_ERROR(FMT("%1%: cannot send trigger: no Netlet associated yet") % getId());

		}

	}

	notifyListeners(Event_AppConnect(flowState, appConn->getMethod(), appConn->getRemoteURI()));
}

/**
 * @brief	Unregister an application connector ("socket").
 *
 * 			The object appConn is *not* destroyed when calling this method.
 *
 * @param appConn	Application connector that is to be unregistered.
 */
void CNetletSelector::unregisterAppConnector(IAppConnector* appConn)
{
	assert(appConn != NULL);

	notifyListeners(Event_AppDisconnect(appConn->getFlowState()));

	if (appConn->getMethod() == IAppConnector::method_bind)
		unregisterAppService(appConn->getRemoteURI(), appConn);

	boost::unique_lock<boost::shared_mutex> appConnLock(appConnMutex);

	map<CFlowState::FlowId, IAppConnector*>::iterator ac_it;
	ac_it = appConns.find(appConn->getFlowState()->getFlowId());
	if (ac_it != appConns.end()) {
		DBG_DEBUG(FMT("%1%: unregistering app connector for local flow ID %2%") %
				getId() % appConn->getFlowState()->getFlowId());
		appConns.erase(ac_it);

	} else {
		DBG_ERROR("Unregistering unknown AppConnector!");

	}

	// check whether this is a parent connection
	map<std::string, IAppConnector *>::iterator pac_it;
	for (pac_it = parentAppConns.begin(); pac_it != parentAppConns.end(); pac_it++) {
		if (pac_it->second == appConn) {
			parentAppConns.erase(pac_it);
		}
	}

	// check whether this is a child connection
	pac_it = parentAppConns.find(appConn->getRemoteURI());
	if (pac_it != parentAppConns.end()) {
		pac_it->second->getFlowState()->removeChildFlowState(appConn->getFlowState());
		parentAppConns.erase(pac_it);
	}
}

/**
 * @brief 	Register an application-layer service. serviceId is an arbitrary string
 * 			for now and can be seen as some sort of "port number".
 *
 * @param	serviceId	String identifying the service
 * @param	appConn		Application connector ("socket")
 */
void CNetletSelector::registerAppService(const std::string& serviceId, IAppConnector* appConn)
{
	map<std::string, IAppConnector*>::const_iterator it;
	it = services.find(serviceId);
	if (it != services.end())
		DBG_WARNING(FMT("%1%: replacing app service %2%") % getId() % serviceId);
	else
		DBG_DEBUG(FMT("%1%: registering app service %2%") % getId() % serviceId);
	services[serviceId] = appConn;
}

/**
 * @brief 	Unregister an application-layer service. serviceId is an arbitrary string
 * 			for now and can be seen as some sort of "port number".
 *
 * @param	serviceId	String identifying the service
 * @param	appConn		Application connector ("socket")
 */
void CNetletSelector::unregisterAppService(const std::string& serviceId, IAppConnector* appConn)
{
	map<std::string, IAppConnector*>::const_iterator it;
	it = services.find(serviceId);
	if (it != services.end()) {
		DBG_DEBUG(FMT("%1%: unregistering app service %2%") % getId() % serviceId);

		std::string netletId;
		std::string remoteNodeName;
		if (appConn != NULL) {
			if (appConn->getNetlet() != NULL)
				netletId = appConn->getNetlet()->getMetaData()->getId();
			remoteNodeName = appConn->getRemoteURI();
		}

		services.erase(serviceId);

	} else {
		DBG_WARNING(FMT("%1%: attempted to unregister unknown app service %2%") % getId() % serviceId);

	}
}

/**
 * @brief	Lookup a published service ID and return a local flow ID which
 * 			identifies the application connector (thus, the application providing
 * 			the service)
 *
 * @param	serviceId	String identifying the service
 *
 * @returns	Local flow ID of an application providing the service
 */
CFlowState::FlowId CNetletSelector::lookupAppService(std::string serviceId) const
{
	map<std::string, IAppConnector*>::const_iterator it;
	it = services.find(serviceId);
	if (it != services.end()) {
		return it->second->getFlowState()->getFlowId();

	} else {
		// try registered app ids
		map<CFlowState::FlowId, IAppConnector*>::const_iterator ac_it;
		for (ac_it = appConns.begin(); ac_it != appConns.end(); ac_it++) {
			if (ac_it->second->getIdentifier() == serviceId)
				return ac_it->first;

		}

	}

	return 0;
}

/**
 * @brief	Returns the app connector for the given flow ID.
 */
IAppConnector* CNetletSelector::lookupFlowId(CFlowState::FlowId flowId) const
{
	map<CFlowState::FlowId, IAppConnector*>::const_iterator it;
	it = appConns.find(flowId);
	if (it != appConns.end())
		return it->second;
	else
		return NULL;
}

/**
 * @brief	Fills the provided list with known service IDs. The list will be
 * 			cleared if it is not empty.
 *
 * @param	services	Reference to the list that is to be filled
 */
void CNetletSelector::getRegisteredAppServices(std::list<std::string>& services) const
{
	services.clear();
	map<std::string, IAppConnector*>::const_iterator it;
	for (it = this->services.begin(); it != this->services.end(); it++) {
		services.push_back(it->first);
	}
}

/**
 * @brief 	Register a name/addr mapper that will be consulted in case an
 * 			app connector has no Netlet associated with it yet.
 */
void CNetletSelector::registerNameAddrMapper(INameAddrMapper* mapper)
{
	assert(mapper != NULL);

	bool alreadyRegistered = false;
	list<INameAddrMapper*>::const_iterator it;
	for (it = nameAddrMappers.begin(); it != nameAddrMappers.end(); it++) {
		if (*it == mapper) {
			DBG_ERROR("Name/addr mapper already registered!");
			alreadyRegistered = true;
			break;
		}
	}

	if (!alreadyRegistered)
		nameAddrMappers.push_back(mapper);
}

/**
 * @brief 	Unregister a name/addr mapper.
 */
void CNetletSelector::unregisterNameAddrMapper(INameAddrMapper* mapper)
{
	assert(mapper != NULL);

	bool erased = false;
	list<INameAddrMapper*>::iterator it;
	for (it = nameAddrMappers.begin(); it != nameAddrMappers.end(); it++) {
		if (*it == mapper) {
			nameAddrMappers.erase(it);
			erased = true;
			break;
		}
	}

	if (!erased)
		DBG_ERROR("Name/addr mapper not registered!");
}

/**
 * @brief	Assigns the given Netlet to the given app connector. If the Netlet
 * 			was not assigned before, an Event_NetletChanged is emitted.
 */
void CNetletSelector::selectNetlet(IAppConnector* appc, INetlet* netlet)
{
	assert(appc != NULL && netlet != NULL);
	if (appc->getNetlet() != netlet) {
		appc->setNetlet(netlet);
		notifyListeners(Event_NetletChanged(appc));

		DBG_INFO(FMT("%1%: selecting %2% for [%3%(%4%)]") % getId() %
				netlet->getMetaData()->getId() % appc->getRemoteURI() % appc->getFlowState()->getFlowId());

	}

}

/**
 * @brief	Implements automatic Netlet selection.
 *
 * @param appConn	Application connector for which the Netlet will be selected
 */
void CNetletSelector::selectNetlet(IAppConnector* appc)
{
	string selectedNetletStr, defaultNetlet;

	if (nena->getConfig()->hasParameter(selectorId, "defaultNetlet"))
		nena->getConfig()->getParameter(selectorId, "defaultNetlet", defaultNetlet);

	list<INetletMetaData*> netlets;
	list<INetletMetaData*>::const_iterator nit;
	list<INameAddrMapper*>::const_iterator namit;

	// gather candidates
	for (namit = nameAddrMappers.begin(); namit != nameAddrMappers.end(); namit++)
		(*namit)->getPotentialNetlets(appc, netlets);

	if (netlets.size() == 0) {
		DBG_DEBUG(
				FMT("No suitable Netlet found (application '%1%', target '%2%')") % appc->getIdentifier() % appc->getRemoteURI());
		notifyListeners(Event_NoSuitableNetlet(appc));
		return;

	}

	if (!defaultNetlet.empty()) {
		// check if default Netlet is a candidate
		for (nit = netlets.begin(); nit != netlets.end(); nit++) {
			if ((*nit)->getId() == defaultNetlet) {
				selectedNetletStr = (*nit)->getId();
				break;

			}

		}

	}

	if (selectedNetletStr.empty()) {
		// evaluate selectionPriority property
		int last_prio = -1;
		INetletMetaData* sel_md = NULL;
		for (nit = netlets.begin(); nit != netlets.end(); nit++) {
			string sp = (*nit)->getProperty("selectionPriority");
			if (!sp.empty()) {
				int prio = lexical_cast<int>(sp);
				if (prio < last_prio || last_prio == -1) {
					last_prio = prio;
					sel_md = *nit;
				}
			}
		}

		if (sel_md)
			selectedNetletStr = sel_md->getId();
	}

	if (selectedNetletStr.empty()) {
		// take first candidate
		// TODO: property-based selection here
		selectedNetletStr = netlets.front()->getId();

	}

	if (selectedNetletStr.empty()) {
		// oops
		DBG_DEBUG(FMT("No suitable Netlet found for service %1%!") % appc->getIdentifier());
		notifyListeners(Event_NoSuitableNetlet(appc));
		return;
	}

	INetlet* selectedNetlet = nena->getInstanceOf(selectedNetletStr);
	if (selectedNetlet == NULL)
		// TODO: to be handled more gracefully
		DBG_FAIL(FMT("Netlet \"%1%\" not found!") % selectedNetletStr);

	selectNetlet(appc, selectedNetlet);
}

void CNetletSelector::getActiveApps(std::list<IAppConnector*>& apps)
{
	apps.clear();

	boost::shared_lock<boost::shared_mutex> appConnLock(appConnMutex);
	map<CFlowState::FlowId, IAppConnector*>::iterator it;
	for (it = appConns.begin(); it != appConns.end(); it++) {
		if ((it->second->getFlowState() != NULL) &&
			(it->second->getFlowState()->getOperationalState() == CFlowState::s_valid) &&
			(it->second->getNetlet() != NULL))
		{
			apps.push_back(it->second);
		}
	}
}

/**
 * @brief	Parse a XML requirement node
 */
void CNetletSelector::parseRequirementNode(sxml::XmlNode* reqNode, CWeightedProperty* parent)
{
	string id, name, desc, weight;
	double weightd = 0.0;
	id = reqNode->attributes["id"];
	if (!id.empty()) {
		sxml::XmlNode* n = reqNode->findFirst("name");
		if (n != NULL && n->children.size() == 1)
			name = n->children[0]->name;
		else
			DBG_WARNING(
					FMT("XML tag '%1%' in file %2% has unexpected children count") % "name" % NETLETSELECTOR_SYSTEMPOLICIESFILE);

		n = reqNode->findFirst("desc");
		if (n != NULL) {
			if (n->children.size() == 1)
				desc = n->children[0]->name;
			else if (n->children.size() > 1)
				DBG_WARNING(
						FMT("XML tag '%1%' in file %2% has unexpected children count") % "desc" % NETLETSELECTOR_SYSTEMPOLICIESFILE);

		}

		n = reqNode->findFirst("weight");
		if (n != NULL && n->children.size() == 1) {
			weight = n->children[0]->name;
			weightd = atof(weight.c_str());

		} else {
			DBG_WARNING(
					FMT("XML tag '%1%' in file %2% has unexpected children count") % "weight" % NETLETSELECTOR_SYSTEMPOLICIESFILE);

		}

		CWeightedProperty* prop = new CWeightedProperty(id, name, NULL, weightd, parent);
		prop->setDescription(desc);

		n = reqNode->findFirst("subreqs");
		if (n != NULL && n->children.size() > 0) {
			sxml::NodeChildren::const_iterator childit;
			for (childit = n->children.begin(); childit != n->children.end(); childit++) {
				if ((*childit)->name == "requirement")
					parseRequirementNode(*childit, prop);
				else
					DBG_WARNING(FMT("Unexpected XML tag \"%1%\"") % (*childit)->name);
			}

		}

	} else {
		DBG_WARNING(
				FMT("XML tag '%1%' in file %2% has a missing attribute") % reqNode->name % NETLETSELECTOR_SYSTEMPOLICIESFILE);

	}

}

/**
 * @brief	Refreshes the system policies for Netlet selection
 */
void CNetletSelector::refreshSystemPolicies()
{
	if (systemPolicies != NULL)
		delete systemPolicies;
	systemPolicies = new CWeightedProperty("netprop://policies/netletSelection", "System Policies", NULL, 1.0);

	sxml::XmlNode xmlTree;
	ifstream ifs(NETLETSELECTOR_SYSTEMPOLICIESFILE);
	if (ifs.good()) {
		try {
			xmlTree.readFromStream(ifs, true);

		} catch (sxml::Exception e) {
			DBG_WARNING(FMT("Error %1% parsing %2%, ignoring") % e % NETLETSELECTOR_SYSTEMPOLICIESFILE);

		}

	} else {
		DBG_INFO(FMT("Cannot open %1%, ignoring...") % NETLETSELECTOR_SYSTEMPOLICIESFILE);

	}

	if (xmlTree.complete) {
		sxml::XmlNode* polTree = xmlTree.findFirst("systemPolicies");
		if (polTree == NULL) {
			DBG_WARNING(
					FMT("XML tag '%1%' in file %2% not found") % "systemPolicies" % NETLETSELECTOR_SYSTEMPOLICIESFILE);
			return;
		}

		sxml::NodeChildren::const_iterator childit;
		for (childit = polTree->children.begin(); childit != polTree->children.end(); childit++) {
			if ((*childit)->name == "requirement")
				parseRequirementNode(*childit, systemPolicies);
			else
				DBG_WARNING(FMT("Unexpected XML tag \"%1%\"") % (*childit)->name);
		}

	}

	DBG_INFO(FMT("Current system policies:\n%1%") % systemPolicies->toStr(true, "    "));
}
