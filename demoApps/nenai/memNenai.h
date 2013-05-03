/**
 * @file memNenai.h
 *
 * @brief Implementation of the nenai interface using shared memory
 *
 *  Created on: April 15, 2010
 *      Author: Benjamin Behringer
 *
 */

#ifndef _MEMNODECONNECTOR_H_
#define _MEMNODECONNECTOR_H_

#include "nenai.h"

#include <string>

#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/list.hpp>

/**
 * @brief types used in shared memory
 */
typedef boost::interprocess::allocator<uint8_t, boost::interprocess::managed_shared_memory::segment_manager> charAllocator;
typedef boost::interprocess::basic_string<uint8_t, std::char_traits<uint8_t>, charAllocator> shmString;
typedef boost::interprocess::allocator<shmString, boost::interprocess::managed_shared_memory::segment_manager> shmStringAllocator;
typedef boost::interprocess::vector<shmString, shmStringAllocator> shmStringVector;
typedef boost::interprocess::allocator<shmStringVector, boost::interprocess::managed_shared_memory::segment_manager> shmStringVectorAllocator;
typedef boost::interprocess::list<shmStringVector, shmStringVectorAllocator> shmStringVectorList;

/**
 * @class MemNenai
 *
 * @brief Offers interface for Communication with Node Architecture over Shared Memory
 *
 */
class MemNenai : public INenai
{
	private:
	std::string identifier, target;

	boost::scoped_ptr<boost::interprocess::managed_shared_memory> shm;
	boost::scoped_ptr<boost::interprocess::named_semaphore> sem;

	/// global index variable
	int * global_index;
	boost::scoped_ptr<boost::interprocess::named_mutex> index_guard;

	/// with this semaphore we get notified of pending messages
	boost::interprocess::interprocess_semaphore * sem_recv;
	/// and queue we put the messages in
	shmStringVectorList * recvQueue;
	/// protecting mutex
	boost::interprocess::interprocess_mutex * mt_recv;
	/// with this semaphore we notify the nodearch of pending messages
	boost::interprocess::interprocess_semaphore * sem_send;
	/// queue for incoming messages
	shmStringVectorList * sendQueue;
	/// protecting mutex
	boost::interprocess::interprocess_mutex * mt_send;

	/// for "all set up" call to client
	boost::scoped_ptr<boost::interprocess::named_semaphore> sem_reply;

	/// shared memory identifier
	int myIndex;

	/// callback function, on receive
	boost::function<void(std::string payload, bool endOfStream)> callback;
	//void (* callback) (string & payload);

	/// recv fkt, waits for incoming messages
	void recv ();

	/// dumb send operation, just sends data
	virtual void rawSend (MSG_TYPE type, const std::string & payload);

	/// Worker Thread Group
	boost::thread_group worker_threads;

	/// condition variable for run/stop
	bool frun;
	boost::mutex run_mutex;
	boost::condition_variable run_cond;

public:
	/*
	 * @brief Constructor
	 *
	 * @param id		identifier, used in nodearch
	 * @param t			target
	 * @param fkt		pointer to fkt with char array and size as parameter, called after receive
	 */
	MemNenai (std::string id, std::string t, boost::function<void(std::string payload, bool endOfStream)> fkt);
        
	virtual ~MemNenai ();

	/**
	 * @brief start I/O activity
	 */
	virtual void run ();

	/**
	 * @brief end I/O activity
	 */
	virtual void stop ();
	virtual unsigned int getConnectionId();
};

#endif
