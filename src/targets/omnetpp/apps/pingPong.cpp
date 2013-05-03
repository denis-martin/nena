/*
 * pingPong.cpp
 *
 *  Created on: Jul 21, 2009
 *      Author: denis
 */

#include "pingPong.h"

#include "debug.h"
#include "messageBuffer.h"
#include "nena.h"

#include "messageBuffer.h"

using namespace std;
using boost::shared_ptr;

const string pingPongAppId = "app://edu.kit.tm/itm/omnet/pingpong";

/**
 * @brief Header class for testing
 *
 * Header format is [A][B...][CCCC][D] where
 * A is the size of testStr (one byte),
 * B is testStr (variable length),
 * C is testInt (four bytes),
 * D is isPong (one byte).
 */
class AppPingPong_HeaderPing: public IHeader
{
public:
	std::string testStr;
	int testInt;
	bool isPong;

	/**
	 * @brief 	Serialize all relevant data into a byte buffer.
	 * 			Remember to do Little/Big Endian conversion.
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		assert(testStr.size() <= 255);

		size_t s = sizeof(unsigned char) + testStr.size() + sizeof(uint32_t) + sizeof(unsigned char);
		shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(s));

		buffer->push_uchar((unsigned char) testStr.size());
		buffer->push_string(testStr);
		buffer->push_ulong((unsigned long) testInt);
		buffer->push_uchar((unsigned char) isPong);

		return buffer;
	}

	/**
	 * @brief 	De-serialize all relevant data from a byte buffer.
	 * 			Remember to do Little/Big Endian conversion.
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer)
	{
		unsigned char strSize = buffer->pop_uchar();
		testStr = buffer->pop_string(strSize);
		testInt = (int) buffer->pop_ulong();
		isPong = (bool) buffer->pop_uchar();
	}

};

/**
 * @brief Ping timer.
 *
 * Single shot timer.
 */
class AppPingPong_PingTimer: public CTimer
{
public:
	/**
	 * @brief Constructor.
	 *
	 * @param timeout	Timeout in seconds
	 * @param proc		Node arch entity the event is linked to (default = NULL)
	 */
	AppPingPong_PingTimer(double timeout, IMessageProcessor *proc) :
		CTimer(timeout, proc)
	{
	}

	/**
	 * @brief Destructor.
	 */
	virtual ~AppPingPong_PingTimer()
	{
	}

};

/* ========================================================================= */

CPingPong::CPingPong(CNena* nodeArch, IMessageScheduler* sched) :
	CAppConnectorOmnet(sched), nodeArch(nodeArch)
{
	className += "::CPingPong";
	DBG_INFO(FMT("%1% started") % className);

	identifier = APP_OMNET_PINGPONG_NAME;

	pingName = nodeArch->getConfigValue(identifier, "ping");
	pongName = nodeArch->getConfigValue(identifier, "pong");

	// set a timeout to occur in 5 secs
	assert(scheduler != NULL);
	if (nodeArch->getNodeName() == pingName) {
		remoteURI = pongName;
		scheduler->setTimer(new AppPingPong_PingTimer(5.0, this));

	} else {
		remoteURI = pingName;
	}

}

CPingPong::~CPingPong()
{
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CPingPong::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CPingPong::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	if (msg->cast<AppPingPong_PingTimer>()) {
		handlePingTimer();

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
void CPingPong::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage();
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void CPingPong::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	/*
	 * Here, we're essentially expecting a ping message, upon which we will
	 * send a reply or relay it.
	 *
	 * This example has a major flaw:
	 * - The application is concerned about relaying the packet. This
	 *   must be moved to the Netlet
	 */

	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL)
	{
		DBG_ERROR("Unhandled incoming message!");
		throw EUnhandledMessage();
	}

	AppPingPong_HeaderPing header;
	pkt->pop_header(header);
	DBG_INFO(FMT("%1% got a packet! Content %2%, %3%.") %
			nodeArch->getNodeName() %
			header.testStr %
			header.testInt);

	string testStr = header.testStr;
	int testInt = header.testInt;
	string sender = pkt->getProperty<CStringValue>(IMessage::p_srcId)->value();
	bool reply = !header.isPong;

	if (reply) {
		// reply
		DBG_INFO(FMT("%1% is sending a reply...") % nodeArch->getNodeName());
		header.testStr = (FMT("Magic!(%1%)") % nodeArch->getNodeName()).str();
		header.testInt = testInt + 1;
		header.isPong = true;
		pkt = shared_ptr<CMessageBuffer>(new CMessageBuffer(this, next, IMessage::t_outgoing));
		pkt->push_header(header);
		pkt->setProperty(IMessage::p_serviceId, new CStringValue(getIdentifier()));
		pkt->setProperty(IMessage::p_destId, new CStringValue(sender));
		pkt->setProperty(IMessage::p_srcId, new CStringValue(nodeArch->getNodeName()));
		sendMessage(pkt);

	}
}

const string & CPingPong::getId () const
{
	return pingPongAppId;
}

void CPingPong::sendUser1(boost::shared_ptr<CMessageBuffer> message)
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
void CPingPong::sendEvent(event_t event, const std::string& param)
{
	throw EUnhandledMessage((FMT("%1%: unhandled event.") % className).str());
	return;
}

/**
 * @brief Handle ping timeout
 */
void CPingPong::handlePingTimer()
{
	DBG_INFO(FMT("%1% CPingPong: Got ping timeout, sending ping message to %2%") %
		nodeArch->getNodeName() % pongName);

	AppPingPong_HeaderPing header;
	header.testStr = (FMT("Magic!(%1%)") % nodeArch->getNodeName()).str();
	header.testInt = 42;
	header.isPong = false;

	shared_ptr<CMessageBuffer> pkt(new CMessageBuffer(this, next, IMessage::t_outgoing));
	pkt->push_header(header);

	pkt->setProperty(IMessage::p_serviceId, new CStringValue(getIdentifier()));
	pkt->setProperty(IMessage::p_destId, new CStringValue(pongName));
	pkt->setProperty(IMessage::p_srcId, new CStringValue(nodeArch->getNodeName()));

	sendMessage(pkt);

	// set a timeout to occur in 5 sec
	scheduler->setTimer(shared_ptr<CTimer> (new AppPingPong_PingTimer(5.0, this)));
}
