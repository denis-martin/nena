#include "memNenai.h"

#include "../../src/targets/boost/msg.h"

#include <string>

#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/detail/move.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/list.hpp>

#include <boost/format.hpp>

using std::string;
using boost::function;

using boost::scoped_ptr;

using boost::interprocess::managed_shared_memory;
using boost::interprocess::named_semaphore;
using boost::interprocess::named_mutex;
using boost::interprocess::interprocess_mutex;
using boost::interprocess::interprocess_semaphore;
using boost::interprocess::open_or_create;
using boost::interprocess::open_only;
using boost::posix_time::microsec_clock;

using boost::mutex;
using boost::condition_variable;
using boost::lock_guard;
using boost::interprocess::scoped_lock;

MemNenai::MemNenai (string id, string t, function<void(string payload, bool endOfStream)> fkt)
{
	identifier = id;
	target = t;
	callback = fkt;
	frun = true;

	shm.reset (new managed_shared_memory(open_only, "nodearch-announce"));
	sem.reset (new named_semaphore(open_only, "nodearch-announce-sem"));
	index_guard.reset (new named_mutex (open_only, "nodearch-index-guard"));
	global_index = shm->find_or_construct<int>("nodearch-index")(0);

	/// announce us at the nodearch
	index_guard->lock ();
	myIndex = *global_index;
	*global_index += 1;
	index_guard->unlock ();
	sem->post ();

	named_semaphore::remove ((boost::format("nodearch-reply-sem-%1%") % myIndex).str().c_str());
	sem_reply.reset (new named_semaphore(open_or_create, (boost::format("nodearch-reply-sem-%1%") % myIndex).str().c_str(), 0));

	/// semaphore test
	assert(!sem_reply->try_wait());

	/// wait for the nodearch to get ready
	sem_reply->wait();

	/// open the prepared shared memory, we don't need the announce shm any longer
	shm.reset (new managed_shared_memory(open_only, (boost::format("nodearch-com-%1%") % myIndex).str().c_str()));

	/// get all the queues and mutexes we need for data transfer
	/// semaphores for notifying
	sem_recv = shm->find<interprocess_semaphore> ("recv-sem").first;
	sem_send = shm->find<interprocess_semaphore> ("send-sem").first;


	/// create the outgoing queue, for messages to the nodearch
	sendQueue = shm->find<shmStringVectorList>("send-q").first;
	mt_send = shm->find<interprocess_mutex>("send-q-mutex").first;
	/// and incoming queue, for messages from the nodearch
	recvQueue = shm->find<shmStringVectorList>("recv-q").first;
	mt_recv = shm->find<interprocess_mutex>("recv-q-mutex").first;

	/// start the listening thread
	worker_threads.create_thread (boost::bind(&MemNenai::recv, this));

	/// send target
	this->rawSend(MSG_TYPE_TARGET, target);

	/// send id
	this->rawSend(MSG_TYPE_ID, identifier);

}

MemNenai::~MemNenai ()
{
	string payload;

	if (frun)
		this->stop ();

	this->rawSend (MSG_TYPE_END, payload);

	worker_threads.interrupt_all();
	worker_threads.join_all();

	named_semaphore::remove ((boost::format("nodearch-reply-sem-%1%") % myIndex).str().c_str());
}

void MemNenai::recv ()
{
	boost::posix_time::ptime timeout;
	bool ret = false;

	while (true)
	{
		{
			/// wait for a global "go"
			boost::unique_lock<mutex> rlock (run_mutex);

			while (!frun)
			{
				/// this is also an interrupt point
				try
				{
					run_cond.wait (rlock);
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
			while (frun && !(ret = sem_recv->timed_wait (timeout)));
		}
		catch (...)
		{
			return;
		}

		if (!frun && !ret)
			continue;

		if (recvQueue->empty())
		{
			//cerr << "error during receive!" << endl;
			continue;
		}

		/// get payload and metadata from vector from queue
		scoped_lock<interprocess_mutex> sl(*mt_recv);
		if (recvQueue->front().size() == 2)
		{
			string payload (reinterpret_cast<char*> (&recvQueue->front()[1].at(0)), recvQueue->front()[1].size());
			//string meta(recvQueue->front()[0].c_str (), recvQueue->front()[0].size());

			callback(payload, false); // TODO: end of stream
		}
		//else
			//cerr << "received malformed message!" << endl;

		recvQueue->pop_front ();
	}
}

void MemNenai::rawSend (MSG_TYPE type, const string & payload)
{
	std::vector<uint8_t> codestring;

	for (uint32_t i=0; i < 4; i++)
		codestring.push_back (reinterpret_cast<uint8_t *> (&type)[i]);

	/// create vector to hold the strings
	shmStringVector v(shm->get_segment_manager());

	/// copy the strings to shared memory

	shmString shmMeta (&codestring.at(0), codestring.size(), shm->get_segment_manager());

	shmString shmPayload (reinterpret_cast<uint8_t *> (const_cast<char*>(payload.c_str())), payload.size(), shm->get_segment_manager());

	/// move the content instead of copying...
	v.push_back(boost::interprocess::move(shmMeta));
	v.push_back(boost::interprocess::move(shmPayload));

	scoped_lock<interprocess_mutex> sl(*mt_send);
	sendQueue->push_back(boost::interprocess::move(v));
	sem_send->post();
}


void MemNenai::run ()
{
	{
		lock_guard<mutex> rlock (run_mutex);
		frun = true;
	}
	run_cond.notify_all ();
}

void MemNenai::stop ()
{
	lock_guard<mutex> rlock (run_mutex);
	frun = false;
}

unsigned int MemNenai::getConnectionId ()
{
	return 0;
}
