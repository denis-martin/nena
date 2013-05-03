/*
 * cbr.cpp
 *
 *  Created on: May 28, 2010
 *      Author: denis
 */

#include "cbr.h"

#include "debug.h"
#include "messageBuffer.h"
#include "nena.h"

#include <boost/lexical_cast.hpp>

using namespace std;
using boost::shared_ptr;

const string cbrAppId = "app://edu.kit.tm/itm/omnet/cbr";

/**
 * @brief Header class for testing
 *
 * Header format is [A][B...][CCCC][D] where
 * A is the size of testStr (one byte),
 * B is testStr (variable length),
 * C is testInt (four bytes),
 * D is isPong (one byte).
 */
class AppCbr_Header: public IHeader
{
public:
	shared_buffer_t payload;

	/**
	 * @brief 	Serialize all relevant data into a byte buffer.
	 * 			Remember to do Little/Big Endian conversion.
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		assert(payload.size() > 0 && payload.size() < 0xffff);
		shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(sizeof(ushort)));
		buffer->push_ushort((ushort) payload.size());
		buffer->push_back(payload);

		return buffer;
	}

	/**
	 * @brief 	De-serialize all relevant data from a byte buffer.
	 * 			Remember to do Little/Big Endian conversion.
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer)
	{
		assert(buffer != NULL);

		ushort size = buffer->pop_ushort();
		message_t b = buffer->pop_buffer(size);
		assert(b.length() == 1);
		payload = b.at(0);
	}

};

/**
 * @brief Ping timer.
 *
 * Single shot timer.
 */
class AppCbr_Timer: public CTimer
{
public:
	/**
	 * @brief Constructor.
	 *
	 * @param timeout	Timeout in seconds
	 * @param proc		Node arch entity the event is linked to (default = NULL)
	 */
	AppCbr_Timer(double timeout, IMessageProcessor *proc) :
		CTimer(timeout, proc)
	{
	}

	/**
	 * @brief Destructor.
	 */
	virtual ~AppCbr_Timer()
	{
	}

};

/* ========================================================================= */

CCbr::CCbr(CNena* nodeArch, IMessageScheduler* sched) :
	CAppConnectorOmnet(sched), nodeArch(nodeArch)
{
	className += "::CCbr";
	DBG_INFO(FMT("%1% started") % className);

	identifier = APP_OMNET_CBR_NAME;

	float f;

	nodeArch->getConfig()->getParameter(identifier, "sourceNode", sourceNode);
	nodeArch->getConfig()->getParameter(identifier, "destinationNode", destinationNode);

	packetSize = 0;
	interval = 0;
	jitter = 0;
	startTime = 0;
	stopTime = 0;

	if (nodeArch->getNodeName() == sourceNode) {
		nodeArch->getConfig()->getParameter(identifier, "packetSize", packetSize);
		if (packetSize < 0 || packetSize > 0xffff)
			DBG_ERROR(FMT("CBR: Packet size must be > 0 and may not exceed 65535 bytes (current value %1%)") % packetSize);

		nodeArch->getConfig()->getParameter(identifier, "interval", f);
		interval = f;
		nodeArch->getConfig()->getParameter(identifier, "jitter", f);
		jitter = f;
		nodeArch->getConfig()->getParameter(identifier, "startTime", f);
		startTime = f;
		nodeArch->getConfig()->getParameter(identifier, "stopTime", f);
		stopTime = f;

		// set a timeout to occur at startTime
		assert(scheduler != NULL);

		remoteURI = destinationNode;
		if (nodeArch->getSysTime() > startTime) {
			DBG_WARNING(FMT("CBR: Start time already past... (current time %1%, startTime %2%)")
				% nodeArch->getSysTime() % startTime);

		} else {
			scheduler->setTimer(new AppCbr_Timer(
				startTime - nodeArch->getSysTime() + (jitter/2.0 - jitter*nodeArch->getSys()->random()), this));

		}

	}

}

CCbr::~CCbr()
{
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CCbr::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CCbr::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	if (msg->cast<AppCbr_Timer>()) {
		handleTimer();

	} else {
		DBG_ERROR("Unhandled timer!");
		throw EUnhandledMessage();

	}
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void CCbr::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage();
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void CCbr::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL)
	{
		DBG_ERROR("Unhandled incoming message!");
		throw EUnhandledMessage();
	}

	AppCbr_Header header;
	pkt->pop_header(header);
	DBG_INFO(FMT("CBR: %1% got a packet! Payload size %2% bytes") %
		nodeArch->getNodeName() %
		header.payload.size());
}

const string & CCbr::getId () const
{
	return cbrAppId;
}

void CCbr::sendUser1(boost::shared_ptr<CMessageBuffer> message)
{
	throw EUnhandledMessage((FMT("%1%: unhandled USER_1 message.") % className).str());
	return;
}

/**
 * @brief	Send an event to the application
 *
 * @param event		Event ID
 * @param param		Parameter for the event
 */
void CCbr::sendEvent(event_t event, const std::string& param)
{
	throw EUnhandledMessage((FMT("%1%: unhandled event.") % className).str());
	return;
}

/**
 * @brief Handle timeout
 */
void CCbr::handleTimer()
{
	DBG_INFO(FMT("CBR: %1% got timeout, sending CBR message to %2%") %
		nodeArch->getNodeName() % destinationNode);

	AppCbr_Header header;
	header.payload = shared_buffer_t(packetSize);

	shared_ptr<CMessageBuffer> pkt(new CMessageBuffer(this, next, IMessage::t_outgoing));
	pkt->push_header(header);
	pkt->setProperty(IMessage::p_serviceId, new CStringValue(getIdentifier()));
	pkt->setProperty(IMessage::p_destId, new CStringValue(destinationNode));
	pkt->setProperty(IMessage::p_srcId, new CStringValue(nodeArch->getNodeName()));
	sendMessage(pkt);

	// set next timeout
	double t = interval + (jitter/2.0 - jitter*nodeArch->getSys()->random());
	if (nodeArch->getSysTime() + t < stopTime) {
		scheduler->setTimer(new AppCbr_Timer(t, this));

	} else {
		DBG_INFO(FMT("CBR: %1% sent last packet of CBR stream to node %2%, stopping")
			% nodeArch->getNodeName() % destinationNode);

	}
}

