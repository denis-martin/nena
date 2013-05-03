/**
 * @file memNenai.h
 *
 * @brief Implementation of the nenai interface using shared memory
 *
 *  Created on: April 15, 2010
 *      Author: Benjamin Behringer
 *
 */

#include "nenai.h"

#ifndef _SOCKETNODECONNECTOR_H_
#define _SOCKETNODECONNECTOR_H_

#include <iostream>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <string>

#include <sys/types.h>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

void empty_event_fkt(MSG_TYPE type, std::string param);

/**
 * @class SocketNenai
 *
 * @brief Offers interface for Communication with Node Architecture over Unix Socket
 *
 */
class SocketNenai : public INenai
{
private:
	std::string identifier; 	///< application name
	std::string target; 		///< remote URI or URI used for bind
	uint32_t connection_id;
	uint32_t error;

	uint32_t header[2]; /// header for incoming messages
	char * buffer;	/// buffer for incoming messages

	/// callback function, on receive
	boost::function<void(std::string payload, bool endOfStream)> ondata_callback;
	boost::function<void(MSG_TYPE type, std::string payload)> onevent_callback;

	boost::asio::io_service io_service;
	boost::asio::local::stream_protocol::socket s;
	boost::thread * worker_thread;

	/**
	 * @brief receives header of incoming messages
	 */
	void handle_header (const boost::system::error_code& error);

	/**
	 * @brief receives body of incoming messages and processes them
	 */
	void handle_body (const boost::system::error_code& error);

	virtual void rawSend (MSG_TYPE type, const std::string & payload);

public:
	/*
	 * @brief Constructor
	 *
	 * @param id		identifier, used in nodearch
	 * @param path		path to local socket
	 * @param fkt		pointer to fkt with string as parameter, called after receive
	 */
	SocketNenai (std::string id, std::string path,
		     boost::function<void(std::string payload, bool endOfStream)> data_fkt,
		     boost::function<void(MSG_TYPE type, std::string payload)> event_fkt = empty_event_fkt);
        
	virtual ~SocketNenai ();

	/**
	 * @brief start I/O activity
	 */
	virtual void run ();

	/// just starts io, blocks
	virtual void blocking_run ();
	virtual void blocking_stop ();

	/**
	 * @brief end I/O activity
	 */
	virtual void stop ();
	virtual uint32_t getConnectionId ();
	virtual uint32_t getError();

};

#endif
