/** @file
 * socketAppConnector.h
 *
 * @brief App Connector based on Boost library
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: April 17, 2010
 *      Author: Benjamin Behringer
 */
#ifndef _SOCKETAPPCONNECTOR_H_
#define _SOCKETAPPCONNECTOR_H_

#include "enhancedAppConnector.h"
#include "messages.h"
#include "messageBuffer.h"

#include <sys/types.h>
#include <list>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>

/**
 * @brief Socket based App Connection
 */

class CSocketAppConnector : public IEnhancedAppConnector
{
private:
	boost::asio::local::stream_protocol::socket s;	///< Socket connection runs on

	uint32_t header[2]; /// header for incoming messages
	uint32_t headerToApp[2];	/// header information sent to app
	shared_buffer_t buffer;	/// buffer for incoming messages
	uint32_t bytes_left;

	boost::mutex blockingVariables;

	bool writeToAppInProgress;
	bool readFromAppInProgress;
	std::list<shared_buffer_t> queueToApp;

	bool releaseOnSendComplete;
        
	boost::shared_ptr<boost::asio::io_service> io_service;	///< reference to io_service

	/**
	 * @brief takes payload from nodearch and sends it to app
	 */
	virtual void sendPayload (MSG_TYPE type, shared_buffer_t payload);

	/**
	 * @brief cleanup fkt. for send operation
	 *
	 * @param data string with payload
	 */
	void send_complete (shared_buffer_t data, const boost::system::error_code& error, std::size_t bytes_transferred);

	/**
	 * @brief receives header of incoming messages
	 */
	void handle_header (const boost::system::error_code& error, std::size_t bytes_transferred);
	/**
	 * @brief receives body of incoming messages and processes them
	 */
	void handle_body (const boost::system::error_code& error, std::size_t bytes_transferred);

	void start_header_receive();

public:

	CSocketAppConnector (CNena * nodearch, IMessageScheduler * sched, boost::shared_ptr<boost::asio::io_service> ios, IAppServer * server);
	virtual ~CSocketAppConnector ();

	virtual void start ();
	virtual void stop ();
	boost::asio::local::stream_protocol::socket & getSocket () { return s; }

	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
};

/**
 * @brief Socket based App Interface Server
 *
 * Waits for new apps to connect and spawns an CBoostAppConnector, which is the Connection.
 *
 */

class CAppSocketServer : public IEnhancedAppServer
{
	private:
	CNena * nodearch;	///< Pointer to Node Architecture, AppConnectors need this to register

	boost::shared_ptr<boost::asio::io_service> io_service;	///< reference to io_service
	boost::asio::local::stream_protocol::acceptor ac;	///< listen

	/// called on new connection
	void handle_accept (boost::shared_ptr<IEnhancedAppConnector> old_con, const boost::system::error_code& error);

	public:
	CAppSocketServer (CNena * nodearch, IMessageScheduler * sched, boost::shared_ptr<boost::asio::io_service> ios);
	virtual ~CAppSocketServer () {}

	virtual void start ();
	virtual void stop ();

	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
};

#endif
