/** @file
 * systemBoost.h
 *
 * @brief System wrapper for Boost supported systems (e.g. Linux, Windows)
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 10, 2008
 *      Author: denis
 *
 *  Modified on: Aug 29, 2009
 *      Author: Benjamin Behringer
 */

#ifndef SYSTEMBOOST_H_
#define SYSTEMBOOST_H_

#include "systemWrapper.h"
#include "netAdaptBoostUDP.h"
#include "socketAppConnector.h"
#include "memAppConnector.h"
#include "sync.h"
#include "debug.h"

#include <exception>
#include <list>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp> 
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/locks.hpp>

class CDebugChannel : public IDebugChannel
{
	private:

	boost::mutex m;

	/// c++ style ofstream
	std::ofstream logfile;

	/**
	 * @brief Log message to file
	 */
	void log (std::string msg);

	/**
	 * @brief Log formatted message to file
	 */
	void log (boost::format fmt);

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

	virtual void logTo (std::string filename);

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
 * @brief Boost ASIO wrapper
 */
class CSystemBoost : public ISystemWrapper
{
public:
	/**
	 * @brief	Attempt to get non-initialized data
	 *
	 * TODO: merge(r299:646): shouldn't that be defined in ISystemWrapper?
	 */
	class ENotInitialized : public std::exception
	{
	protected:
		std::string msg;

	public:
		ENotInitialized() throw () : msg(std::string()) {};
		ENotInitialized(const std::string& msg) throw () : msg(msg){};
		virtual ~ENotInitialized() throw () {};

		virtual const char* what() const throw () { return msg.c_str(); };
	};

private:
	int port; 					///< default port for net Adapts
	int nthreads;				///< number of threads to be used

	boost::posix_time::ptime daemonStartTime;	///< time of daemon start

	std::string nodeName;							///< node name

	boost::shared_mutex s_mutex;						/// protecting scheduler list
	IMessageScheduler * mainScheduler;			///< main Scheduler
	std::list<boost::shared_ptr<IMessageScheduler> > schedulers;	///< all spawned schedulers

	CNena * nodearch;					/// for schedulers...

	boost::shared_ptr<CDebugChannel> chan;					///< debug channel
		
	std::list<boost::shared_ptr<INetAdapt> > netAdapts;			///< List of avaiable net Adapts
	std::list<boost::shared_ptr<IAppServer> > appServers;			///< List of avaiable App Servers
	
	boost::shared_ptr<boost::asio::io_service> io;

	boost::shared_ptr<ISyncFactory> syncFactory;				///< Factory for Sync Primitives

	boost::shared_ptr<boost::thread> io_thread;						/// running i/o service

	/// runs io stuff
	void io_run ();

public:
	CSystemBoost(IDebugChannel::VerbosityLevel vlevel);
	CSystemBoost(IDebugChannel::VerbosityLevel vlevel, int nthreads);								///< Constructor
	virtual ~CSystemBoost();					///< Destructor

	virtual void initNetAdapts(CNena * nodearch, IMessageScheduler * sched);				///< Initialize network accesses
	virtual void getNetAdapts(std::list<INetAdapt *> & netAdapts);		///< return list of NetAdapts
	virtual void releaseNetAdapts();			///< Release network accesses

	virtual void initAppServers(CNena * nodearch, IMessageScheduler * sched);			///< Initiliaze application interface
	virtual void getAppServers(std::list<boost::shared_ptr<IAppServer> > & appServers);
	virtual void closeAppServers();			///< Tear down application interface
	
	/**
	 * @brief give nodearch to schedulers
	 */
	virtual void initSchedulers (CNena * na);

	/**
	 * @brief	Returns the system-specific main scheduler (e.g. main thread)
	 */
	virtual IMessageScheduler *getMainScheduler() const;

	/**
	 * @brief looks up scheduler which will handle messages to given message proessor
	 *
	 * @param 	proc	MessageProcessor to look for
	 * @return 	Associated Message Scheduler or NULL if there is none
	 */
	virtual IMessageScheduler * lookupScheduler (IMessageProcessor * proc);

	/**
	 * @brief return a pointer to the scheduler with name or Null pointer, if not exists
	 */
	virtual IMessageScheduler * getSchedulerByName (std::string name);

	/**
	 * @brief	Returns a system-specific scheduler.
	 */
	virtual IMessageScheduler *schedulerFactory();

	virtual std::string getNodeName() const;				///< Return the current node's name
	/**
	 * Set the port for net Adapts
	 */
	virtual void setNodePort(int p);
	virtual void setNodeName(std::string name);		///< Set the current node's name

	virtual double getSysTime();					///< Return the current system time in seconds
	virtual double random();						///< Returns a random number between [0, 1]

	/*
	 * Deliver sync Factory
	 */
	virtual ISyncFactory * getSyncFactory ();

	/**
	 * @brief Returns the system specific debug channel
	 */
	virtual IDebugChannel * getDebugChannel ();

	/**
	 * @brief	Update the graphical representation of if the daemon where
	 * 			applicable.
	 *
	 * @param iconName	Name of the new graphical representation
	 */
	virtual void setIcon(const std::string& iconName);

	void run ();
	void stop ();
        void end ();


};

#endif /* SYSTEMBOOST_H_ */

