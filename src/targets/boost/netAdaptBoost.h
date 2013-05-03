/** @file
 * netAdaptBoost.h
 *
 * @brief Interface to boost::asio based network adaptors
 *
 * (c) 2008-2011 Institut fuer Telematik, KIT, Germany
 *
 *  Created on: Aug 28, 2008
 *      Authors: Benjamin Behringer, Denis Martin
 */

#ifndef NETADAPTBOOST_H_
#define NETADAPTBOOST_H_

#include "netAdapt.h"
#include "messageBuffer.h"

#include <string>
#include <pugixml.h>

#include <boost/asio.hpp>

#define BOOST_RECVBUFSIZE		2048		///< receive_from buffer size for a single datagram
#define BOOST_SYSRECVBUFSIZE	2097152		///< socket receive buffer size (2 MB)
#define BOOST_SYSSENDBUFSIZE	1048576		///< socket send buffer size (1 MB)

/*****************************************************************************/

class CBoostNetAdapt : public INetAdapt
{
protected:

	class CNetworkFrame {
	public:
		boctet_t* buffer;
		bsize_t size;
		boost::shared_ptr<CFlowState> flowState;

		CNetworkFrame(bsize_t size, boost::shared_ptr<CFlowState> flowState) :
			buffer(new boctet_t[size]), size(size), flowState(flowState)
		{
			assert(size > 0);
		};

		virtual ~CNetworkFrame() {
			delete[] buffer;
			flowState.reset();
		}
	};

protected:
	boost::shared_ptr<boost::asio::io_service> io_service;		///< io service which does the work for us
	shared_buffer_t recv_buffer; 								///< incoming buffer
	boost::shared_ptr<pugi::xml_document> sysConf; 				///< holds pointer to system config

	float dropRate;											///< drop rate (for testing only)

	std::size_t stat_rx_bytes; 									///< for measuring incoming bandwidth
	std::size_t stat_tx_bytes; 									///< for measuring outgoing bandwidth

	CMessageBufferPool messageBufferPool; 						///< message buffer object pool
	CSharedBufferPool recvBufferPool; 							///< buffer pool



	/**
	 * @brief	Start async receive
	 *
	 * 			When something is received, handle_receive() will be called by boost's io_service.
	 */
	virtual void start_receive() = 0;
	
	/**
	 * @brief	Callback after something has been received
	 */
	virtual void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);
	
	/**
	 * @brief	Callback after a send operation
	 */
	virtual void handle_send(boost::shared_ptr<CNetworkFrame> frame, const boost::system::error_code& error, std::size_t bytes_transferred);

	/**
	 * @brief	Additional packet mangling of child classes (optional)
	 *
	 * @return True, if packet should be sent to the network stack, false if it should be dropped
	 */
	virtual bool mangle_incoming(boost::shared_ptr<CMessageBuffer>& pkt);

	/**
	 * @grief	Return architecture-specific locator of last sender
	 */
	virtual boost::shared_ptr<CMorphableValue> getLastSender() = 0;
		
public:
	CBoostNetAdapt(CNena *nodeA, IMessageScheduler *sched,
			const std::string& uri,
			boost::shared_ptr<boost::asio::io_service> ios);
	virtual ~CBoostNetAdapt();

	/**
	 * @brief	Check whether network adaptor is ready for sending/receiving data
	 */
	virtual bool isReady() const = 0;

	/**
	 * @brief Process an event message directed to this message processing unit
	 *
	 * Nothing implemented yet.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	 * @brief Process a timer message directed to this message processing unit
	 *
	 * Nothing implemented yet.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	 * @brief Process an outgoing message directed towards the network.
	 *
	 * This is the most common case of a application induced message for this NetAdapt.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) = 0;

	/**
	 * @brief Process an incoming message directed towards the application.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	 * @brief this version of the NetAdapt is thread safe
	 */
	virtual bool is_threadsafe () const { return true; }
};

#endif // NETADAPTBOOST_H_
