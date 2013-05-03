/** @file
 * memAppConnector.h
 *
 * @brief App Connector based on Boost library
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: April 17, 2010
 *      Author: Benjamin Behringer
 */
#ifndef _MEMAPPCONNECTOR_H_
#define _MEMAPPCONNECTOR_H_

#include "enhancedAppConnector.h"
#include "messages.h"
#include "messageBuffer.h"
#include "msg.h"

#include <sys/types.h>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/list.hpp>
/**
 * @brief types used in shared memory
 */
typedef boost::interprocess::allocator<uint8_t, boost::interprocess::managed_shared_memory::segment_manager> byteAllocator;
typedef boost::interprocess::basic_string<uint8_t, std::char_traits<uint8_t>, byteAllocator> shmString;
typedef boost::interprocess::allocator<shmString, boost::interprocess::managed_shared_memory::segment_manager> shmStringAllocator;
typedef boost::interprocess::vector<shmString, shmStringAllocator> shmStringVector;
typedef boost::interprocess::allocator<shmStringVector, boost::interprocess::managed_shared_memory::segment_manager> shmStringVectorAllocator;
typedef boost::interprocess::list<shmStringVector, shmStringVectorAllocator> shmStringVectorList;

/**
 * @brief Shared Memory based App Connector
 *
 * Spawned by CAppMemServer
 *
 */
class CMemAppConnector: public IEnhancedAppConnector
{
	private:
	int myIndex;

	boost::scoped_ptr<boost::interprocess::managed_shared_memory> shm;
	boost::scoped_ptr<boost::interprocess::remove_shared_memory_on_destroy> shm_guard;
	/// with this semaphore we notify the client of pending messages
	boost::interprocess::interprocess_semaphore * sem_recv;
	/// and queue we put the messages in
	shmStringVectorList * recvQueue;
	/// protecting mutex
	boost::interprocess::interprocess_mutex * mt_recv;
	/// with this semaphore the client notifies us of pending messages
	boost::interprocess::interprocess_semaphore * sem_send;
	/// queue for incoming messages
	shmStringVectorList * sendQueue;
	/// protecting mutex
	boost::interprocess::interprocess_mutex * mt_send;

	/// for "all set up" call to client
	boost::scoped_ptr<boost::interprocess::named_semaphore> sem_reply;


	/// recv thread
	boost::thread_group worker_threads;
	void recv_fkt ();

	/// condition variable for run/stop
	bool frun;
	boost::mutex run_mutex;
	boost::condition_variable run_cond;

	/**
	 * @brief takes payload from nodearch and sends it to app
	 */
	virtual void sendPayload (MSG_TYPE type, shared_buffer_t payload);

	public:
	CMemAppConnector (CNena * nodearch, IMessageScheduler * sched, int index, IAppServer * server);
	virtual ~CMemAppConnector ();

	/// return shared memory index
	int getIndex () const { return myIndex; }

	virtual void start ();
	virtual void stop ();

	/**
	 * @brief Process an event message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual const std::string & getId () const;
};

/**
 * @brief Shared Memory based App Interface Server
 *
 * Waits for new apps to connect and spawns an CMemAppConnector, which is the Connection.
 *
 */

class CAppMemServer : public IEnhancedAppServer
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

	CNena * nodearch;	///< Pointer to Node Architecture, AppConnectors need this to register

	/// variables for announce
	boost::scoped_ptr<boost::interprocess::managed_shared_memory> shm;
	boost::scoped_ptr<boost::interprocess::remove_shared_memory_on_destroy> shm_guard;
	boost::scoped_ptr<boost::interprocess::named_semaphore> sem;

	/// Worker Thread Group
	boost::thread_group worker_threads;

	/// condition variable for run/stop
	bool frun;
	boost::mutex run_mutex;
	boost::condition_variable run_cond;

	/// index variable identifies the shared memory areas
	int * global_index;
	/// local version for sync
	int local_index;
	/// mutex to protect index variable
	boost::scoped_ptr<boost::interprocess::named_mutex> index_guard;

	/**
	 * @brief processes all incoming connections
	 */
	void listen_fkt ();

	public:
	CAppMemServer (CNena * nodearch, IMessageScheduler * sched);
	virtual ~CAppMemServer ();

	virtual void start ();
	virtual void stop ();

	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual const std::string & getId () const;
};

#endif

