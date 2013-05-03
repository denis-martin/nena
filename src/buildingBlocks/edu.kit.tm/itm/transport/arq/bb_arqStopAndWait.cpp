/*
 * bb_arqStopAndWait.cpp
 *
 *  Created on: Jan 29, 2010
 *      Author: denis
 */

#include "bb_arqStopAndWait.h"

#include "nena.h"
#include "mutexes.h"
#include "locks.h"
#include <string>

namespace edu_kit_tm {
namespace itm {
namespace transport {

using std::list;
using boost::shared_ptr;
using std::string;

#define ARQSTOPANDWAIT_SEQNO_BITS			16
#define ARQSTOPANDWAIT_SEQNO_INC(seqNo)		seqNo = (seqNo + 1) % (1 << ARQSTOPANDWAIT_SEQNO_BITS)
#define ARQSTOPANDWAIT_SEQNO_ISGT(a, b)		((int) a - (int) b < -1*(1 << (ARQSTOPANDWAIT_SEQNO_BITS-1)) ? true : ((int) a - (int) b > 0 && (int) a - (int) b < (1 << (ARQSTOPANDWAIT_SEQNO_BITS-1))))

#define ARQSTOPANDWAIT_RETRYLIMIT			10

const string & ADMIN_NETLET_NAME = "netlet://edu.kit.tm/itm/simpleArch/adminNetlet";
const string & AGENT_NETLET_NAME = "netlet://edu.kit.tm/itm/simpleArch/agentNetlet";

extern const std::string arqStopAndWaitClassName = "bb://edu.kit.tm/itm/transport/arq/stopAndWait";

class ArqStopAndWaitHeader : public IHeader
{
public:
	ushort seqNo;
	bool isAck;

	ArqStopAndWaitHeader(ushort seqNo = 0, bool isAck = false) : seqNo(seqNo), isAck(isAck) {}
	virtual ~ArqStopAndWaitHeader() {}

	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(sizeof(ushort) + sizeof(unsigned char)));
		buffer->push_ushort(seqNo);
		buffer->push_uchar((unsigned char) isAck);
		return buffer;
	}

	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer)
	{
		seqNo = buffer->pop_ushort();
		isAck = (bool) buffer->pop_uchar();
	}
};

Bb_ArqStopAndWait::Bb_ArqStopAndWait(CNena *nena, IMessageScheduler *sched, IComposableNetlet *netlet, const string id)
	: IBuildingBlock(nena, sched, netlet, id)
{
	className += "::Bb_ArqStopAndWait";

	seqNoMutex.reset(nena->getSys()->getSyncFactory()->mutexFactory());
	queueMutex.reset(nena->getSys()->getSyncFactory()->mutexFactory());

	// random initialization
	localSeqNo = (ushort) (nena->getSys()->random() * (1 << ARQSTOPANDWAIT_SEQNO_BITS) - 1);
}

Bb_ArqStopAndWait::~Bb_ArqStopAndWait()
{
	if (outgoingQueue.size() > 0)
		DBG_WARNING(FMT("%1% is missing %2% acknowledgments") % getClassName() % outgoingQueue.size());

	list<QueueItem>::iterator it;
	outgoingQueue.clear();
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_ArqStopAndWait::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_ArqStopAndWait::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<AckTimer> timer = msg->cast<AckTimer>();
	if (timer.get() == NULL) {
		DBG_ERROR("Unhandled timer!");
		throw EUnhandledMessage();

	}

	lastTimer.reset();

	if (!timer->isCancled) {
		// resend packet
		sendNextPacket();
	}
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void Bb_ArqStopAndWait::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	assert(pkt.get() != NULL);
	
	ushort seqNo;

	{
		LockGuard<IMutex> seqNoGuard(*seqNoMutex);
		ARQSTOPANDWAIT_SEQNO_INC(localSeqNo);
		seqNo = localSeqNo;
	}
	pkt->push_header(ArqStopAndWaitHeader(seqNo));


	bool sendNow = false;
	{
		LockGuard<IMutex> queueGuard(*queueMutex);
		outgoingQueue.push_back(QueueItem(seqNo, pkt));
		sendNow = (outgoingQueue.size() == 1);
	}
	if (sendNow)
	{
		// send out immediately
		sendNextPacket();
	}
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void Bb_ArqStopAndWait::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	assert(pkt.get() != NULL);

	ArqStopAndWaitHeader hdr;
	pkt->pop_header(hdr);
	bool sendNow = false;

	if (hdr.isAck)
	{
		// received ACK for one of our packets
//		DBG_DEBUG(FMT("%1%: Received ACK for seqNo %2%") % className % hdr->seqNo);
		
		{
			LockGuard<IMutex> queueGuard(*queueMutex);
			
			list<QueueItem>::iterator it = outgoingQueue.begin();
			// should be at the beginning of the queue anyway
			for (; it != outgoingQueue.end() && (it->seqNo != hdr.seqNo); it++)
				DBG_INFO(FMT("%1%: tested seqNo %2% for %3%") % className % it->seqNo % hdr.seqNo);

			if (it != outgoingQueue.end())
			{
				outgoingQueue.erase(it);
				sendNow = true;
			}
			else
			{
				DBG_WARNING(FMT("%1%: ack'ed packet with seqNo %2% not in outgoing Queue, queue size: %3%!") % className % hdr.seqNo % outgoingQueue.size());
			}
		}
		
		// send next packet
		if (sendNow)
			sendNextPacket();

	}
	else
	{
		// hand to upper entity
		pkt->setFrom(this);
		pkt->setTo(getPrev());
		sendMessage(pkt);

		// send ACK
		shared_ptr<CMorphableValue> lv;
		if (pkt->hasProperty(IMessage::p_srcLoc, lv)) {
//			DBG_DEBUG(FMT("%1%: Sending ACK for seqNo %2% to %3%") % className % hdr->seqNo % lv->toStr());
			shared_ptr<CMessageBuffer> ack(new CMessageBuffer(this, getNext()));
			ack->push_header(ArqStopAndWaitHeader(hdr.seqNo, true));
			ack->setProperty(IMessage::p_destLoc, lv);
			/// fake this, so the message will be received by a agent netlet
			bool isAdminNetlet = (this->netlet->getMetaData()->getId() == ADMIN_NETLET_NAME);
			ack->setProperty(IMessage::p_netletId, new CStringValue(isAdminNetlet ? AGENT_NETLET_NAME : ADMIN_NETLET_NAME));
			sendMessage(ack);

		} else {
			DBG_WARNING("Unknown src locator, cannot send ACK!");

		}

	}

}

void Bb_ArqStopAndWait::sendNextPacket()
{
	LockGuard<IMutex> queueGuard(*queueMutex);

	while (outgoingQueue.size() > 0 && outgoingQueue.front().retries >= ARQSTOPANDWAIT_RETRYLIMIT) {
		DBG_WARNING(FMT("%1%: exceeded retry limit, discarding packet") % getClassName());
		outgoingQueue.pop_front();
	}

	if (outgoingQueue.size() > 0)
	{
		// we need to duplicate the packet
		shared_ptr<CMessageBuffer> pkt = outgoingQueue.front().pkt->clone();
//		DBG_DEBUG (FMT("%1%: sending packet with seqNo %2%") % className % outgoingQueue.front().seqNo);
		pkt->setFrom(this);
		pkt->setTo(getNext());
		sendMessage(pkt);
		outgoingQueue.front().retries++;

		if (lastTimer != NULL)
			// lastTimer will be deleted in processTimer()
			lastTimer->isCancled = true;

		lastTimer.reset();
		lastTimer = shared_ptr<AckTimer>(new AckTimer(this));
		scheduler->setTimer<AckTimer>(lastTimer);

	} else {
		if (lastTimer != NULL)
			// lastTimer will be deleted in processTimer()
			lastTimer->isCancled = true;

	}
}

}
}
}
