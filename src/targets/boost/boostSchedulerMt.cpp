#include "boostSchedulerMt.h"
#include "systemBoost.h"
#include "messages.h"
#include "debug.h"

#include "nena.h"

#include <boost/bind.hpp>

#include <list>
#include <exception>
#include <string>

#define NENA_MESSAGE_TRACE_FILE_SEND "nena-messages-send.log"
#define NENA_MESSAGE_TRACE_FILE_RECV "nena-messages-recv.log"

using boost::asio::deadline_timer;
using boost::mutex;
using boost::shared_mutex;
using boost::shared_lock;
using boost::lock_guard;
using boost::unique_lock;
using boost::condition_variable;
using boost::upgrade_lock;
using boost::upgrade_to_unique_lock;
using boost::shared_ptr;

using namespace std;

static const std::string & schedulerName = "scheduler://boost/multithreaded";

CSyncMessageQueue::~CSyncMessageQueue ()
{
	lock_guard<mutex> lock (m);

	// delete all remaining messages
	while (!q.empty ()) {
		q.pop ();
	}
}

CSyncMessageQueue::CSyncMessageQueue (CSyncMessageQueue & rhs)
{
	lock_guard<mutex> lock (m);
	q = rhs.q;
}

CSyncMessageQueue & CSyncMessageQueue::operator= (CSyncMessageQueue & rhs)
{
	if (this == &rhs)
		return *this;

	lock_guard<mutex> lock1 (m);
	lock_guard<mutex> lock2 (rhs.m);
	q = rhs.q;

	return *this;
}

shared_ptr<IMessage> CSyncMessageQueue::pop ()
{
	shared_ptr<IMessage> ret;

	lock_guard<mutex> lock (m);
	if (q.empty ())
		return ret;

	ret = q.front ();
	q.pop ();

	return ret;
}

void CSyncMessageQueue::push (shared_ptr<IMessage> msg)
{
	lock_guard<mutex> lock (m);
	q.push (msg);
}

bool CSyncMessageQueue::empty () const
{
	return q.empty ();
}

size_t CSyncMessageQueue::size () const
{
	return q.size ();
}

bool operator== (CSyncMessageQueue & lhs, CSyncMessageQueue & rhs)
{
	lock_guard<mutex> lock1 (lhs.m);
	lock_guard<mutex> lock2 (rhs.m);

	return lhs.q == rhs.q;
}

bool operator!= (CSyncMessageQueue & lhs, CSyncMessageQueue & rhs)
{
	return !(lhs == rhs);
}

/*****************************************************************************/

CBoostSchedulerMT::CBoostSchedulerMT (CNena * na, boost::shared_ptr<boost::asio::io_service> ios, int nthreads) :
	IMessageScheduler(NULL),
	nena (na),
	io_service(ios),
	msg_count(0),
	frun(false)
{
	name += "::CBoostSchedulerMT";

#ifdef NENA_MESSAGE_TRACE
	messageLogSend.open(NENA_MESSAGE_TRACE_FILE_SEND);
	messageLogRecv.open(NENA_MESSAGE_TRACE_FILE_RECV);
#endif

	for (int i=0; i < nthreads; i++)
	{
		worker_threads.create_thread (boost::bind(&CBoostSchedulerMT::worker_fkt, this, i));
		rr_iterators.push_back (queues.begin ());
	}

	DBG_DEBUG(FMT("%1% starting with %2% worker threads.") % getId() % nthreads);
}

CBoostSchedulerMT::CBoostSchedulerMT (CNena * na, boost::shared_ptr<boost::asio::io_service> ios, int nthreads, string name):
	IMessageScheduler(NULL, name),
	nena (na),
	io_service(ios),
	msg_count(0),
	frun(false)
{
	this->name += "::CBoostSchedulerMT";

	for (int i=0; i < nthreads; i++)
	{
		worker_threads.create_thread (boost::bind(&CBoostSchedulerMT::worker_fkt, this, i));
		rr_iterators.push_back (queues.begin ());
	}

	DBG_DEBUG(FMT("%1% starting with %2% worker threads.") % getId() % nthreads);
}

CBoostSchedulerMT::~CBoostSchedulerMT ()
{
	worker_threads.interrupt_all();
	worker_threads.join_all();

	{
		lock_guard<mutex> lg(timerMapMutex);
		map<deadline_timer *, shared_ptr<CTimer> >::iterator it;

		for (it = timerMap.begin (); it != timerMap.end (); it++)
		{
			it->first->cancel();

			DBG_INFO(FMT("%1%: killing timerMap entry %2% with timer %3%.") % getId() % it->first % it->second);

			delete it->first;
		}
		timerMap.clear ();
	}

	DBG_DEBUG(FMT("%1% destroyed") % getId());
}

/**
 * @brief processes all messages
 */
void CBoostSchedulerMT::worker_fkt (uint32_t index) throw (ESync)
{
	shared_ptr<IMessage> msg;

	DBG_INFO(boost::format("Worker Thread %1% alive!") % index);

	while (true)
	{
		/// wait for a global "go"
		{
			unique_lock<mutex> rlock (run_mutex);

			while (!frun)
			{
				/// this is also an interrupt point
				try
				{
					run_cond.wait (rlock);
					DBG_INFO(boost::format("Worker Thread %1% ready to work!") % index);
				}
				catch (boost::thread_interrupted & inter)
				{
					return;
				}
			}
		}

		{
			unique_lock<mutex> mlock (msg_mutex);

			/// this is also an interrupt point
			try
			{
				while (msg_count == 0)
					msg_cond.wait (mlock);
			}
			catch (boost::thread_interrupted & inter)
			{
				return;
			}

			msg_count--;
		}

		/// find an unprocessed message, through all queues (round robin like)
		msg.reset();

		/// did we add or delete a message processor?
		bool procWork = false;
		
		/// delete message processors
		{
			unique_lock<shared_mutex> reglock(unregisterMutex);
			if (!unregisterQueue.empty())
			{
				IMessageProcessor * proc = unregisterQueue.front();
				unregisterQueue.pop_front();
				
				/// acquire unique access to the queues
				unique_lock<shared_mutex> uniqueLock (queue_mutex);
				std::map<IMessageProcessor *, shared_ptr<IMessageQueue> >::iterator it;
				it = queues.find(proc);
				if (it == queues.end()) throw EUnknowMessageProcessor(
						str(FMT("%1%: cannot unregister unknown message processor %2% (CBoostSchedulerMT::worker_fkt)") % getId() % proc->getId()));
				queues.erase(it);

				/// reset round robin pointers, as they might be invalid now
				std::vector<std::map<IMessageProcessor *, shared_ptr<IMessageQueue> >::iterator>::iterator vit;

				for (vit = rr_iterators.begin (); vit != rr_iterators.end (); vit++)
					*vit = queues.begin ();
				
				procWork = true;
			}
		}
		
		if (!procWork)
		/// make sure we now have all current message processors
		{
			unique_lock<shared_mutex> reglock(registerMutex);
			if (!registerQueue.empty())
			{
				// acquire unique access to the queues
				unique_lock<shared_mutex> uniqueLock (queue_mutex);

				map<IMessageProcessor*, shared_ptr<IMessageQueue> >::const_iterator rit;
				for (rit = registerQueue.begin(); rit != registerQueue.end(); rit++)
					queues[rit->first] = rit->second;
				
				if (registerQueue.size() > 1) {
					DBG_INFO(FMT("%1%: bulk registered %2% message processors") % getId() % registerQueue.size());
					msg_count -= registerQueue.size()-1;
					assert(msg_count >= 0);

				}

				registerQueue.clear();

				// reset round robin pointers, as they might be invalid now
				std::vector<std::map<IMessageProcessor *, shared_ptr<IMessageQueue> >::iterator>::iterator vit;

				for (vit = rr_iterators.begin (); vit != rr_iterators.end (); vit++)
					*vit = queues.begin ();
				
				procWork = true;
			}
		}
		
		if (!procWork)
		/// need shared access to queues
		{
			shared_lock<shared_mutex> qlock (queue_mutex);

			/// round robin scheduling: start searching for new messages at next MessageProc
			//for (it_mp = queues.begin(); it_mp != queues.end(); it_mp++)

			//uint32_t old_index = index;
			do
			{
				// increment through queues from last point on

				if (rr_iterators[index] == --queues.end())
					rr_iterators[index] = queues.begin ();
				else
					rr_iterators[index]++;

				if (!rr_iterators[index]->second->empty())
				{
					msg = rr_iterators[index]->second->pop();

					if (msg == NULL)
					{
						DBG_ERROR(boost::format("Worker Thread %1% popped NULL message!") % index);
						continue;
					}

					if (msg->getTo() != rr_iterators[index]->first)
						DBG_FAIL("This should not happen...");

#ifdef NENA_MESSAGE_TRACE
					logMessage(messageLogRecv, msg);
#endif // NENA_MESSAGE_TRACE

					try
					{
						if (msg->getTo()->isThreadsafe ())
						{
							msg->getTo()->processMessage(msg);
						}
						else
						{
							map<IMessageProcessor *, shared_ptr<mutex> >::iterator mm_it;


							unique_lock<shared_mutex> uniqueLock(mp_mutex);

							if (mp_mutex_map.end () == mp_mutex_map.find(msg->getTo()))
								mp_mutex_map[msg->getTo()] = shared_ptr<mutex> (new mutex());


							/// only lock the mutex of the Message Processor
							lock_guard<mutex> mp_guard(*mp_mutex_map[msg->getTo()]);
							msg->getTo()->processMessage(msg);
						}
					}
					catch (IMessageProcessor::EUnhandledMessage& e)
					{
						if (msg->getFlowState().get() != NULL &&
							msg->getFlowState()->getOperationalState() == CFlowState::s_stale)
						{
							DBG_WARNING(FMT("%1%: Unhandled message exception (stale flow state, what \"%2%\")") %
								getId() % e.what());

						} else if (msg->getType() == IMessage::t_event) {
							boost::shared_ptr<IEvent> event = msg->cast<IEvent>();
							DBG_WARNING(FMT("%1%: Unhandled event exception (proc %2%, event %3% from %4%, what \"%5%\")") %
								getId() %
								event->getTo()->getId() %
								event->getId() %
								event->getFrom()->getId() %
								e.what());

						} else {
							DBG_WARNING(FMT("%1%: Unhandled message exception (proc %2%, msg %3% from %4%, what \"%5%\")") %
								getId() %
								msg->getTo()->getId() %
								msg->getClassName() %
								msg->getFrom()->getId() %
								e.what());

						}

					}

					break;
				} // queue not empty
			} // check all queues
			while (true); //(old_index = index);
		} // local scope
	} // main loop
}

/**
 * @brief triggers Scheduler to process messages
 *
 * Notify all worker threads to run.
 *
 */
void CBoostSchedulerMT::run ()
{
	lock_guard<mutex> rlock (run_mutex);
	frun = true;
	run_cond.notify_all ();
	//io_thread = worker_threads.create_thread (boost::bind(&CBoostSchedulerMT::io_run, this));

	DBG_DEBUG(FMT("%1% runs...") % getId());
}

/**
 * @brief stops Scheduler
 *
 * Set condition variable, which stops all worker threads.
 *
 */
void CBoostSchedulerMT::stop ()
{
//	DBG_DEBUG(FMT("%1%: stopping") % getId());

	lock_guard<mutex> rlock (run_mutex);
	frun = false;

	//worker_threads.remove_thread(io_thread);
	DBG_DEBUG(FMT("%1%: stopped") % getId());
}

/**
 * @brief Register a new message processor belonging to this scheduler
 */
void CBoostSchedulerMT::registerMessageProcessor(IMessageProcessor *proc) throw (EUnknowMessageProcessor, EAlreadyRegistered)
{
	if (proc == NULL) throw EUnknowMessageProcessor("processor == NULL");

	bool found = false;

	{
		shared_lock<shared_mutex> uplock(queue_mutex);
		found = queues.find(proc) != queues.end();
	}

	if (found) {
		// check whether it is already in the unregister queue
		shared_lock<shared_mutex> reglock(unregisterMutex);
		std::list<IMessageProcessor *>::iterator it;
		for (it = unregisterQueue.begin(); it != unregisterQueue.end(); it++) {
			if (*it == proc) {
				found = false;
				break;
			}
		}

	}

	if (found)
		throw EAlreadyRegistered("message processor already registered");

	// not found
	{
//		DBG_DEBUG(FMT("%1%: registering %2%(%3%)") % getId() % proc->getId() % proc->getClassName());

		shared_ptr<CSyncMessageQueue> mq(new CSyncMessageQueue());

		unique_lock<shared_mutex> reglock(registerMutex);
		if (registerQueue.find(proc) != registerQueue.end())
			throw EAlreadyRegistered("message processor already in register queue");

		registerQueue[proc] = mq;
	}

	/// notify worker threads
	lock_guard<mutex> mlock(msg_mutex);
	msg_count++;
	msg_cond.notify_one();
};

/**
 * @brief Unregister a new message processor belonging to this scheduler
 */
void CBoostSchedulerMT::unregisterMessageProcessor(IMessageProcessor *proc) throw (EUnknowMessageProcessor)
{
	if (proc == NULL)
		throw EUnknowMessageProcessor("processor == NULL");

	{
		// check whether it is still in our register queue
		unique_lock<shared_mutex> reglock(registerMutex);
		map<IMessageProcessor*, boost::shared_ptr<IMessageQueue> >::iterator rqit = registerQueue.find(proc);
		if (rqit != registerQueue.end()) {
			// yes, erase it
			registerQueue.erase(rqit);
			return;
		}
	}

	{
		shared_lock<shared_mutex> uplock(queue_mutex);
		std::map<IMessageProcessor *, shared_ptr<IMessageQueue> >::iterator it;
		it = queues.find(proc);
		if (it == queues.end()) {
			throw EUnknowMessageProcessor(
				str(FMT("%1%: cannot unregister unknown message processor %2% (CBoostSchedulerMT::unregisterMessageProcessor())") % getId() % proc->getId()));

		}
	}
	
	unique_lock<shared_mutex> reglock(unregisterMutex);
	unregisterQueue.push_back(proc);

	/// notify worker threads
	lock_guard<mutex> mlock(msg_mutex);
	msg_count++;
	msg_cond.notify_one();
};

void CBoostSchedulerMT::handle_timer (deadline_timer * t)
{
	shared_ptr<CTimer> timer;
	{
		lock_guard<mutex> lg(timerMapMutex);
		map<deadline_timer *, shared_ptr<CTimer> >::iterator it = timerMap.find(t);
		if (it != timerMap.end()) {
			map<shared_ptr<CTimer>, deadline_timer *>::iterator tit = reverseTimerMap.find(it->second);
			if (tit != reverseTimerMap.end()) {
				reverseTimerMap.erase(tit);
			}

			timer = it->second;
			timerMap.erase(it);
		}

	}
	//DBG_INFO(FMT("%1%: erasing timerMap entry %2% with timer %3%.") % name % t % timer);
	shared_ptr<IMessage> msg = boost::dynamic_pointer_cast<IMessage>(timer);
	sendMessage(msg);
	delete t;
}

void CBoostSchedulerMT::setTimer(shared_ptr<CTimer>& timer)
{
	deadline_timer* t = new deadline_timer(*io_service);

	boost::posix_time::time_duration td = boost::posix_time::milliseconds ((long) (timer->timeout*1000));
	//DBG_INFO(FMT("%1%: adding timerMap entry %2% with timer %3%.") % name % t % timer);
	{
		lock_guard<mutex> lg(timerMapMutex);
		timerMap[t] = timer;
		reverseTimerMap[timer] = t;
	}
	t->expires_from_now (td);
	t->async_wait (boost::bind (&CBoostSchedulerMT::handle_timer, this, t));
}

void CBoostSchedulerMT::cancelTimer(shared_ptr<CTimer>& timer)
{
	deadline_timer * t;
	{
		lock_guard<mutex> lg(timerMapMutex);
		map<shared_ptr<CTimer>, deadline_timer *>::iterator tit = reverseTimerMap.find(timer);
		if (tit != reverseTimerMap.end()) {
			t = tit->second;
			reverseTimerMap.erase(tit);

			map<deadline_timer *, shared_ptr<CTimer> >::iterator it = timerMap.find(t);
			if (it != timerMap.end()) {
				timerMap.erase(it);
			}
		}
	}
	t->cancel();
	delete t;
}

/**
 * @brief 	Send a message to another processing unit and start the processing
 *
 * @param msg	Message to send
 */
void CBoostSchedulerMT::sendMessage(boost::shared_ptr<IMessage>& msg) throw (EUnknowMessageProcessor, EMessageLoop)
{
	assert(msg != NULL);
	if (msg->getFrom() == NULL)
		throw EUnknowMessageProcessor("from = NULL");
	if (msg->getTo() == NULL)
		throw EUnknowMessageProcessor("to = NULL");

	map<IMessageProcessor *, shared_ptr<IMessageQueue> >::iterator it;

	IMessageProcessor *src = msg->getFrom();

	/// we need shared access to the queues, but perhaps we already have (if
	shared_lock<shared_mutex> qlock (queue_mutex);

	if (queues.find(src) == queues.end()) {
		// check if it is in our register queue
		bool found = false;
		{
			shared_lock<shared_mutex> reglock(registerMutex);
			found = registerQueue.find(src) != registerQueue.end();
		}

		if (!found) {
			// src message processor not in our queues, look for others
			// happens in passMessage TODO: can we get rid of this?
			IMessageScheduler * ims = NULL;
			lock_guard<shared_mutex> lg(src_cache_mutex);
			if (src_cache.find(src) == src_cache.end ()) {
				ims = nena->lookupScheduler(src);
				if (ims == NULL) {
					string errstr(str(FMT("%1%: unknown src message processor %2% (CBoostSchedulerMT::sendMessage())") % getId() % src->getId()));
					throw EUnknowMessageProcessor(errstr);
				}
				else
					src_cache[src] = ims;
			}

		}
	}

	IMessageProcessor *dest = msg->getTo();
	it = queues.find(dest);
	if (it == queues.end()) {
		// check if it is in our register queue
		bool found = false;
		{
			unique_lock<shared_mutex> reglock(registerMutex);
			map<IMessageProcessor*, shared_ptr<IMessageQueue> >::iterator rit;
			rit = registerQueue.find(dest);
			if (rit != registerQueue.end()) {
				found = true;
				rit->second->push(msg);
				DBG_INFO(FMT("%1%: queueing message for message processor in register queue") % getId());

				// continue to end of function
			}
		}

		if (!found) {
			/// dest message processor not in our queues, look for others
			IMessageScheduler * ims = NULL;
			lock_guard<shared_mutex> lg(dest_cache_mutex);
			if (dest_cache.find(dest) == dest_cache.end ()) {
				ims = nena->lookupScheduler(dest);
				if (ims == NULL) {
					string errstr(str(FMT("%1%: unknown dest message processor %2% (CBoostSchedulerMT::sendMessage())") % getId() % dest->getId()));
					throw EUnknowMessageProcessor(errstr);
				} else {
					dest_cache[dest] = ims;
				}
			}

			dest_cache[dest]->passMessage(msg);
			return;
		}

	} else {
		it->second->push (msg); // enqueue message

	}

#ifdef NENA_MESSAGE_TRACE
	logMessage(messageLogSend, msg);
#endif // NENA_MESSAGE_TRACE

	/// notify worker threads
	lock_guard<mutex> mlock(msg_mutex);
	msg_count++;
	msg_cond.notify_one();
}

void CBoostSchedulerMT::processMessages() throw (EUnknowMessageProcessor, EBadCall)
{
	//throw EBadCall ("processMessages should not be called in MT version!");
}

bool CBoostSchedulerMT::hasMessageProcessor (IMessageProcessor * proc)
{
	bool found = false;

	{
		// we need shared access to the queues
		shared_lock<shared_mutex> qlock (queue_mutex);
		found = queues.find(proc) != queues.end();
	}

	if (!found) {
		// check if it is in our register queue
		shared_lock<shared_mutex> reglock(registerMutex);
		found = registerQueue.find(proc) != registerQueue.end();
	}

	return found;
}

void CBoostSchedulerMT::passMessage (boost::shared_ptr<IMessage> msg)
{
	if (!hasMessageProcessor(msg->getTo()))
		throw ENotResponsible ();
	else
		sendMessage (msg);
}

const std::string & CBoostSchedulerMT::getId () const
{
	return schedulerName;
}
