/*
 * nena.cpp
 *
 *  Created on: Dec 8, 2008
 *      Author: denis
 */

#include "nena.h"
#include "netAdaptBroker.h"
#include "netAdapt.h"
#include "netletSelector.h"
#include "localRepository.h"
#include "management.h"

#include <appConnector.h>

#include "debug.h"

#include <list>
#include <exception>

#include "boost/algorithm/string/trim.hpp"
#include <boost/shared_ptr.hpp> 

#define NENA_FLOWSTATE_STALEAGE	10

using namespace std;
using namespace boost;
using boost::shared_ptr;
using boost::shared_mutex;
using boost::shared_lock;
using boost::unique_lock;

/* ========================================================================= */

/**
 * @brief Constructor
 *
 * Set up initial daemon configuration
 */
CNena::CNena(ISystemWrapper* sys)
{
	management.reset(new Management(sys, this));
	config.reset(new NenaConfig(getNodeName()));
	lastFlowId = 0;

	DBG_DEBUG(FMT("\n====== NA Daemon on \"%1%\" started ======\n") % getNodeName());
}

/**
 * @brief Constructor
 */
CNena::~CNena()
{
	DBG_DEBUG(FMT("\n====== NA Daemon on \"%1%\" shut down ======\n") % getNodeName());
	management->disassemble();
	management.reset(NULL);
}

/**
 * @brief Initialize daemon.
 *
 * Loading multiplexers, Netlets, initialize network accesses, application
 * interface, etc.
 */
void CNena::init()
{
	management->assemble();
}

/**
 * @brief	Start main event loop.
 *
 * 			Starts default Scheduler, then system wrapper and application interface.
 */
void CNena::run()
{
	DBG_DEBUG("\n=== Entering main loop ===\n");
	management->run();

}

/**
 * @brief Stops main event loop and all other activity
 */
void CNena::stop()
{
	DBG_DEBUG("\n=== Ok, nothing to do anymore, exiting... ===\n");
	management->stop();
}

/**
 * @brief	Register a NENA internal service.
 */
void CNena::registerInternalService(const std::string& serviceId, IMessageProcessor* service)
{
	assert(!serviceId.empty() && service != NULL);

	unique_lock<shared_mutex>(internalServicesMutex);
	map<string, IMessageProcessor*>::const_iterator it = internalServices.find(serviceId);
	if (it == internalServices.end()) {
		internalServices[serviceId] = service;
		DBG_INFO(FMT("Registering internal service: %1% (%2%)") % serviceId % service->getClassName());

	} else {
		if (it->second == service) {
			DBG_DEBUG(FMT("Internal service %1% (%2%) already registered.") % serviceId % service->getClassName());

		} else {
			DBG_DEBUG(
					FMT("Cannot register internal service %1% (%2%): Already registered as %3%.") % serviceId % service->getClassName() % it->second->getClassName());

		}

	}
}

/**
 * @brief	Unregister a NENA internal service.
 */
void CNena::unregisterInternalService(const std::string& serviceId, IMessageProcessor* service)
{
	assert(!serviceId.empty());

	unique_lock<shared_mutex>(internalServicesMutex);
	map<string, IMessageProcessor*>::iterator it = internalServices.find(serviceId);
	if (it != internalServices.end()) {
		internalServices.erase(it);

	} else {
		DBG_DEBUG(FMT("Internal service %1% not registered, cannot unregister.") % serviceId);

	}
}

/**
 * @brief	Lookup a NENA internal service.
 * 			Note: You should not call any methods directly since this is
 * 			normally not thread-safe. In addition, validity of the pointer
 * 			cannot be guaranteed. Use it just as address for your messages
 * 			to this service.
 */
IMessageProcessor* CNena::lookupInternalService(const std::string& serviceId)
{
	shared_lock<shared_mutex>(internalServicesMutex);
	map<string, IMessageProcessor*>::const_iterator it = internalServices.find(serviceId);
	if (it != internalServices.end()) {
		return it->second;

	} else {
		DBG_DEBUG(FMT("Lookup for unknown internal service %1%") % serviceId);

	}

	return NULL;
}

/**
 * @brief	Get all instantiated Netlets belonging to architecture archName.
 *
 * @param archName	Name of architecture
 * @param list		List to be filled (will be cleared if non-empty)
 */
void CNena::getNetlets(std::string archName, std::list<INetlet*>& netletList)
{
	netletList.clear();
	list<INetlet*>::const_iterator it;
	for (it = management->getRepository()->netlets.begin(); it != management->getRepository()->netlets.end(); it++)
		if ((*it)->getMetaData()->getArchName() == archName)
			netletList.push_back(*it);
}

/**
 * @brief	Return the multiplexer for the given architecture
 *
 * @param archName	Name or architecture
 */
INetletMultiplexer* CNena::getMultiplexer(std::string archName)
{
	list<INetletMultiplexer *>::const_iterator it;
	for (it = management->getRepository()->multiplexers.begin(); it != management->getRepository()->multiplexers.end();
			it++)
		if ((*it)->getMetaData()->getArchName() == archName)
			return *it;

	return NULL;
}
/**
 * @brief	Returns an instance of the given Netlet
 */
INetlet* CNena::getInstanceOf(const std::string& netletName)
{
	// TODO: instantiate a Netlet if none found
	list<INetlet*>::const_iterator it;
	for (it = management->getRepository()->netlets.begin(); it != management->getRepository()->netlets.end(); it++)
		if ((*it)->getMetaData()->getId() == netletName)
			return *it;

	return NULL;
}

/**
 * @brief	Returns a value for the given configuration parameter key
 *
 * @param	id	Component ID
 * @param	key	Key of config parameter
 *
 * @return	Value of config parameter
 */
std::string CNena::getConfigValue(const std::string& id, const std::string& key)
{
	string value;

	try {
		getConfig()->getParameter(id, key, value);

	} catch(std::exception& e) {
		DBG_DEBUG(FMT("%1%: No config value for %2%") % id % key);
		return string();

	}

	return value;
}

const string CNena::getNodeName()
{
	return management->getSys()->getNodeName();
}
;

IMessageScheduler * CNena::getDefaultScheduler() const
{
	return management->getSys()->getMainScheduler();
}

ISystemWrapper * CNena::getSys()
{
	return management->getSys();
}
;

CNetAdaptBroker * CNena::getNetAdaptBroker() const
{
	return management->getNetAdaptBroker();
}

CNetletSelector * CNena::getNetletSelector() const
{
	return management->getNetletSelector();
}

INetletRepository * CNena::getRepository() const
{
	return management->getRepository();
}

Management * CNena::getManagement() const
{
	return management.get();
}

double CNena::getSysTime()
{
	return management->getSys()->getSysTime();
}

/**
 * @brief looks up scheduler which will handle messages to given message processor
 *
 * @param 	proc	MessageProcessor to look for
 * @return 	Associated Message Scheduler or NULL if there is none
 */
IMessageScheduler * CNena::lookupScheduler(IMessageProcessor * proc)
{
	return management->getSys()->lookupScheduler(proc);
}

IMessageScheduler * CNena::getSchedulerByName(std::string name) const
{
	return management->getSys()->getSchedulerByName(name);
}

shared_ptr<NenaConfig> CNena::getConfig() const
{
	return config;
}

/**
 * @brief	Create a new flow state object
 */
boost::shared_ptr<CFlowState> CNena::createFlowState(IMessageProcessor *owner)
{
	CFlowState::FlowId id = 0;
	shared_ptr<CFlowState> state;

	{
		unique_lock<shared_mutex>(flowStateMutex);
		id = ++lastFlowId;

		state = shared_ptr<CFlowState>(new CFlowState(id, owner));
		flowStates[id] = state;
	}

	return state;
}

/**
 * @brief	Releases an obsolete flow state object. It should no longer be
 * 			accessed afterwards.
 */
void CNena::releaseFlowState(boost::shared_ptr<CFlowState> flowState)
{
	assert(flowState.get() != NULL);

	flowState->setOperationalState(NULL, CFlowState::s_stale);
	flowState->cleanUp();

	DBG_DEBUG(FMT("NC: releasing flow state %1% (refcount %2%)") % flowState->getFlowId() % flowState.use_count());

	unique_lock<shared_mutex>(flowStateMutex);
	map<CFlowState::FlowId, shared_ptr<CFlowState> >::iterator it = flowStates.find(flowState->getFlowId());
	assert(it != flowStates.end());
	flowStates.erase(it);
}

boost::shared_ptr<CFlowState> CNena::getFlowState(CFlowState::FlowId flowId)
{
	shared_lock<shared_mutex>(flowStateMutex);
	map<CFlowState::FlowId, shared_ptr<CFlowState> >::iterator it;
	it = flowStates.find(flowId);
	if (it != flowStates.end()) {
		return it->second;
	}
	return boost::shared_ptr<CFlowState>();
}

void CNena::lookupFlowStates(const std::string& remoteUri, std::list<boost::shared_ptr<CFlowState> >& flowStates)
{
	flowStates.clear();

	shared_lock<shared_mutex>(flowStateMutex);
	map<CFlowState::FlowId, shared_ptr<CFlowState> >::iterator it;
	for (it = this->flowStates.begin(); it != this->flowStates.end(); it++) {
		if (it->second->getServiceId() == remoteUri) {
			flowStates.push_back(it->second);
		}
	}
}
