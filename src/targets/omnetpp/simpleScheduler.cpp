#include "simpleScheduler.h"

#include "messages.h"
#include "debug.h"

#include <list>
#include <map>
#include <exception>
#include <string>

using boost::shared_ptr;

CMessageQueue::~CMessageQueue ()
{
	// remaining messages are auto-deleted due to shared_ptr
}

CMessageQueue::CMessageQueue (CMessageQueue & rhs)
{

	q = rhs.q;
}

CMessageQueue & CMessageQueue::operator= (CMessageQueue & rhs)
{
	if (this == &rhs)
		return *this;

	q = rhs.q;

	return *this;
}

boost::shared_ptr<IMessage> CMessageQueue::pop ()
{
	shared_ptr<IMessage> ret = q.front();
	q.pop();
	return ret;
}

void CMessageQueue::push (boost::shared_ptr<IMessage> msg)
{
	q.push (msg);
}

bool CMessageQueue::empty () const
{
	return q.empty ();
}

size_t CMessageQueue::size () const
{
	return q.size ();
}

bool operator== (CMessageQueue & lhs, CMessageQueue & rhs)
{
	return lhs.q == rhs.q;
}

bool operator!= (CMessageQueue & lhs, CMessageQueue & rhs)
{
	return !(lhs == rhs);
}

/*****************************************************************************/

CSimpleScheduler::CSimpleScheduler (IMessageScheduler *parent):
	IMessageScheduler(parent)
{
	name += "::CSimpleScheduler";
}

CSimpleScheduler::~CSimpleScheduler ()
{
}

/**
 * @brief triggers Scheduler to process messages
 *
 * As we are not multithreaded yet, just do nothing.
 *
 */
void CSimpleScheduler::run ()
{
	DBG_DEBUG(FMT("%1% runs...") % name);
}

/**
 * @brief stops Scheduler
 *
 * As we are not multithreaded yet, just do nothing.
 *
 */
void CSimpleScheduler::stop ()
{
	DBG_DEBUG(FMT("%1% stop.") % name);
}

/**
 * @brief Register a new message processor belonging to this scheduler
 */
void CSimpleScheduler::registerMessageProcessor(IMessageProcessor *proc) throw (EUnknowMessageProcessor, EAlreadyRegistered)
{
	if (proc == NULL) throw EUnknowMessageProcessor();

	std::map<IMessageProcessor *, shared_ptr<IMessageQueue> >::iterator it;
	it = queues.find(proc);
	if (it != queues.end()) throw EAlreadyRegistered();

	queues[proc] = shared_ptr<IMessageQueue>(new CMessageQueue());
};

/**
 * @brief Unregister a new message processor belonging to this scheduler
 */
void CSimpleScheduler::unregisterMessageProcessor(IMessageProcessor *proc) throw (EUnknowMessageProcessor)
{
	if (proc == NULL) throw EUnknowMessageProcessor();

	std::map<IMessageProcessor *, shared_ptr<IMessageQueue> >::iterator it;
	it = queues.find(proc);
	if (it == queues.end()) throw EUnknowMessageProcessor();
	queues.erase(it);
};

/**
 * @brief 	Send a message to another processing unit and start the processing
 *
 * @param msg	Message to send
 */
void CSimpleScheduler::sendMessage(boost::shared_ptr<IMessage>& msg) throw (EUnknowMessageProcessor, EMessageLoop)
{
	assert(msg != NULL);
	if (msg->getFrom() == NULL)
		throw EUnknowMessageProcessor("from = NULL");
	if (msg->getTo() == NULL)
		throw EUnknowMessageProcessor("to = NULL");

	std::map<IMessageProcessor *, shared_ptr<IMessageQueue> >::iterator it;

	IMessageProcessor *src = msg->getFrom();


	if (queues.find(src) == queues.end()) throw EUnknowMessageProcessor(src->getClassName().c_str());

	IMessageProcessor *dest = msg->getTo();
	it = queues.find(dest);
	if (it == queues.end()) throw EUnknowMessageProcessor(dest->getClassName().c_str());

	it->second->push (msg); // enqueue message
}

void CSimpleScheduler::processMessages() throw (EUnknowMessageProcessor)
{
	std::map<IMessageProcessor *, shared_ptr<IMessageQueue> >::iterator it_mp;

	bool allQueuesEmpty = false;

	// basic round-robin
	while (!allQueuesEmpty)
	{
		allQueuesEmpty = true; // think positive!

		for (it_mp = queues.begin(); it_mp != queues.end(); it_mp++)
		{

			if (it_mp->second->size() > 0)
			{

				shared_ptr<IMessage> msg = it_mp->second->pop();
				if (msg->getTo() != it_mp->first)
					DBG_FAIL("This should not happen...");

				try
				{
					// this may result in new messages, so we need to reiterate
					if (allQueuesEmpty) allQueuesEmpty = false;

					msg->getTo()->processMessage(msg);


				}
				catch (IMessageProcessor::EUnhandledMessage e)
				{
					DBG_INFO(FMT("CSimpleMsgScheduler: Unhandled message exception (proc %1%, msg %2%, what \"%3%\")") %
						msg->getTo()->getClassName() %
						msg->getClassName() %
						e.what());

				}
			}

		} // for

	} // while
}





