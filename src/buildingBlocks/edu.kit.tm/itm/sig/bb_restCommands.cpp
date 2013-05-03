/*
 * bb_restCommands.cpp
 *
 *  Created on: Jul 11, 2012
 *      Author: denis
 */

#include "bb_restCommands.h"

#include "nena.h"
#include "netletSelector.h"
#include "mutexes.h"
#include "locks.h"
#include <string>

namespace edu_kit_tm {
namespace itm {
namespace sig {

using std::list;
using boost::shared_ptr;
using std::string;

class RestCommandsHeader : public IHeader
{
public:
	IAppConnector::method_t msgType;
	string requestUri;

	RestCommandsHeader(IAppConnector::method_t msgType = IAppConnector::method_none, string requestUri = string()) :
		msgType(msgType), requestUri(requestUri) {}
	virtual ~RestCommandsHeader() {}

	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		size_t size = sizeof(unsigned char);
		if (msgType != IAppConnector::method_none)
			size += sizeof(ushort) + requestUri.size();

		shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(size));

		buffer->push_uchar((unsigned char) msgType);
		if (msgType != IAppConnector::method_none) {
			buffer->push_ushort((ushort) requestUri.size());
			if (!requestUri.empty())
				buffer->push_string(requestUri);
		}
		return buffer;
	}

	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer)
	{
		msgType = (IAppConnector::method_t) buffer->pop_uchar();
		if (msgType != IAppConnector::method_none) {
			ushort size = buffer->pop_ushort();
			if (size > 0)
				requestUri = buffer->pop_string(size);
			else
				requestUri.clear();
		}
	}
};

Bb_RestCommands::Bb_RestCommands(CNena *nena, IMessageScheduler *sched, IComposableNetlet *netlet, const string id)
	: IBuildingBlock(nena, sched, netlet, id)
{
	className += "::Bb_RestCommands";
	setId(BB_RESTCOMMANDS_ID);
}

Bb_RestCommands::~Bb_RestCommands()
{
}

shared_ptr<CMessageBuffer> Bb_RestCommands::createCtrlPacket(shared_ptr<CFlowState> flowState,
		shared_ptr<RestCommandsStateObject> so, IAppConnector::method_t msgType, std::string requestUri)
{
	RestCommandsHeader hdr(msgType, requestUri);
	boost::shared_ptr<CMessageBuffer> pkt = hdr.serialize();
	pkt->setFrom(this);
	pkt->setTo(getNext());
	pkt->setType(IMessage::t_outgoing);
	pkt->setProperty(IMessage::p_srcId, new CStringValue(nena->getNodeName()));
	pkt->setProperty(IMessage::p_destId, new CStringValue(flowState->getRemoteId()));
	pkt->setFlowState(flowState);
	return pkt;
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_RestCommands::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<IEvent> event = msg->cast<IEvent>();
	assert(event.get() != NULL);

	if (event->getId() == EVENT_NETLETSELECTOR_APPCONNECT) {
		shared_ptr<CNetletSelector::Event_AppConnect> ac_event = msg->cast<CNetletSelector::Event_AppConnect>();
		assert(ac_event.get() != NULL);

		DBG_DEBUG(FMT("%1%: got app connect event (method %2%)") % getId() % ac_event->method);

		switch (ac_event->method) {
		case IAppConnector::method_get:
		case IAppConnector::method_put:
		case IAppConnector::method_connect: {
			shared_ptr<CMessageBuffer> pkt = createCtrlPacket(msg->getFlowState(),
					shared_ptr<RestCommandsStateObject>(), ac_event->method, ac_event->requestUri);
			sendMessage(pkt);
			break;
		}
		default:
			break;
		}

		return;

	}

	throw EUnhandledMessage("unhandled event!");
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_RestCommands::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage("unhandled timer!");
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void Bb_RestCommands::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	assert(pkt.get() != NULL);
	
	bool endOfStream = false;
	try {
		endOfStream = pkt->getProperty<CBoolValue>(IMessage::p_endOfStream)->value();
	} catch (...) {
		// nothing
	}

	if (!endOfStream) {
		RestCommandsHeader hdr;
		pkt->push_header(hdr);

	}

	pkt->setFrom(this);
	pkt->setTo(getNext());
	sendMessage(pkt);
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void Bb_RestCommands::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	assert(pkt.get() != NULL);

	bool endOfStream = false;
	try {
		endOfStream = pkt->getProperty<CBoolValue>(IMessage::p_endOfStream)->value();
	} catch (...) {
		// nothing
	}

	if (!endOfStream) {
		RestCommandsHeader hdr;
		pkt->pop_header(hdr);

		switch (hdr.msgType) {
		case IAppConnector::method_get:
		case IAppConnector::method_put:
		case IAppConnector::method_connect: {
			std::map<IAppConnector::method_t, string> methodMap;
			methodMap[IAppConnector::method_get] = "GET";
			methodMap[IAppConnector::method_put] = "PUT";
			methodMap[IAppConnector::method_connect] = "CONNECT";
			DBG_DEBUG(FMT("%1%: %2% %3%") % getId() % methodMap[hdr.msgType] % hdr.requestUri);

			pkt->getFlowState()->setRequestUri(hdr.requestUri);
			pkt->getFlowState()->setRequestMethod(hdr.msgType);
			pkt->setFrom(this);
			pkt->setTo(getPrev());
			sendMessage(pkt);
			break;
		}
		default:
			pkt->setFrom(this);
			pkt->setTo(getPrev());
			sendMessage(pkt);
			break;
		}

	} else {
		// forward end-of-stream marker
//		DBG_DEBUG(FMT("%1%(%2%) forwarding incoming end-of-stream marker") % getId() % pkt->getFlowState()->getFlowId());
		pkt->setFrom(this);
		pkt->setTo(getPrev());
		sendMessage(pkt);

	}
}

const std::string & Bb_RestCommands::getId() const
{
	return BB_RESTCOMMANDS_ID;
}

}
}
}
