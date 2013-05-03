/*
 * bb_lossSig.cpp
 *
 *  Created on: Jul 11, 2012
 *      Author: denis
 */

#include "bb_lossSig.h"

#include "nena.h"
#include "netletSelector.h"
#include "mutexes.h"
#include "locks.h"
#include <string>

#define SEQNO_BITS				16
#define SEQNO_INC(seqNo)		seqNo = (seqNo + 1) % (1 << SEQNO_BITS)
#define SEQNO_ISGT(a, b)		((int) a - (int) b < -1*(1 << (SEQNO_BITS-1)) ? true : ((int) a - (int) b > 0 && (int) a - (int) b < (1 << (SEQNO_BITS-1))))
#define SEQNO_DIFF(low, high)	((high >= low) ? (high - low) : ((1 << SEQNO_BITS) - low + high))

namespace edu_kit_tm {
namespace itm {
namespace sig {

using std::list;
using boost::shared_ptr;
using std::string;

/**
 * @brief	Header sent with each outgoing packet
 */
class LossSigHeader : public IHeader
{
public:
	ushort seqNo;
	unsigned char hasLossInfo;

	LossSigHeader(ushort seqNo = 0, unsigned char hasLossInfo = 0) : seqNo(seqNo), hasLossInfo(hasLossInfo) {}
	virtual ~LossSigHeader() {}

	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		size_t size = sizeof(ushort) + sizeof(unsigned char);
		shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(size));
		buffer->push_ushort(seqNo);
		buffer->push_uchar(hasLossInfo);
		return buffer;
	}

	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer)
	{
		seqNo = buffer->pop_ushort();
		hasLossInfo = buffer->pop_uchar();
	}
};

/**
 * @brief	Loss info sent upon timer event
 */
class LossSigInfo : public IHeader
{
public:
	uint32_t packetCount; // total number of packets received
	uint32_t lossCount; // total number of packets lost

	LossSigInfo(uint32_t packetCount = 0, uint32_t lossCount = 0) :
		packetCount(packetCount), lossCount(lossCount) {}
	virtual ~LossSigInfo() {}

	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		size_t size = sizeof(uint32_t) * 2;
		shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(size));
		buffer->push_ulong(packetCount);
		buffer->push_ulong(lossCount);
		return buffer;
	}

	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer)
	{
		packetCount = buffer->pop_ulong();
		lossCount = buffer->pop_ulong();
	}
};

Bb_LossSig::Bb_LossSig(CNena *nena, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id)
	: IBuildingBlock(nena, sched, netlet, id)
{
	className += "::Bb_LossSig";
	setId(BB_LOSSSIG_ID);
}

Bb_LossSig::~Bb_LossSig()
{
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_LossSig::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage("unhandled event");
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_LossSig::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<SigTimer> sigt = msg->cast<SigTimer>();
	if (sigt.get() == 0)
		throw EUnhandledMessage("unhandled timer");

	if (sigt->flowState->getOperationalState() == CFlowState::s_valid) {
		shared_ptr<CFlowState::StatisticsObject> sso = sigt->flowState->getStateObject(FLOWSTATE_STATISTICS_ID)->cast<CFlowState::StatisticsObject>();
		shared_ptr<LossSigStateObject> lso = sigt->flowState->getStateObject(BB_LOSSSIG_ID)->cast<LossSigStateObject>();

		uint32_t packetCount = sso->values[CFlowState::stat_packetCountIn]->cast<CIntValue>()->value();
		uint32_t lossCount = sso->values[CFlowState::stat_lossCountIn]->cast<CIntValue>()->value();

		shared_ptr<CMessageBuffer> pkt = LossSigInfo(packetCount, lossCount).serialize();
		pkt->push_header(LossSigHeader(++lso->lastSeqNoSent, 1));

		pkt->setFrom(this);
		pkt->setTo(getNext());
		pkt->setType(IMessage::t_outgoing);
		pkt->setProperty(IMessage::p_srcId, new CStringValue(nena->getNodeName()));
		pkt->setProperty(IMessage::p_destId, new CStringValue(sigt->flowState->getRemoteId()));
		pkt->setFlowState(sigt->flowState);

//		DBG_DEBUG(FMT("%1%: sending loss info (packetCount %2%, lossCount %3%)") % getId() % packetCount % lossCount);

		sendMessage(pkt);
		scheduler->setTimer(sigt);

	}
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void Bb_LossSig::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
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
		shared_ptr<CFlowState::StateObject> so = msg->getFlowState()->getStateObject(BB_LOSSSIG_ID);
		if (so.get() == NULL) {
			so.reset(new LossSigStateObject());
			msg->getFlowState()->addStateObject(BB_LOSSSIG_ID, so);

			// SigTimer not started (only needed when packets are received)

		}

		shared_ptr<LossSigStateObject> lso = so->cast<LossSigStateObject>();
		LossSigHeader hdr(++lso->lastSeqNoSent);
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
void Bb_LossSig::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
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
		shared_ptr<CFlowState::StateObject> so = msg->getFlowState()->getStateObject(BB_LOSSSIG_ID);
		if (so.get() == NULL) {
			so.reset(new LossSigStateObject());
			msg->getFlowState()->addStateObject(BB_LOSSSIG_ID, so);

			scheduler->setTimer(new SigTimer(this, msg->getFlowState()));
		}

		shared_ptr<LossSigStateObject> lso = so->cast<LossSigStateObject>();

		shared_ptr<CFlowState::StatisticsObject> sso = msg->getFlowState()->getStateObject(FLOWSTATE_STATISTICS_ID)->cast<CFlowState::StatisticsObject>();
		shared_ptr<CIntValue> iv;

		LossSigHeader hdr;
		pkt->pop_header(hdr);

		if (!lso->recvSeqNoValid) {
			lso->recvSeqNoValid = true;

		} else {
			ushort diff = SEQNO_DIFF(lso->lastSeqNoRcvd, hdr.seqNo);
			if (diff > 1) {
				iv = sso->values[CFlowState::stat_lossCountIn]->cast<CIntValue>();
				iv->set(iv->value() + diff - 1);
			}
		}

		lso->lastSeqNoRcvd = hdr.seqNo;

		// TODO: count that in multiplexer
		iv = sso->values[CFlowState::stat_packetCountIn]->cast<CIntValue>();
		iv->set(iv->value() + 1);

		if (hdr.hasLossInfo) {
			LossSigInfo info;
			pkt->pop_header(info);

			shared_ptr<CIntValue> packetCount = sso->values[CFlowState::stat_remote_packetCountIn]->cast<CIntValue>();
			shared_ptr<CIntValue> lossCount = sso->values[CFlowState::stat_remote_lossCountIn]->cast<CIntValue>();
			shared_ptr<CDoubleValue> lossRate = sso->values[CFlowState::stat_remote_lossRate]->cast<CDoubleValue>();

			packetCount->set(info.packetCount);
			lossCount->set(info.lossCount);

			double currLossRate = 0;
			if (((uint32_t) packetCount->value()) > 0 && lso->lastPacketCount < (uint32_t) packetCount->value())
				currLossRate = ((double) ((uint32_t) lossCount->value()) - lso->lastLossCount) / ((double) ((uint32_t) packetCount->value()) - lso->lastPacketCount);

			lossRate->set(0.9 * currLossRate + 0.1 * lossRate->value());

			lso->lastPacketCount = packetCount->value();
			lso->lastLossCount = lossCount->value();

//			DBG_DEBUG(FMT("%1%: updating remote loss info (packetCount %2%, lossCount %3%, ratio %4%)") %
//					getId() % info.packetCount % info.lossCount % ((double) info.lossCount / (info.packetCount + info.lossCount)));

		}

		if (pkt->size() > 0) {
			pkt->setFrom(this);
			pkt->setTo(getPrev());
			sendMessage(pkt);

		} else {
			msg->getFlowState()->decInFloatingPackets();

		}

	} else {
		// forward end-of-stream marker
//		DBG_DEBUG(FMT("%1%(%2%) forwarding incoming end-of-stream marker") % getId() % pkt->getFlowState()->getFlowId());
		pkt->setFrom(this);
		pkt->setTo(getPrev());
		sendMessage(pkt);

	}
}

const std::string & Bb_LossSig::getId() const
{
	return BB_LOSSSIG_ID;
}

}
}
}
