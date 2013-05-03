#include "memAppConnector.h"

#include "systemBoost.h"
#include "nena.h"
#include "messages.h"
#include "messageBuffer.h"
#include "debug.h"
#include "msg.h"
#include "enhancedAppConnector.h"

#include <boost/lexical_cast.hpp>

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/detail/move.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using boost::interprocess::managed_shared_memory;
using boost::interprocess::remove_shared_memory_on_destroy;
using boost::interprocess::named_semaphore;
using boost::interprocess::named_mutex;
using boost::interprocess::interprocess_mutex;
using boost::interprocess::interprocess_semaphore;
using boost::interprocess::open_or_create;
using boost::interprocess::open_only;
using boost::interprocess::allocator;
using boost::interprocess::scoped_lock;

using boost::posix_time::microsec_clock;

using boost::shared_ptr;
using boost::scoped_ptr;
using boost::unique_lock;
using boost::lock_guard;
using boost::mutex;

static const std::string & connectorName = "appConnector://boost/sharedMemory";
static const std::string & serverName = "appServer://boost/sharedMemory";

CMemAppConnector::CMemAppConnector (CNena * nodearch, IMessageScheduler * sched, int index, IAppServer * server):
IEnhancedAppConnector (sched, nodearch, server),
myIndex(index),
frun(true)
{
	className += "::CMemAppConnector";
	DBG_DEBUG(boost::format("CMemAppConnector %1%: Constructor.") % index);

	/// for safety, delete old shared memory if it still persists
	boost::interprocess::shared_memory_object::remove((boost::format("nodearch-com-%1%") % index).str().c_str());
	/// set up shared memory
	shm.reset (new managed_shared_memory(open_or_create, (boost::format("nodearch-com-%1%")%index).str().c_str(), 20 MB));
	shm_guard.reset (new remove_shared_memory_on_destroy((boost::format("nodearch-com-%1%")%index).str().c_str()));

	/// semaphores for notifying
	sem_recv = shm->construct<interprocess_semaphore> ("recv-sem") (0);
	sem_send = shm->construct<interprocess_semaphore> ("send-sem") (0);
	/// semmaphore safety checks
	assert(!sem_recv->try_wait());
	assert(!sem_send->try_wait());

	/// create the outgoing queue, for messages from the app to the net
	sendQueue = shm->construct<shmStringVectorList> ("send-q")(shm->get_segment_manager());
	mt_send = shm->construct<interprocess_mutex>("send-q-mutex")();
	/// and incoming queue, for messages from the net to the app
	recvQueue = shm->construct<shmStringVectorList> ("recv-q")(shm->get_segment_manager());
	mt_recv = shm->construct<interprocess_mutex>("recv-q-mutex")();

	/// queue safety checks
	assert (sendQueue->empty());
	assert (recvQueue->empty());

	/// start recv thread
	worker_threads.create_thread (boost::bind(&CMemAppConnector::recv_fkt, this));

	/// and notify the app, that all is prepared
	DBG_INFO(boost::format("CMemAppConnector %1%: opening nodearch-reply-sem-%1%.") % myIndex);
	sem_reply.reset (new named_semaphore(open_only, (boost::format("nodearch-reply-sem-%1%") % myIndex).str().c_str()));
	sem_reply->post ();
}

CMemAppConnector::~CMemAppConnector ()
{
	DBG_DEBUG (boost::format("CMemAppConnector %1%: destructor.") % myIndex);

	named_semaphore::remove ((boost::format("nodearch-reply-sem-%1%") % myIndex).str().c_str());

	if (frun)
		stop ();

	worker_threads.interrupt_all();
	worker_threads.join_all();

	/*shm->destroy<shmStringVectorList>("send-q");
	shm->destroy<shmStringVectorList>("recv-q");
	shm->destroy<interprocess_mutex>("send-q-mutex");
	shm->destroy<interprocess_mutex>("recv-q-mutex");*/
}

void CMemAppConnector::recv_fkt ()
{
	DBG_INFO (boost::format("%1% %2%: receiving...") % className % myIndex);
	boost::posix_time::ptime timeout;
	bool ret = false;

	while (true)
	{
		{
			/// wait for a global "go"
			unique_lock<mutex> rlock (run_mutex);

			while (!frun)
			{
				/// this is also an interrupt point
				try
				{
					DBG_INFO(boost::format("%1% %2%: stopped...") % className % myIndex);
					run_cond.wait (rlock);
					DBG_INFO(boost::format("%1% %2%: runs...") % className % myIndex);
				}
				catch (boost::thread_interrupted & inter)
				{
					return;
				}
			}
		}

		try
		{
			do
			{
				timeout = microsec_clock::universal_time();
				timeout += boost::posix_time::seconds (1);
			}
			while (frun && !(ret = sem_send->timed_wait (timeout)));
		}
		catch (...)
		{
			return;
		}

		if (!frun && !ret)
			continue;

		if (sendQueue->empty())
		{
			DBG_WARNING (boost::format("CMemAppConnector %1%: received no message, but got posted!") % myIndex);
			continue;
		}

		/// get payload and metadata from vector from queue
		scoped_lock<interprocess_mutex> sl(*mt_send);
		if (sendQueue->front().size() == 2)
		{
			shared_buffer_t payload;

			// copy data from shared memory to process exclusive memory
			if (sendQueue->front()[1].size() != 0)
				payload = shared_buffer_t(reinterpret_cast<char*>(&sendQueue->front()[1].at(0)),
						sendQueue->front()[1].size());

			buffer_t codestring(sendQueue->front()[0].size(), &sendQueue->front()[0].at(0));

			MSG_TYPE msgtype;
			for (uint32_t i=0; i < 4; i++)
				reinterpret_cast<uint8_t *> (&msgtype)[i] = codestring[i];

			//DBG_DEBUG (boost::format("CMemAppConnector %1%: msg %2% with payload %3%.") % myIndex % msgtype % payload.getBuffer());
			handlePayload (msgtype, payload);
		}
		else
			DBG_WARNING (boost::format("CMemAppConnector %1%: received misformed message!") % myIndex);

		sendQueue->pop_front ();
	}
}

void CMemAppConnector::start ()
{
	DBG_INFO (boost::format("%1%: starting worker threads!") % className);
	{
		lock_guard<mutex> rlock (run_mutex);
		frun = true;
	}
	run_cond.notify_all ();
}

void CMemAppConnector::stop ()
{
	DBG_INFO (boost::format("%1%: stopping worker threads!") % className);
	lock_guard<mutex> rlock (run_mutex);
	frun = false;
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * Nothing implemented yet.
 *
 * @param msg	Pointer to message
 */
void CMemAppConnector::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * Nothing implemented yet.
 *
 * @param msg	Pointer to message
 */
void CMemAppConnector::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

void CMemAppConnector::sendPayload (MSG_TYPE type, shared_buffer_t payload)
{
	/// copy buffer to string in shared memory
	shmString spayload(payload.data(), payload.size(), shm->get_segment_manager());
	shmString meta(reinterpret_cast<uint8_t *> (&type), sizeof(MSG_TYPE), shm->get_segment_manager());

	/// create vector [metadata, payload]
	shmStringVector vec(shm->get_segment_manager());
	vec.push_back (boost::interprocess::move(meta));
	vec.push_back (boost::interprocess::move(spayload));

	/// push vector to queue
	{
		scoped_lock<interprocess_mutex> sl (*mt_recv);
		//recvQueue->push_back (boost::interprocess::move(vec));
		recvQueue->push_back (vec);
	}

	/// notify app
	sem_recv->post ();

	return;
}

const std::string & CMemAppConnector::getId () const
{
	return connectorName;
}

/* ========================================================================= */

CAppMemServer::CAppMemServer (CNena * nodearch, IMessageScheduler * sched):
IEnhancedAppServer (sched),
nodearch (nodearch),
frun(false),
local_index(0)
{
	className += "::CAppMemServer";

	DBG_INFO(boost::format("%1%: Creating shared memory.") % className);

	/// for safety, delete old shared memory if it still persists
	boost::interprocess::shared_memory_object::remove("nodearch-announce");
	/// set up shared memory
	shm.reset (new managed_shared_memory(open_or_create, "nodearch-announce", 1 KB));
	shm_guard.reset (new remove_shared_memory_on_destroy("nodearch-announce"));

	named_semaphore::remove ("nodearch-announce-sem");
	sem.reset (new named_semaphore(open_or_create, "nodearch-announce-sem", 0));

	/// make sure that the semaphore is clean
	assert (!sem->try_wait());

	/// create the index variable
	global_index = shm->find_or_construct<int>("nodearch-index")(0);
	/// check global index
	assert(*global_index == 0);
	/// and its guard
	named_mutex::remove ("nodearch-index-guard");
	index_guard.reset (new named_mutex (open_or_create, "nodearch-index-guard"));

	/// start the listening thread
	worker_threads.create_thread (boost::bind(&CAppMemServer::listen_fkt, this));

	DBG_INFO(boost::format("%1%: Listen Thread launched.") % className);
}

CAppMemServer::~CAppMemServer ()
{
	named_semaphore::remove ("nodearch-announce-sem");
	named_mutex::remove ("nodearch-index-guard");

	worker_threads.interrupt_all();
	worker_threads.join_all();
}

void CAppMemServer::listen_fkt ()
{
	DBG_INFO(boost::format("%1%: Listen Thread alive!") % className);

	while (true)
	{
		{
			/// wait for a global "go"
			unique_lock<mutex> rlock (run_mutex);

			while (!frun)
			{
				/// this is also an interrupt point
				try
				{
					DBG_INFO(boost::format("%1%: stopped...") % className);
					run_cond.wait (rlock);
				}
				catch (boost::thread_interrupted & inter)
				{
					return;
				}
			}
		}

		DBG_INFO(boost::format("%1%: waiting for announce...") % className);

		boost::posix_time::ptime t;
		bool ret = false;

		do
		{
			try
			{
				boost::this_thread::interruption_point();
			}
			catch (boost::thread_interrupted & inter)
			{
				return;
			}

			t = microsec_clock::universal_time() + boost::posix_time::seconds (1);
		}
		while (!(ret = sem->timed_wait (t)) && frun);

		/// put thread to sleep when no connection attemp is pending and the nodearch requested so
		if (!frun && !ret)
			continue;

		if (local_index <= *global_index)
		{
			DBG_DEBUG (boost::format("%1%: spawning new connection with index %2%") % className % local_index);
			addNew (shared_ptr<IEnhancedAppConnector> (new CMemAppConnector(nodearch, scheduler, local_index++, this)));
		}
		else
		{
			DBG_WARNING(boost::format("%1%: index variable out of sync!") % className);
			throw ESync ((boost::format("%1%: index variable out of sync!") % className).str());
		}

	} // main loop
}

void CAppMemServer::start ()
{
	{
		lock_guard<mutex> rlock (run_mutex);
		frun = true;
	}
	run_cond.notify_all ();

	DBG_DEBUG(boost::format("%1%: runs...") % className);
}

void CAppMemServer::stop ()
{
	lock_guard<mutex> rlock (run_mutex);
	frun = false;

	DBG_DEBUG(boost::format("%1%: stop.") % className);
}

void CAppMemServer::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

void CAppMemServer::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

void CAppMemServer::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

void CAppMemServer::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

const std::string & CAppMemServer::getId () const
{
	return serverName;
}
