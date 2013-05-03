/** @file
 * systemOmnet.h
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

#ifndef SYSTEMOMNET_H_
#define SYSTEMOMNET_H_

#include "systemWrapper.h"

#include "messagesOmnet.h"
#include "netAdaptOmnet.h"

#include "appConnector.h"
#include "messages.h"

#include <string>
#include <map>
#include <list>

#include <boost/shared_ptr.hpp>

#define SYS_OMNET_MTU	2048

class CNena;

class CDebugChannel : public IDebugChannel
{
	private:
	/**
	 * @brief Print a message.
	 */
	void print(std::string msg);

	/**
	 * @brief Print a formatted message.
	 */
	void print(boost::format fmt);

	public:

	CDebugChannel(VerbosityLevel vl):IDebugChannel(vl) {};
	virtual ~CDebugChannel() {};
	/**
	 * @brief Print an info message.
	 */
	virtual void info(boost::format fmt);

	/**
	 * @brief Print an debug message.
	 */
	virtual void debug(boost::format fmt);

	/**
	 * @brief Print an warning message.
	 */
	virtual void warning(boost::format fmt);

	/**
	 * @brief Print an error message.
	 */
	virtual void error(boost::format fmt);

	/**
	 * @brief Print an error message and terminate execution.
	 */
	virtual void fail(boost::format fmt);

};

/* ========================================================================= */

/**
 * @brief omnet specific app connector
 *
 * Spawns all predefined app connections on start
 */

class COmnetAppServer : public IAppServer
{
	private:
	std::list<boost::shared_ptr<IAppConnector> > conlist;
	CNena * nodeArch;

	public:
	COmnetAppServer (CNena * na, IMessageScheduler * ms): IAppServer(ms), nodeArch (na) {}
	virtual ~COmnetAppServer () {}

	/// start all servers
	virtual void start ();
	/// stop server
	virtual void stop ();

	virtual const std::string & getId () const;

	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/// start all spawned app connectors
	virtual void startAppConnectors ();
	/// stop all spawned app connectors
	virtual void stopAppConnectors ();
};

/* ========================================================================= */

/**
 * @brief OMNeT++ Wrapper
 *
 * CSystemOmnet is a Omnet++ module that registers itself at Omnet. It gets
 * instantiated by Omnet for every node as defined in the .ned-file. During
 * initialization, it instantiates the Node Architecture - hence, we have
 * *multiple* instances within the Omnet address space. This is different
 * to standalone targets, where the Node Architecture is really a singleton.
 */
class CSystemOmnet : public ISystemWrapper, public COmnetScheduler
{
private:
	CNena *nena;								///< Pointer to node architecture daemon
	std::map<std::string, COmnetNetAdapt *> omnetNetAdapts;	///< Pointer to OMNeT network accesses (key: gate name)
	IDebugChannel * chan;						///< debug channel

	std::list<boost::shared_ptr<IAppServer> > appServers;	///< List of avaiable App Servers

protected:
	virtual void initialize(int stage);			///< From cSimpleModule
	virtual void handleMessage(cMessage *msg);	///< From cSimpleModule

	public:
	CSystemOmnet();								///< Constructor
	virtual ~CSystemOmnet();					///< Destructor

	virtual int numInitStages() const {return 2;};	///< From cSimpleModule

	virtual void run() {};						///< does nothing in omnet

	virtual void stop () {}

	virtual void end () {};

	/**
	 * @brief	Returns the system-specific main scheduler (e.g. main thread)
	 */
	virtual IMessageScheduler *getMainScheduler() const;

	/**
	 * @brief	Returns a system-specific scheduler.
	 */
	virtual IMessageScheduler *schedulerFactory ();

	virtual void initSchedulers (CNena * na);
	virtual IMessageScheduler * lookupScheduler (IMessageProcessor * proc);

	virtual void initNetAdapts(CNena * nodearch, IMessageScheduler * sched); ///< Initialize network accesses
	virtual void getNetAdapts(std::list<INetAdapt *>& netAdapts);		///< Initialize network accesses
	virtual void releaseNetAdapts();			///< Release network accesses

	/**
	 * @brief 	Initialize application interface
	 *
	 * @param nodearch	Pointer to Daemon object
	 * @param sched		Pointer to scheduler to use for application interfaces
	 */
	virtual void initAppServers(CNena * nodearch, IMessageScheduler * sched);

	/**
	 * @brief 	Return list of App Servers
	 *
	 * @param appServers	List to which the App Servers will be appended
	 */
	virtual void getAppServers(std::list<boost::shared_ptr<IAppServer> > &);

	/**
	 * @brief	Tear down application interfaces
	 */
	virtual void closeAppServers();

	/**
	 * @brief Returns a system-specific synch Factory
	 */
	virtual ISyncFactory * getSyncFactory ();

	/**
	 * @brief Returns the system specific debug channel
	 */
	virtual IDebugChannel * getDebugChannel ();

	virtual std::string getNodeName() const;			///< Return the current node's name
	virtual double getSysTime();				///< Return the current system time in seconds
	virtual double random();					///< Returns a random number between [0, 1]

	virtual void setIcon(const std::string& iconName); ///< Update the graphical representation of of the daemon

	/**
	 * @brief return a unique name string for this MessageProcessor
	 */
	virtual const std::string & getId () const;

	/**
	 * @brief return a pointer to the scheduler with name or Null pointer, if not exists
	 */
	virtual IMessageScheduler * getSchedulerByName (std::string name);
};

#endif /* SYSTEMOMNET_H_ */
