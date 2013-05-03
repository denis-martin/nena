/** @file
 * simpleMultiplexer.h
 *
 * @brief	Stub for a single threaded Scheduler
 *
 *  Created on: Jun 8, 2010
 *      Author: Benjamin Behringer
 */

#ifndef CSIMPLESCHEDULER
#define CSIMPLESCHEDULER

#include "messages.h"

#include <queue>

class CNena;

/**
 * @brief Message Queue for use in singlethreaded environments
 */
class CMessageQueue : public IMessageQueue
{
private:
	std::queue<boost::shared_ptr<IMessage> > q;

public:
	CMessageQueue () {}
	virtual ~CMessageQueue ();
	CMessageQueue (CMessageQueue & rhs);

	virtual boost::shared_ptr<IMessage> pop ();
	virtual void push (boost::shared_ptr<IMessage> msg);

	virtual bool empty () const;
	virtual size_t size () const;

	CMessageQueue & operator= (CMessageQueue & rhs);

	friend bool operator== (CMessageQueue &, CMessageQueue &);
	friend bool operator!= (CMessageQueue &, CMessageQueue &);
};

bool operator== (CMessageQueue &, CMessageQueue &);
bool operator!= (CMessageQueue &, CMessageQueue &);

/*****************************************************************************/

class CSimpleScheduler : public IMessageScheduler
{
public:

	CSimpleScheduler (IMessageScheduler *parent);
	virtual ~CSimpleScheduler ();

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
	virtual void setTimer(boost::shared_ptr<CTimer>& timer) = 0;

	/**
	 * @brief Cancel a timer
	 *
	 * @param timer Timer to cancel
	 */
	virtual void cancelTimer(boost::shared_ptr<CTimer>& timer) = 0;

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
	virtual void processMessages() throw (EUnknowMessageProcessor);
};

#endif
