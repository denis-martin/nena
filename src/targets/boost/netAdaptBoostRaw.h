/** @file
 * netAdaptBoostRaw.h
 *
 * @brief Interface to a RAW device.
 *
 * (c) 2008-2011 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Jun 16, 2011
 *     Authors: Denis Martin
 */

#ifndef NETADAPTBOOSTRAW_H_
#define NETADAPTBOOSTRAW_H_

#include "netAdapt.h"
#include "messageBuffer.h"
#include "flowState.h"
#include "morphableValue.h"

#include <net/ethernet.h>

#include <string>

/// Boost libs
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>

#define BOOSTRAW_RECVBUFSIZE	2048		///< receive_from buffer size for a single datagram
#define BOOSTRAW_SYSRECVBUFSIZE	2097152		///< raw socket receive buffer size (2 MB)
#define BOOSTRAW_SYSSENDBUFSIZE	1048576		///< raw socket send buffer size (1 MB)

/*****************************************************************************/

class CBoostRawNetAdapt : public INetAdapt
{
private:

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
		}
	};

private:
	/// io service which does the work for us
	boost::shared_ptr<boost::asio::io_service> io_service;

	/// stream descriptor of raw socket
	boost::asio::posix::stream_descriptor ifdesc;

	/// ethernet address
	struct sockaddr hwaddr;
	std::string hwaddr_str;

	std::string interface; //< device name (eg. eth0)
	uint32_t ifindex; //< devince index
	uint32_t ethertype; //< Ethernet protocol ID

	/// incoming buffer
	char recv_buffer[BOOSTRAW_RECVBUFSIZE];
	
	/// ready for send/receive
	bool isReady_;

	/// PCAP file
	std::ofstream pcapfile;

	/// callback method for socket
	void start_receive ();
	
	/**
	* @brief callback method for socket
	*
	* We use async receive and call "handle_receive", when something was received.
	*
	*/
	void handle_receive (const boost::system::error_code& error, std::size_t /*bytes_transferred*/);
	
	/// cleanup function, called after send
	void handle_send (boost::shared_ptr<CNetworkFrame> frame, const boost::system::error_code& error, std::size_t /*bytes_transferred*/);
		
public:
	CBoostRawNetAdapt (CNena *nodeA, IMessageScheduler *sched,
			 const std::string& uri,
			 boost::shared_ptr<boost::asio::io_service> ios);

	virtual ~CBoostRawNetAdapt ();
	
	/**
	 * @brief	Check whether network adaptor is ready for sending/receiving data
	 */
	virtual bool isReady() const;

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
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

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

#endif // NETADAPTBOOSTRAW_H_
