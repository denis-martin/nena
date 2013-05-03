/** @file
 * netAdaptBoostUDP.h
 *
 * @brief Interface to UDP Communication using Boost library.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Aug 28, 2008
 *      Author: Benjamin Behringer
 */

#ifndef NETADAPTBOOSTUDP_H_
#define NETADAPTBOOSTUDP_H_

#include "netAdaptBoost.h"
#include "ipv4.h"


/*****************************************************************************/

class CBoostUDPNetAdapt : public CBoostNetAdapt
{
protected:
	boost::asio::ip::udp::socket socket;		///< socket we want to use
	boost::asio::ip::udp::endpoint sender;		///< this is where the last message came from
	std::map<uint32_t, bool> allowedHosts;		///< list of allowed hosts

	/**
	 * @brief	Start async receive
	 *
	 * 			When something is received, handle_receive() will be called by boost's io_service.
	 */
	virtual void start_receive();
	
	/**
	 * @brief	Additional packet mangling of child classes (optional)
	 */
	virtual bool mangle_incoming(boost::shared_ptr<CMessageBuffer>& pkt);

	/**
	 * @grief	Return architecture-specific locator of last sender
	 */
	virtual boost::shared_ptr<CMorphableValue> getLastSender();
		
public:
	CBoostUDPNetAdapt(CNena *nodeA, IMessageScheduler *sched,
			const std::string& uri,
			boost::shared_ptr<boost::asio::io_service> ios);
	virtual ~CBoostUDPNetAdapt ();

	/**
	 * @brief	Check whether network adaptor is ready for sending/receiving data
	 */
	virtual bool isReady() const;

	/**
	 * @brief Process an outgoing message directed towards the network.
	 *
	 * This is the most common case of a application induced message for this NetAdapt.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	 * @brief	Manually set configuration. Socket is bound to port and
	 * 			architecture given in the netadapt's property list.
	 */
	virtual void setConfig();
};

#endif // NETADAPTBOOSTUDP_H_
