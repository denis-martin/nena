/**
 * systemOmnet.cpp
 *
 * @brief OMNeT++ system wrapper
 *
 * CSystemOmnet is a Omnet++ module that registers itself at Omnet. It gets
 * instantiated by Omnet for every node as defined in the .ned-file. During
 * initialization, it instantiates the Node Architecture - hence, we have
 * *multiple* instances within the Omnet address space. This is different
 * to standalone targets, where the Node Architecture is really a singleton.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Jan 14, 2009
 *      Author: denis
 */

#include "nena.h"
#include "debug.h"
#include "netletSelector.h"
#include "netAdaptBroker.h"

#include "edu.kit.tm/itm/simpleArch/simpleMultiplexer.h"

#include "systemOmnet.h"

#include "messagesOmnet.h"
#include "netAdaptOmnet.h"

// traffic generator apps
#include "apps/pingPong.h"
#include "apps/cbr.h"

// C
#include <assert.h>

// C++
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

// OMNeT++
#include <omnetpp.h>

// boost
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>

using boost::shared_ptr;

using namespace std;

static const string & systemOmnetId = "system://edu.kit.tm/itm/omnet/system";
static const string & appServerName = "appServer://omnet/default";

#define NETADAPT_OMNET_NAME "netadapt://edu.kit.tm/itm/omnetpp"

// Pointer to Architecture specific channel, filled by nodearch
IDebugChannel * globalChannel;

/**
 * @brief Print a message.
 */
void CDebugChannel::print(std::string msg)
{
	std::cerr << boost::format("%1$09.3f %2%") % simTime().dbl() % msg << std::endl;
};

/**
 * @brief Print a formatted message.
 */
void CDebugChannel::print(boost::format fmt)
{
	std::cerr << boost::format("%1$09.3f %2%") % simTime().dbl() % fmt << std::endl;
};

/**
 * @brief Print an info message.
 */
void CDebugChannel::info(boost::format fmt)
{
	if (vlevel >= INFO)
		print (boost::format("INF: %1%") % fmt);
};

/**
 * @brief Print an debug message.
 */
void CDebugChannel::debug(boost::format fmt)
{
	if (vlevel >= DEBUG)
		print (boost::format("DBG: %1%") % fmt);
};

/**
 * @brief Print an warning message.
 */
void CDebugChannel::warning(boost::format fmt)
{
	if (vlevel >= WARNING)
		print (boost::format("WRN: %1%") % fmt);
};

/**
 * @brief Print an error message.
 */
void CDebugChannel::error(boost::format fmt)
{
	if (vlevel >= ERROR)
		print (boost::format("ERR: %1%") % fmt);
};

/**
 * @brief Print an error message and terminate execution.
 */
void CDebugChannel::fail(boost::format fmt)
{
	print (boost::format("FAIL: %1%") % fmt);
	assert(false);
};

/* ========================================================================= */

void COmnetAppServer::start ()
{
	IAppConnector * pingPong;
	vector<string> applist;
	nodeArch->getConfig()->getParameterList(getId(), "applications", applist);
	vector<string>::const_iterator it;
	for (it = applist.begin(); it != applist.end(); it++)
	{
		if (*it == APP_OMNET_PINGPONG_NAME)
		{
			pingPong = new CPingPong(nodeArch, scheduler);
			conlist.push_back (shared_ptr<IAppConnector> (pingPong));
			nodeArch->getNetletSelector()->registerAppConnector(pingPong);
		}

		if (*it == APP_OMNET_CBR_NAME)
		{
			pingPong = new CCbr(nodeArch, scheduler);
			conlist.push_back (shared_ptr<IAppConnector> (pingPong));
			nodeArch->getNetletSelector()->registerAppConnector(pingPong);
		}
	}
}

void COmnetAppServer::stop ()
{
	conlist.clear ();
}

const std::string & COmnetAppServer::getId () const
{
	return appServerName;
}
void COmnetAppServer::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}
void COmnetAppServer::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}
void COmnetAppServer::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}
void COmnetAppServer::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

void COmnetAppServer::startAppConnectors ()
{
	return;
}

void COmnetAppServer::stopAppConnectors ()
{
	return;
}

/* ========================================================================= */

// The module class needs to be registered with OMNeT++
Define_Module(CSystemOmnet);

/**
 * Constructor
 */
CSystemOmnet::CSystemOmnet():
	COmnetScheduler(NULL), nena(NULL)
{
	omnetNetAdapts.clear();

	chan = new CDebugChannel (IDebugChannel::INFO);
	globalChannel = chan;
}

/**
 * Destructor
 */
CSystemOmnet::~CSystemOmnet()
{
	if (nena != NULL) {
		nena->stop();
		delete nena;
	}

	releaseNetAdapts();

	delete chan;
}

/**
 * cSimpleModule::initialize()
 */
void CSystemOmnet::initialize(int stage)
{
	if (stage == 0) {
		setIcon("laptop");
		COmnetScheduler::initialize();
		nena = new CNena(this);
		nena->init(); // should be the only upcall


	}
	else if (stage == 1)
	{
		map<string, COmnetNetAdapt *>::const_iterator it;
		for (it = omnetNetAdapts.begin(); it != omnetNetAdapts.end(); it++)
		{
			it->second->configureInterface();
		}
		nena->run();

	}
}

/**
 * cSimpleModule::handleMessage()
 */
void CSystemOmnet::handleMessage(cMessage *msg)
{
	if (msg->isPacket()) {
		assert(msg->getArrivalGate() != NULL);

		string arrivalGate = msg->getArrivalGate()->getFullName();
		map<string, COmnetNetAdapt *>::iterator it = omnetNetAdapts.find(arrivalGate);
		if (it != omnetNetAdapts.end()) {
			COmnetPacket *opkt = (COmnetPacket *) msg;
			it->second->handleOmnetPacket(opkt);

		} else {
			DBG_WARNING(FMT("OmnetSys: WARNING: Received packet via unknown gate \"%1%\"") % arrivalGate);

		}

	} else {
		// lookup event
		EventMap::iterator it = events.find(msg);
		if (it != events.end()) {
			shared_ptr<IMessage> event = it->second;
			if (event->getTo() != NULL) {
				if (event->getTo()->getMessageScheduler() != this)
					DBG_FAIL("Multiple schedulers not yet supported");
				sendMessage(event); // enqueue event

				map<shared_ptr<IMessage>, cMessage *>::iterator rit = reverseEvents.find(event);
				if (rit != reverseEvents.end()) {
					reverseEvents.erase(rit);
				}
				events.erase(it);

			} else {
				DBG_WARNING("OmnetSys: Got event with no processor associated!");

			}

		} else {
			DBG_WARNING("OmnetSys: Got unhandled OMNeT++ message!");

		}

	}

	// deliver all pending messages
	processMessages();

	delete msg;
}

/**
 * @brief	Returns a system-specific scheduler.
 */
IMessageScheduler * CSystemOmnet::schedulerFactory ()
{
	return this;
}

void CSystemOmnet::initSchedulers (CNena * na)
{
	return;
}

IMessageScheduler * CSystemOmnet::lookupScheduler (IMessageProcessor * proc)
{
	return this;
}

/**
 * @brief	Initialize network accesses
 *
 * @param nodearch	Pointer to Daemon object
 * @param sched		Pointer to scheduler to use for Network Accesses
 */
void CSystemOmnet::initNetAdapts(CNena * nodearch, IMessageScheduler * sched)
{
	assert(nodearch == nena);
	assert(sched == this);

	vector<const char *> gateNames = getGateNames();
	vector<const char *>::const_iterator it;
	for (it = gateNames.begin(); it != gateNames.end(); it++) {
		int n = gateSize(*it);
		for (int i = 0; i < n; i++) { // always assuming vector gates
			string fullInName = (FMT("%1%$i[%2%]") % *it % i).str();
			// only consider gates that start with "eth"
			if (fullInName.substr(0, 3) == "eth" && omnetNetAdapts[fullInName] == NULL)
				omnetNetAdapts[fullInName] = new COmnetNetAdapt(nena, this, SIMPLE_ARCH_NAME, NETADAPT_OMNET_NAME, *it, i);
		}
	}
}

/**
 * @brief 	Return list of Network Accesses.
 *
 * @param netAdapts	List to which the Network Access will be appended
 */
void CSystemOmnet::getNetAdapts(std::list<INetAdapt *>& netAdapts)
{
	assert(netAdapts.size() == 0);
	assert(omnetNetAdapts.size() > 0);

	map<std::string, COmnetNetAdapt *>::const_iterator it;
	for (it = omnetNetAdapts.begin(); it != omnetNetAdapts.end(); it++) {
		netAdapts.push_back(it->second);
	}
}

/**
 * Release previously initialized network accesses
 */
void CSystemOmnet::releaseNetAdapts()
{
	map<string, COmnetNetAdapt *>::iterator it;
	for (it = omnetNetAdapts.begin(); it != omnetNetAdapts.end(); it++) {
		if (it->second != NULL)
			delete it->second;

	}
	omnetNetAdapts.clear();
}

/**
 * @brief 	Initialize application interface
 *
 * @param nodearch	Pointer to Daemon object
 * @param sched		Pointer to scheduler to use for application interfaces
 */
void CSystemOmnet::initAppServers(CNena * nodearch, IMessageScheduler * sched)
{
	appServers.push_back (shared_ptr<IAppServer> (new COmnetAppServer(nodearch, this)));
}

void CSystemOmnet::getAppServers(list<shared_ptr<IAppServer> > & appServers)
{
	appServers.insert (++appServers.begin (), this->appServers.begin (), this->appServers.end ());
}


/**
 * Release / tear down application interface
 */
void CSystemOmnet::closeAppServers()
{
	appServers.clear ();
}

ISyncFactory * CSystemOmnet::getSyncFactory ()
{
	return NULL;
}

/**
 * @brief Returns the system specific debug channel
 */
IDebugChannel * CSystemOmnet::getDebugChannel ()
{
	return chan;
}

/**
 * Return an arbitrary name for the node
 */
std::string CSystemOmnet::getNodeName() const
{
	string nodeName = par("nodeName");
	if (nodeName != "") {
		return nodeName;

	} else {
		return string("node://edu.kit.tm/itm/test/") + getFullName();

	}
}

/**
 * @brief	Returns the system time in seconds (with at least milli-second
 * 			precision)
 */
double CSystemOmnet::getSysTime()
{
	return simTime().dbl();
}

/**
 * @brief	Returns a random number between [0, 1].
 */
double CSystemOmnet::random()
{
	return getRNG(0)->doubleRandIncl1();
}

/**
 * @brief	Update the graphical representation of if the daemon where
 * 			applicable.
 *
 * @param iconName	Name of the new graphical representation
 */
void CSystemOmnet::setIcon(const std::string& iconName)
{
	cModule* parent = getParentModule();
	if (parent != NULL) {
		string icon = "i=" + iconName;
		parent->getDisplayString().updateWith(icon.c_str());
		DBG_INFO(FMT("Display string: %1%") % parent->getDisplayString().str());

	} else {
		DBG_WARNING("CSystemOmnet::setIcon(): Cannot find parent module!");

	}
}

const string & CSystemOmnet::getId () const
{
	return systemOmnetId;
}

IMessageScheduler * CSystemOmnet::getMainScheduler() const
{
	return const_cast<CSystemOmnet*> (this);
}

IMessageScheduler * CSystemOmnet::getSchedulerByName (string name)
{
	return this;
}

