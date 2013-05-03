/*
 * bb_simpleSegment.cpp
 *
 *  Created on: Jul 16, 2012
 *      Author: denis
 */

#include "bb_simpleSegment.h"

#include "nena.h"

#include <string>

#define SIMPLESEGMENT_SIZE	1024

namespace edu_kit_tm {
namespace itm {
namespace transport {

using std::list;
using boost::shared_ptr;
using std::string;

Bb_SimpleSegment::Bb_SimpleSegment(CNena *nena, IMessageScheduler *sched, IComposableNetlet *netlet, const string id)
	: IBuildingBlock(nena, sched, netlet, id)
{
	className += "::Bb_SimpleSegment";
	setId(BB_SIMPLESEGMENT_ID);
}

Bb_SimpleSegment::~Bb_SimpleSegment()
{
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleSegment::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<IEvent> ev = msg->cast<IEvent>();
	assert(ev.get());

	string m = (FMT("%1%: unhandled event %2%") % getId() % ev->getId()).str();
//	DBG_ERROR(m);
	throw EUnhandledMessage(m);
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleSegment::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage("Unhandled timer!");
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleSegment::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	assert(pkt.get() != NULL);
	
	shared_ptr<CFlowState> flowState = pkt->getFlowState();
	assert(flowState.get() != NULL);

	bool endOfStream = false;
	try {
		endOfStream = pkt->getProperty<CBoolValue>(IMessage::p_endOfStream)->value();
	} catch (...) {
		// nothing
	}

	pkt->setFrom(this);
	pkt->setTo(getNext());

	if (endOfStream)
		pkt->getProperty<CBoolValue>(IMessage::p_endOfStream)->set(false);

	while (pkt->size() > SIMPLESEGMENT_SIZE) {
		shared_ptr<CMessageBuffer> newpkt = pkt->clone();
		newpkt->remove_back(SIMPLESEGMENT_SIZE);
		pkt->remove_front(SIMPLESEGMENT_SIZE);

		sendMessage(newpkt);
		FLOWSTATE_FLOATOUT_INC(flowState, 1, "segment", nena->getSysTime());

	}

	if (endOfStream)
		pkt->getProperty<CBoolValue>(IMessage::p_endOfStream)->set(true);

	if (pkt->size() > 0 || endOfStream) {
		sendMessage(pkt);

	} else {
		FLOWSTATE_FLOATOUT_DEC(flowState, 1, "segment", nena->getSysTime());

	}
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleSegment::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	msg->setFrom(this);
	msg->setTo(getPrev());
	sendMessage(msg);
}

const std::string & Bb_SimpleSegment::getId() const
{
	return BB_SIMPLESEGMENT_ID;
}

}
}
}
