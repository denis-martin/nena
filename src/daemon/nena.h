/** @file
 * nena.h
 *
 * @brief Netlet-based Node Architecture daemon
 *
 * (c) 2008-2012 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 8, 2008
 *      Author: denis
 */

#ifndef NENA_H_
#define NENA_H_

#include "netlet.h"
#include "netletMultiplexer.h"
#include "netletRepository.h"
#include "appConnector.h"
#include "flowState.h"
#include "systemWrapper.h"
#include "debug.h"
#include "nenaconfig.h"

#include <string>

#include <boost/thread/shared_mutex.hpp>
#include <boost/scoped_ptr.hpp>

class Management;
class CNetAdaptBroker;
class CNetletSelector;

/**
 * @brief Netlet-based Node Architecture daemon class
 *
 * Conceptually, this is a singleton class. But since it may run in a
 * simulator, more than one instance may exist in the same executable.
 *
 * TODO: All public methods should be thread-safe.
 */
class CNena
{
private:
	boost::scoped_ptr<Management> management;	///< management class
	boost::shared_ptr<NenaConfig> config;		///< wrapper to configuration files

	boost::shared_mutex internalServicesMutex;
	std::map<std::string, IMessageProcessor*> internalServices;		///< Internal NENA services

	boost::shared_mutex flowStateMutex;
	CFlowState::FlowId lastFlowId;									///< last assigned flow id
	std::map<CFlowState::FlowId, boost::shared_ptr<CFlowState> > flowStates;

public:

	CNena(ISystemWrapper* sys);							///< Constructor
	virtual ~CNena();									///< Destructor

	void init();										///< initialize daemon
	void run();											///< start main event loop; calls ISystemWrapper::run()
	void stop ();										///< stops main event loop
        
        /**
	 * @brief return a pointer to the default scheduler
	 */
	IMessageScheduler * getDefaultScheduler () const;

	/**
	 * @brief return a pointer to the scheduler with name or Null pointer, if not exists
	 */
	IMessageScheduler * getSchedulerByName (std::string name) const;

	/**
	 * @brief Return an arbitrary name for the node.
	 */
	const std::string getNodeName();

	/**
	 * @brief	Register a NENA internal service.
	 */
	void registerInternalService(const std::string& serviceId, IMessageProcessor* service);

	/**
	 * @brief	Unregister a NENA internal service.
	 */
	void unregisterInternalService(const std::string& serviceId, IMessageProcessor* service = NULL);

	/**
	 * @brief	Lookup a NENA internal service.
	 *
	 * 			Note: You should not call any methods of the returned
	 * 			IMessageProcessor directly since this is not thread-safe. In
	 * 			addition, validity of the pointer cannot be guaranteed if you
	 * 			store it. Use it just as address for your messages to this
	 * 			service.
	 */
	IMessageProcessor* lookupInternalService(const std::string& serviceId);
        
	/**
	 * @brief 	Return the NA Broker
	 *
	 * @deprecated Use lookupInternalService("internalservice://nena/netAdaptBroker") instead.
	 */
	CNetAdaptBroker *getNetAdaptBroker() const;

	/**
	 * @brief Return the Netlet selector
	 *
	 * @deprecated Use lookupInternalService("internalservice://nena/netletSelector") instead.
	 */
	CNetletSelector *getNetletSelector() const;

	/**
	 * @brief Return the repository
	 *
	 * @deprecated Use lookupInternalService("internalservice://nena/netletRepository") instead.
	 */
	INetletRepository *getRepository() const;
        
        /**
	 * @brief Return the management unit
	 */
	Management * getManagement () const;

	/**
	 * @brief	Return a pointer to the system wrapper.
	 *
	 * 			Same as lookupInternalService("internalservice://nena/system"),
	 * 			but faster.
	 */
	ISystemWrapper* getSys();

	/**
	 * @brief	Get all instantiated Netlets belonging to architecture archName.
	 *
	 * @param archName	Name of architecture
	 * @param list		List to be filled (will be cleared if non-empty)
	 */
	virtual void getNetlets(std::string archName, std::list<INetlet*>& netletList);

	/**
	 * @brief	Return the multiplexer for the given architecture
	 *
	 * @param archName	Name or architecture
	 */
	virtual INetletMultiplexer* getMultiplexer(std::string archName);

	/**
	 * @brief	Returns an instance of the given Netlet
	 */
	virtual INetlet* getInstanceOf(const std::string& netletName);

	/**
	 * @brief	Returns the system time in seconds (with at least milli-second
	 * 			precision)
	 */
	virtual double getSysTime();

	/**
	 * @brief	Returns a value for the given configuration parameter key
	 *
	 * @param	id	Component ID
	 * @param	key	Key of config parameter
	 *
	 * @return	Value of config parameter
	 */
	virtual std::string getConfigValue(const std::string& id, const std::string& key);

	/**
	 * @brief looks up scheduler which will handle messages to given message proessor
	 *
	 * @param 	proc	MessageProcessor to look for
	 * @return 	Associated Message Scheduler or NULL if there is none
	 */
	virtual IMessageScheduler * lookupScheduler (IMessageProcessor * proc);


	/**
	 * @fn boost::shared_ptr<NenaConfig> getConfig () const;
	 *
	 * @brief return a pointer to nena configuration
	 *
	 * @return a pointer to nena configuration
	 */
	boost::shared_ptr<NenaConfig> getConfig () const;

	boost::shared_ptr<CFlowState> createFlowState(IMessageProcessor *owner);
	void releaseFlowState(boost::shared_ptr<CFlowState> flowState);

	/**
	 * @brief	Returns the flow state object of a given FlowId
	 */
	boost::shared_ptr<CFlowState> getFlowState(CFlowState::FlowId flowId);

	/**
	 * @brief	Searches for flow IDs for a given remote URI.
	 *
	 * @param remoteUri		Remote URI of the request
	 * @param flowStates	List for results (list is cleared before storing results)
	 */
	void lookupFlowStates(const std::string& remoteUri, std::list<boost::shared_ptr<CFlowState> >& flowStates);
};

#endif /* NENA_H_ */
