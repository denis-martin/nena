/** @file
 * boostScheduler.h
 *
 * @brief Defines a concrete Scheduler for the Boost environment
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Aug 29, 2009
 *      Author: Benjamin Behringer
 */

#ifndef BOOSTSCHEDULERMT_H_
#define BOOSTSCHEDULERMT_H_


#include "messages.h"
#include "nena.h"

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <queue>
#include <map>
#include <vector>
#include <fstream>

// log messages to log file(s)
//#define NENA_MESSAGE_TRACE

/**
 * @brief Message Queue for use in multithreaded environments
 */
class CSyncMessageQueue : public IMessageQueue
{
	private:
	std::queue<boost::shared_ptr<IMessage> > q;
	boost::mutex m;

	public:
	CSyncMessageQueue () {}
	virtual ~CSyncMessageQueue ();
	CSyncMessageQueue (CSyncMessageQueue & rhs);
	
	virtual boost::shared_ptr<IMessage> pop ();
	virtual void push (boost::shared_ptr<IMessage>);
	
	virtual bool empty () const;
	virtual size_t size () const;

	CSyncMessageQueue & operator= (CSyncMessageQueue & rhs);

	friend bool operator== (CSyncMessageQueue &, CSyncMessageQueue &);
	friend bool operator!= (CSyncMessageQueue &, CSyncMessageQueue &);
};

bool operator== (CSyncMessageQueue &, CSyncMessageQueue &);
bool operator!= (CSyncMessageQueue &, CSyncMessageQueue &);

/*****************************************************************************/

class CBoostSchedulerMT : public IMessageScheduler
{
public:
	/**
	 * @brief	Sync error
	 */
	class ESync : public std::exception
	{
	protected:
		std::string msg;

	public:
		ESync() throw () : msg(std::string()) {};
		ESync(const std::string& msg) throw () : msg(msg){};
		virtual ~ESync() throw () {};

		virtual const char* what() const throw () { return msg.c_str(); };
	};

private:
	CNena* nena;

#ifdef NENA_MESSAGE_TRACE
	std::ofstream messageLogSend;
	std::ofstream messageLogRecv;
#endif // NENA_MESSAGE_TRACE

	boost::shared_ptr<boost::asio::io_service> io_service;

	/// Worker Thread Group
	boost::thread_group worker_threads;

	/// semaphore workaround for message count
	int msg_count;
	boost::mutex msg_mutex;
	boost::condition_variable msg_cond;

	/// condition variable for run/stop
	bool frun;
	boost::mutex run_mutex;
	boost::condition_variable run_cond;

	/// protects the queues
	boost::shared_mutex queue_mutex;

	/// round robin pointers to queue
	std::vector<std::map<IMessageProcessor *, boost::shared_ptr<IMessageQueue> >::iterator> rr_iterators;

	/// workaround for not threadsafe Message Processors
	boost::shared_mutex mp_mutex;	/// secures mutex map
	std::map<IMessageProcessor *, boost::shared_ptr<boost::mutex> > mp_mutex_map;	/// holds mutex for every non-mt Message Processor

	/// map for all running timers, for cleanup mostly
	std::map<boost::asio::deadline_timer *, boost::shared_ptr<CTimer> > timerMap;
	std::map<boost::shared_ptr<CTimer>, boost::asio::deadline_timer *> reverseTimerMap;
	boost::mutex timerMapMutex;

	/// "cache" for message passing between schedulers
	boost::shared_mutex dest_cache_mutex;
	std::map<IMessageProcessor *, IMessageScheduler * > dest_cache;
	boost::shared_mutex src_cache_mutex;
	std::map<IMessageProcessor *, IMessageScheduler * > src_cache;

	/// queue for new message processors waiting to be registered
	boost::shared_mutex registerMutex;
	std::map<IMessageProcessor*, boost::shared_ptr<IMessageQueue> > registerQueue;

	/// queue for message processors waiting to be unregistered
	boost::shared_mutex unregisterMutex;
	std::list<IMessageProcessor *> unregisterQueue;


	/// handles timer events	
	void handle_timer (boost::asio::deadline_timer * t);

	/**
	 * @brief processes all messages
	 */
	void worker_fkt (uint32_t index) throw (ESync);

	/// check if there are new or outdated message processors
//	void checkMsgProcs ();

#ifdef NENA_MESSAGE_TRACE
	inline void logMessage(std::ofstream& stream, boost::shared_ptr<IMessage>& msg)
	{
		std::string from(msg->getFrom()->getId());
		std::string to(msg->getTo()->getId());

		if (from.empty())
			from = msg->getFrom()->getClassName();
		if (to.empty())
			to = msg->getTo()->getClassName();

		stream << nena->getSysTime() << " " << from << " " << to << " ";
		switch (msg->getType()) {
		case IMessage::t_outgoing: stream << "t_outgoing "; break;
		case IMessage::t_incoming: stream << "t_incoming "; break;
		case IMessage::t_timer: stream << "t_timer "; break;
		case IMessage::t_event: stream << "t_event "; break;
		case IMessage::t_message: stream << "t_message "; break;
		default: stream << "unknown "; break;
		}

		if (msg->getType() == IMessage::t_event)
			stream << msg->cast<IEvent>()->getId() << std::endl;
		else
			stream << msg->getClassName() << std::endl;
	}
#endif // NENA_MESSAGE_TRACE

public:
	CBoostSchedulerMT (CNena * na, boost::shared_ptr<boost::asio::io_service> ios, int nthreads);
	CBoostSchedulerMT (CNena * na, boost::shared_ptr<boost::asio::io_service> ios, int nthreads, std::string name);
	virtual ~CBoostSchedulerMT ();
	
	/**
	 * @brief triggers Scheduler to process messages
	 *
	 * As we are not multithreaded yet, just do nothing.
	 *
	 */
	virtual void run ();

	/**
	 * @brief stops Scheduler
	 *
	 * As we are not multithreaded yet, just do nothing.
	 *
	 */
	virtual void stop ();

	/**
	 * @brief Register a new message processor belonging to this scheduler
	 */
	virtual void registerMessageProcessor(IMessageProcessor *proc) throw (EUnknowMessageProcessor, EAlreadyRegistered);

	/**
	 * @brief Unregister a new message processor belonging to this scheduler
	 */
	virtual void unregisterMessageProcessor(IMessageProcessor *proc) throw (EUnknowMessageProcessor);

	/**
	 * @brief Set a single shot time
	 *
	 * @param timer	Timer to be set
	 */
	virtual void setTimer(boost::shared_ptr<CTimer>& timer);

	/**
	 * @brief Cancel a timer
	 *
	 * @param timer Timer to cancel
	 */
	virtual void cancelTimer(boost::shared_ptr<CTimer>& timer);

	/**
	 * @brief 	Send a message to another processing unit
	 *
	 * @param msg	Message to send
	 */
	virtual void sendMessage(boost::shared_ptr<IMessage>& msg) throw (EUnknowMessageProcessor, EMessageLoop);

	/**
	 * @brief Process all waiting messages and call respective receivers.
	 * Returns if all queues are empty.
	 */
	virtual void processMessages() throw (EUnknowMessageProcessor, EBadCall);

	/**
	 * @brief Test if scheduler handles messages for a MessageProcessor
	 *
	 * @param	proc	MessageProcessor to look up
	 *
	 * @return True if this scheduler knows the MessageProcessor, false otherwise
	 */
	virtual bool hasMessageProcessor (IMessageProcessor * proc);

	/**
	 * @brief Takes over message from other message scheduler, which has no corresponding message processor
	 */
	virtual void passMessage (boost::shared_ptr<IMessage> msg);

	virtual const std::string & getId () const;
};

#endif // BOOSTSCHEDULER_H_

