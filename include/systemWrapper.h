/** @file
 * systemWrapper.h
 *
 * @brief Generic interface for system wrappers.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 10, 2008
 *      Author: denis
 */

#ifndef SYSTEMWRAPPER_H_
#define SYSTEMWRAPPER_H_

#include <string>
#include <list>

#include <appConnector.h>

#include "messages.h"
#include "netAdapt.h"
#include "sync.h"
#include "debug.h"

#include <list>

#include <boost/shared_ptr.hpp>
#include <exceptions.h>

class CNena;

/**
 * @brief Host system wrapper.
 *
 * May not be static since there may be several instances when used in a
 * simulator.
 */
class ISystemWrapper
{
public:

	/**
	 * @brief System Wrapper failed to load config
	 */
	NENA_EXCEPTION(EConfig);

	/**
	 * @brief 	Constructor.
	 */
	ISystemWrapper() {};

	/**
	 * @brief 	Destructor.
	 */
	virtual ~ISystemWrapper() {};

	/**
	 * @brief give nodearch to schedulers
	 */
	virtual void initSchedulers (CNena * na) = 0;

	/**
	 * @brief	Returns the system-specific main scheduler (e.g. main thread)
	 */
	virtual IMessageScheduler *getMainScheduler() const = 0;

	/**
	 * @brief looks up scheduler which will handle messages to given message proessor
	 *
	 * @param 	proc	MessageProcessor to look for
	 * @return 	Associated Message Scheduler or NULL if there is none
	 */
	virtual IMessageScheduler * lookupScheduler (IMessageProcessor * proc) = 0;

	/**
	 * @brief return a pointer to the scheduler with name or Null pointer, if not exists
	 */
	virtual IMessageScheduler * getSchedulerByName (std::string name) = 0;

	/**
	 * @brief	Returns a system-specific scheduler.
	 */
	virtual IMessageScheduler *schedulerFactory() = 0;

	/**
	 * @brief Returns a system-specific synch Factory
	 */
	virtual ISyncFactory * getSyncFactory () = 0;

	/**
	 * @brief Returns the system specific debug channel
	 */
	virtual IDebugChannel * getDebugChannel () = 0;

	/**
	 * @brief	Initialize network accesses
	 *
	 * @param nodearch	Pointer to Daemon object
	 * @param sched		Pointer to scheduler to use for Network Accesses
	 */
	virtual void initNetAdapts(CNena * nodearch, IMessageScheduler * sched) = 0;

	/**
	 * @brief 	Return list of Network Accesses.
	 *
	 * @param netAdapts	List to which the Network Access will be appended
	 */
	virtual void getNetAdapts(std::list<INetAdapt *>& netAdapts) = 0;

	/**
	 * @brief 	Release previously initialized network accesses.
	 */
	virtual void releaseNetAdapts() = 0;

	/**
	 * @brief 	Initialize application interface
	 *
	 * @param nodearch	Pointer to Daemon object
	 * @param sched		Pointer to scheduler to use for application interfaces
	 */
	virtual void initAppServers(CNena * nodearch, IMessageScheduler * sched) = 0;

	/**
	 * @brief 	Return list of App Servers
	 *
	 * @param appServers	List to which the App Servers will be appended
	 */
	virtual void getAppServers(std::list<boost::shared_ptr<IAppServer> > &) = 0;

	/**
	 * @brief	Tear down application interfaces
	 */
	virtual void closeAppServers() = 0;

	/**
	 * @brief	Return an arbitrary name for the node.
	 */
	virtual std::string getNodeName() const = 0;

	/**
	 * @brief	Returns the system time in seconds (with at least milli-second
	 * 			precision)
	 */
	virtual double getSysTime() = 0;

	/**
	 * @brief	Returns a random number between [0, 1].
	 */
	virtual double random() = 0;

	/**
	 * @brief	Update the graphical representation of if the daemon where
	 * 			applicable.
	 *
	 * @param iconName	Name of the new graphical representation
	 */
	virtual void setIcon(const std::string& iconName) = 0;

	/**
	 * @brief start message processing on all schedulers
	 */
	virtual void run () = 0;

	/**
	 * @brief stop message processing on all schedulers
	 */
	virtual void stop () = 0;
        
	/**
	 * @brief end system operations
	 */
	virtual void end () = 0;

};

#endif /* SYSTEMWRAPPER_H_ */
