/*
 * bb_arqGoBackN.cpp
 *
 *  Created on: Jan 29, 2010
 *      Author: denis
 */

#include "bb_arqGoBackN.h"

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

#define SEQNO_BITS				16
#define SEQNO_INC(seqNo)		seqNo = (seqNo + 1) % (1 << SEQNO_BITS)
#define SEQNO_ISGT(a, b)		((int) a - (int) b < -1*(1 << (SEQNO_BITS-1)) ? true : ((int) a - (int) b > 0 && (int) a - (int) b < (1 << (SEQNO_BITS-1))))
#define SEQNO_DIFF(low, high)	((high >= low) ? (high - low) : ((1 << SEQNO_BITS) - low + high))

// retransmission retries and timeout
#define RETRYLIMIT			10
#define RETRYTIMEOUT		0.2

// default window size (N)
#define WINDOW_SIZE			128

class ArqGoBackNHeader : public IHeader
{
public:
	Bb_ArqGoBackN::SeqNo seqNo;
	Bb_ArqGoBackN::MsgType msgType;

	ArqGoBackNHeader(Bb_ArqGoBackN::SeqNo seqNo = 0, Bb_ArqGoBackN::MsgType msgType = Bb_ArqGoBackN::mt_data) :
		seqNo(seqNo), msgType(msgType) {}
	virtual ~ArqGoBackNHeader() {}

	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(sizeof(ushort) + sizeof(unsigned char)));
		buffer->push_ushort(seqNo);
		buffer->push_uchar(msgType);
		return buffer;
	}

	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer)
	{
		seqNo = buffer->pop_ushort();
		msgType = (Bb_ArqGoBackN::MsgType) buffer->pop_uchar();
	}
};

Bb_ArqGoBackN::Bb_ArqGoBackN(CNena *nena, IMessageScheduler *sched, IComposableNetlet *netlet, const string id)
	: IBuildingBlock(nena, sched, netlet, id)
{
	className += "::Bb_ArqGoBackN";
	setId(BB_ARQGOBACKN_ID);

	// random initialization
	//localSeqNo = (ushort) (nodeArch->getSys()->random() * (1 << SEQNO_BITS) - 1);
}

Bb_ArqGoBackN::~Bb_ArqGoBackN()
{
}

shared_ptr<CMessageBuffer> Bb_ArqGoBackN::createCtrlPacket(shared_ptr<CFlowState> flowState,
		shared_ptr<GoBackNStateObject> so, MsgType msgType, SeqNo seqNo)
{
	ArqGoBackNHeader hdr(seqNo, msgType);
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
void Bb_ArqGoBackN::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<IEvent> ev = msg->cast<IEvent>();
	assert(ev.get());

	if (ev->getId() == FLOWSTATE_NOTIFICATION_EVENT) {
		shared_ptr<CFlowState::Notification> notif = msg->cast<CFlowState::Notification>();
		if (notif->event == CFlowState::ev_stateChanged) {
			switch (notif->flowState->getOperationalState()) {
			case CFlowState::s_end: {
//				DBG_DEBUG(FMT("%1% flow state end event") % getId());
				break;
			}
			case CFlowState::s_stale: {
//				DBG_DEBUG(FMT("%1% flow state stale event") % getId());
				shared_ptr<GoBackNStateObject> myso;
				shared_ptr<CFlowState::StateObject> so = notif->flowState->getStateObject(getId());
				if (so.get()) {
					// existing state
					myso = so->cast<GoBackNStateObject>();
					if (myso->localState == cs_syn ||
						myso->localState == cs_rdy ||
						myso->remoteState == cs_rdy)
					{
						DBG_DEBUG(FMT("%1%(%2%) flow state stale event, sending rst") % getId() % notif->flowState->getFlowId());
						sendMessage(createCtrlPacket(notif->flowState, shared_ptr<GoBackNStateObject>(), mt_rst, 0));
						myso->localState = cs_error;
						myso->remoteState = cs_error;
						myso->cleanUp();
					}
				}
				break;
			}
			default: {
				// nothing
				break;
			}
			}

		}

	} else {
		string m = (FMT("%1%: unhandled event %2%") % getId() % ev->getId()).str();
		DBG_ERROR(m);
		throw EUnhandledMessage(m);

	}
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_ArqGoBackN::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<AckTimer> ackTimer = msg->cast<AckTimer>();
	if (ackTimer.get() != NULL) {
		ackTimer->so->lastAckTimer.reset();
		// send ack
		sendMessage(createCtrlPacket(ackTimer->flowState, ackTimer->so, mt_ack, ackTimer->so->remoteSeqNo));
		return;
	}

	shared_ptr<RetrTimer> retrTimer = msg->cast<RetrTimer>();
	if (retrTimer.get() != NULL) {
		retrTimer->so->lastRetrTimer.reset();

//		DBG_DEBUG(FMT("%1% retransmission timer fired (flow id %2%)") % getId() % retrTimer->flowState->getFlowId());

		if (retrTimer->so->localState != cs_none && retrTimer->so->localState != cs_error) {
			assert(retrTimer->so->retrTimeoutStart > 0);
			if ((nena->getSysTime() - retrTimer->so->retrTimeoutStart) > retrTimer->so->nextRetrTimeout) {
				// retransmission
				retransmitPackets(retrTimer->flowState, retrTimer->so);
				retrTimer->so->retrTimeoutStart = nena->getSysTime();
			}

			// timer restart if necessary
			if ((retrTimer->so->lastRetrTimer.get() == NULL) && (retrTimer->so->retrBuffer.size() > 0)) {
				retrTimer->so->lastRetrTimer = shared_ptr<RetrTimer>(new RetrTimer(this, retrTimer->flowState, retrTimer->so));
				scheduler->setTimer(retrTimer->so->lastRetrTimer);

			}

//			if (retrTimer->so->retrBuffer.size() == 0 && retrTimer->so->sendBuffer.size() == 0) {
//				DBG_DEBUG(FMT("%1%(%2%): no more packets in queues (flowState outFloatingPackets %3%)") %
//						getId() % retrTimer->flowState->getFlowId() %
//						retrTimer->flowState->getOutFloatingPackets());
//			}

		}

		return;
	}

	throw EUnhandledMessage("Unhandled timer!");
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void Bb_ArqGoBackN::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	assert(pkt.get() != NULL);
	
	shared_ptr<CFlowState> flowState = pkt->getFlowState();
	assert(flowState.get() != NULL);

	bool endOfStream = false;
	try {
		endOfStream = msg->getProperty<CBoolValue>(IMessage::p_endOfStream)->value();
	} catch (...) {
		// nothing
	}

	shared_ptr<GoBackNStateObject> myso;
	shared_ptr<CFlowState::StateObject> so = flowState->getStateObject(getId());
	if (so.get()) {
		// existing state
		myso = so->cast<GoBackNStateObject>();

		if ((myso->localState == cs_none) && !endOfStream) {
			myso->localState = cs_syn;
			flowState->setOutMaxFloatingPackets(myso->windowSize);

			boost::shared_ptr<CMessageBuffer> syn = createCtrlPacket(flowState, myso, mt_syn, myso->localSeqNo);
			myso->retrBuffer.push_back(syn->clone());
			SEQNO_INC(myso->localSeqNo);

			FLOWSTATE_FLOATOUT_INC(flowState, 1, "gobackn:syn", nena->getSysTime());
			sendMessage(syn);

			assert(myso->lastRetrTimer.get() == NULL);
			myso->retrCount = 0;
			myso->nextRetrTimeout = RETRYTIMEOUT;
			myso->retrTimeoutStart = nena->getSysTime();
			myso->lastRetrTimer = shared_ptr<RetrTimer>(new RetrTimer(this, flowState, myso));
			scheduler->setTimer(myso->lastRetrTimer);

			DBG_DEBUG(FMT("%1%(%2%): sent syn to %3%") % getId() % flowState->getFlowId() % flowState->getRemoteId());

		}

	} else if (!endOfStream) {
		// new state
		myso = shared_ptr<GoBackNStateObject>(new GoBackNStateObject());
		myso->windowSize = WINDOW_SIZE;
		myso->seqNoAcked = (ushort) (nena->getSys()->random() * (1 << SEQNO_BITS) - 1);
		myso->localSeqNo = myso->seqNoAcked + 1;
		myso->localState = cs_syn;
		myso->remoteState = cs_none;
		flowState->addStateObject(myso->getId(), myso);
		flowState->registerListener(this);
		flowState->setOutMaxFloatingPackets(myso->windowSize);

		ArqGoBackNHeader hdr(myso->localSeqNo, mt_syn);

		boost::shared_ptr<CMessageBuffer> syn = createCtrlPacket(flowState, myso, mt_syn, myso->localSeqNo);
		myso->retrBuffer.push_back(syn->clone());
		SEQNO_INC(myso->localSeqNo);

		FLOWSTATE_FLOATOUT_INC(flowState, 1, "gobackn:syn", nena->getSysTime());
		sendMessage(syn);

		assert(myso->lastRetrTimer.get() == NULL);
		myso->retrCount = 0;
		myso->nextRetrTimeout = RETRYTIMEOUT;
		myso->retrTimeoutStart = nena->getSysTime();
		myso->lastRetrTimer = shared_ptr<RetrTimer>(new RetrTimer(this, flowState, myso));
		scheduler->setTimer(myso->lastRetrTimer);

		DBG_DEBUG(FMT("%1%(%2%): sent syn to %3%") % getId() % flowState->getFlowId() % flowState->getRemoteId());
	}

	if (myso != NULL) {
		if (endOfStream) {
			if (myso->localState == cs_rdy || myso->localState == cs_syn) {
				// send fin
				myso->localState = cs_fin;
				shared_ptr<CMessageBuffer> fin = createCtrlPacket(flowState, myso, mt_fin, myso->localSeqNo);
				SEQNO_INC(myso->localSeqNo);
				flowState->incOutFloatingPackets();
				if ((myso->retrBuffer.size() < myso->windowSize) &&
					myso->sendBuffer.empty())
				{
					shared_ptr<CMessageBuffer> clone(fin->clone());
					myso->retrBuffer.push_back(clone);
					flowState->incOutFloatingPackets();
					sendMessage(fin); // send immediately

					if (myso->lastRetrTimer.get() == NULL) {
						myso->retrCount = 0;
						myso->nextRetrTimeout = RETRYTIMEOUT;
						myso->retrTimeoutStart = nena->getSysTime();
						myso->lastRetrTimer = shared_ptr<RetrTimer>(new RetrTimer(this, flowState, myso));
						scheduler->setTimer(myso->lastRetrTimer);

					}

					DBG_DEBUG(FMT("%1%(%2%): sent fin directly") % getId() % flowState->getFlowId());

				} else {
					myso->sendBuffer.push_back(fin);
					sendNextPackets(flowState, myso);

					DBG_DEBUG(FMT("%1%(%2%): sent fin to send queue") % getId() % flowState->getFlowId());

				}

			} else {
//				DBG_DEBUG(FMT("%1%(%2%) fowarding outgoing EoS marker") % getId() % flowState->getFlowId());

				// forward end-of-stream marker (should not contain data)
				pkt->setFrom(this);
				pkt->setTo(getNext());
				sendMessage(pkt);

			}

		} else {
			shared_ptr<ArqGoBackNHeader> hdr(new ArqGoBackNHeader(myso->localSeqNo));
			SEQNO_INC(myso->localSeqNo);
			pkt->push_header(hdr);
			pkt->setFrom(this);
			pkt->setTo(getNext());

			if ((myso->retrBuffer.size() < myso->windowSize) &&
				myso->sendBuffer.empty() &&
				(myso->localState == cs_rdy))
			{
				shared_ptr<CMessageBuffer> clone(pkt->clone()); // payload is not copied
				myso->retrBuffer.push_back(clone);
				FLOWSTATE_FLOATOUT_INC(flowState, 1, "gobackn:senddirectly", nena->getSysTime());
				sendMessage(pkt); // send immediately

				if (myso->lastRetrTimer.get() == NULL) {
					myso->retrCount = 0;
					myso->nextRetrTimeout = RETRYTIMEOUT;
					myso->retrTimeoutStart = nena->getSysTime();
					myso->lastRetrTimer = shared_ptr<RetrTimer>(new RetrTimer(this, flowState, myso));
					scheduler->setTimer(myso->lastRetrTimer);

				}

//				DBG_DEBUG(FMT("%1% sent directly") % getId());

			} else {
				myso->sendBuffer.push_back(pkt);
				sendNextPackets(flowState, myso);

//				DBG_DEBUG(FMT("%1% sent to send queue") % getId());

			}

		}

	} else {
		throw EUnhandledMessage((FMT("%1% could not determine valid state for outgoing message") % getId()).str());

	}
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void Bb_ArqGoBackN::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	assert(pkt.get() != NULL);

	ArqGoBackNHeader hdr;
	pkt->pop_header(hdr);

	switch (hdr.msgType) {
	case mt_data: {
		shared_ptr<CFlowState> flowState = msg->getFlowState();
		shared_ptr<CFlowState::StateObject> so = flowState->getStateObject(getId());
		if (so.get() == NULL)
			throw EUnhandledMessage((FMT("%1%(%2%): data for unknown flow state, dropping") % getId() % flowState->getFlowId()).str());

		shared_ptr<GoBackNStateObject> myso = so->cast<GoBackNStateObject>();

//		DBG_DEBUG(FMT("%1%(2%): got data") % getId() % flowState->getFlowId());

		if ((flowState->getOperationalState() != CFlowState::s_valid) || (myso->remoteState != cs_rdy)) {
			if (myso->remoteState == cs_error) {
				sendMessage(createCtrlPacket(flowState, shared_ptr<GoBackNStateObject>(), mt_rst, 0));
				throw EUnhandledMessage((FMT("%1%(%2%): data for erroneous flow state, sending rst") % getId() % flowState->getFlowId()).str());

			} else {
				if (flowState->getOperationalState() == CFlowState::s_end) {
					sendMessage(createCtrlPacket(flowState, shared_ptr<GoBackNStateObject>(), mt_rst, 0));
					throw EUnhandledMessage((FMT("%1%(%2%): data for ended flow state, sending rst") % getId() % flowState->getFlowId()).str());

				} else {
					throw EUnhandledMessage((FMT("%1%(%2%): data for invalid flow state, dropping") % getId() % flowState->getFlowId()).str());

				}

			}
		}

		if (SEQNO_ISGT(hdr.seqNo, myso->remoteSeqNo)) {
			if (SEQNO_DIFF(myso->remoteSeqNo, hdr.seqNo) == 1) {
				pkt->setFrom(this);
				pkt->setTo(getPrev());
				sendMessage(pkt);

				myso->remoteSeqNo = hdr.seqNo;

			} else {
				// drop
				flowState->decInFloatingPackets();
				DBG_INFO(FMT("%1%(%2%): dropping future data (pkt seqNo %3%, current seqNo %4%)") %
						getId() % flowState->getFlowId() % hdr.seqNo % myso->remoteSeqNo);

			}

		} else {
			// drop
			flowState->decInFloatingPackets();
			DBG_INFO(FMT("%1%(%2%): dropping duplicate data (pkt seqNo %3%, current seqNo %4%)") %
					getId() % flowState->getFlowId() % hdr.seqNo % myso->remoteSeqNo);

		}

		if (myso->lastAckTimer.get() == NULL) {
			myso->lastAckTimer = shared_ptr<AckTimer>(new AckTimer(this, flowState, myso));
			scheduler->setTimer<AckTimer>(myso->lastAckTimer);
		}

		break;
	}
	case mt_ack: {
		shared_ptr<CFlowState> flowState = msg->getFlowState();
		shared_ptr<CFlowState::StateObject> so = flowState->getStateObject(getId());
		flowState->decInFloatingPackets();

		if (so.get() == NULL)
			throw EUnhandledMessage((FMT("%1%(%2%): ack for unknown flow state, dropping") % getId() % flowState->getFlowId()).str());

		shared_ptr<GoBackNStateObject> myso = so->cast<GoBackNStateObject>();

		assert(myso->localState != cs_none);

		if (myso->localState == cs_error) {
			sendMessage(createCtrlPacket(flowState, shared_ptr<GoBackNStateObject>(), mt_rst, 0));
			throw EUnhandledMessage((FMT("%1%(%2%): ack for erroneous flow state, sending rst") % getId() % flowState->getFlowId()).str());

		}

		if (SEQNO_ISGT(hdr.seqNo, myso->seqNoAcked)) {
			string acktype = "ack";
			if (myso->localState == cs_syn)
				acktype = "syn-ack";
			else if (myso->localState == cs_fin && hdr.seqNo == (SeqNo) (myso->localSeqNo - 1))
				acktype = "fin-ack";

			if (myso->localState == cs_syn)
				myso->localState = cs_rdy;

			unsigned int diff = SEQNO_DIFF(myso->seqNoAcked, hdr.seqNo);
			if (acktype != "ack")
				DBG_DEBUG(FMT("%1%(%2%): got %3% (%4% packets acked: %5%-%6%)") %
						getId() % flowState->getFlowId() % acktype % diff % myso->seqNoAcked % hdr.seqNo);

			myso->retrCount = 0;
			myso->nextRetrTimeout = RETRYTIMEOUT;
			myso->retrTimeoutStart = nena->getSysTime();

			while (SEQNO_ISGT(hdr.seqNo, myso->seqNoAcked)) {
				SEQNO_INC(myso->seqNoAcked);
				myso->retrBuffer.pop_front();
			}
			FLOWSTATE_FLOATOUT_DEC(flowState, 0, "gobackn:ack", nena->getSysTime()); // get current time
			FLOWSTATE_FLOATOUT_DEC(flowState, diff, "gobackn:ack", nena->getSysTime());

//			if (myso->localState == cs_fin && acktype == "fin-ack") {
//				assert(myso->sendBuffer.empty());
//			}

			sendNextPackets(flowState, myso);

		} else {
			// duplicate ack
			DBG_DEBUG(FMT("%1%(%2%): got duplicate ack (rcvd ack %3%, seqNoAcked %4%)") %
					getId() % flowState->getFlowId() % myso->seqNoAcked % hdr.seqNo);
			retransmitPackets(flowState, myso);

		}

		break;
	}
	case mt_syn: {
		shared_ptr<GoBackNStateObject> myso;
		shared_ptr<CFlowState::StateObject> so = msg->getFlowState()->getStateObject(getId());
		if (so.get()) {
			// existing state
			myso = so->cast<GoBackNStateObject>();

			if (myso->remoteState == cs_none) {
				myso->remoteSeqNo = hdr.seqNo;
				myso->remoteState = cs_rdy;

				DBG_DEBUG(FMT("%1%(%2%): got syn") % getId() % msg->getFlowState()->getFlowId());

				// send immediate ACK
				sendMessage(createCtrlPacket(msg->getFlowState(), myso, mt_ack, hdr.seqNo));

			} else if ((myso->remoteState == cs_rdy || myso->remoteState == cs_fin) &&
				(myso->remoteSeqNo == hdr.seqNo || SEQNO_ISGT(myso->remoteSeqNo, hdr.seqNo)))
			{
				DBG_DEBUG(FMT("%1%(%2%): got duplicate syn") % getId() % msg->getFlowState()->getFlowId());

			} else {
				DBG_ERROR(FMT("%1%(%2%): got syn with higher sequence number, sending rst!") % getId() % msg->getFlowState()->getFlowId());
				sendMessage(createCtrlPacket(msg->getFlowState(), shared_ptr<GoBackNStateObject>(), mt_rst, 0));

			}

		} else {
			// new state
			myso = shared_ptr<GoBackNStateObject>(new GoBackNStateObject());
			myso->windowSize = WINDOW_SIZE;
			myso->seqNoAcked = (ushort) (nena->getSys()->random() * (1 << SEQNO_BITS) - 1);
			myso->localSeqNo = myso->seqNoAcked + 1;
			myso->localState = cs_none;
			myso->remoteSeqNo = hdr.seqNo;
			myso->remoteState = cs_rdy;
			msg->getFlowState()->addStateObject(myso->getId(), myso);
			msg->getFlowState()->registerListener(this);

			DBG_DEBUG(FMT("%1%(%2%): got syn") % getId() % msg->getFlowState()->getFlowId());

			// send immediate ACK
			sendMessage(createCtrlPacket(msg->getFlowState(), myso, mt_ack, hdr.seqNo));

		}
		break;
	}
	case mt_fin: {
		shared_ptr<CFlowState> flowState = msg->getFlowState();
		shared_ptr<GoBackNStateObject> myso;
		shared_ptr<CFlowState::StateObject> so = flowState->getStateObject(getId());
		if (so.get()) {
			// existing state
			shared_ptr<GoBackNStateObject> myso = so->cast<GoBackNStateObject>();
			DBG_DEBUG(FMT("%1%(%2%): got fin") % getId() % flowState->getFlowId());

			if (SEQNO_ISGT(hdr.seqNo, myso->remoteSeqNo)) {
				if (SEQNO_DIFF(myso->remoteSeqNo, hdr.seqNo) == 1) {
					myso->remoteSeqNo = hdr.seqNo;
					myso->remoteState = cs_fin;

					if (flowState->getOperationalState() == CFlowState::s_valid) {
						// re-use packet as end-of-stream marker
//						DBG_DEBUG(FMT("%1%(%2%): generating end of stream marker") % getId() % flowState->getFlowId());
						pkt->setProperty(IMessage::p_endOfStream, new CBoolValue(true));
						pkt->setFrom(this);
						pkt->setTo(getPrev());
						sendMessage(pkt);

					} else {
						// flow already shutting down, consume fin
						flowState->decInFloatingPackets();

					}

				} else {
					// drop
					flowState->decInFloatingPackets();
					DBG_DEBUG(FMT("%1%(%2%): dropping future fin (pkt seqNo %3%, current seqNo %4%)") %
							getId() % flowState->getFlowId() % hdr.seqNo % myso->remoteSeqNo);

				}

			} else {
				// drop
				flowState->decInFloatingPackets();
				DBG_DEBUG(FMT("%1%(%2%): dropping duplicate fin (pkt seqNo %3%, current seqNo %4%)") %
						getId() % flowState->getFlowId() % hdr.seqNo % myso->remoteSeqNo);

			}

			if (myso->lastAckTimer.get() == NULL) {
				myso->lastAckTimer = shared_ptr<AckTimer>(new AckTimer(this, flowState, myso));
				scheduler->setTimer<AckTimer>(myso->lastAckTimer);
			}

		} else {
			DBG_DEBUG(FMT("%1%(%2%): got fin for unknown flow, sending rst") % getId() % flowState->getFlowId());
			sendMessage(createCtrlPacket(msg->getFlowState(), shared_ptr<GoBackNStateObject>(), mt_rst, 0));

		}
		break;
	}
	case mt_rst: {
		DBG_DEBUG(FMT("%1%(%2%): got rst") % getId() % msg->getFlowState()->getFlowId());
		shared_ptr<CFlowState> flowState = msg->getFlowState();
		shared_ptr<GoBackNStateObject> myso;
		shared_ptr<CFlowState::StateObject> so = flowState->getStateObject(getId());
		if (so.get()) {
			myso = so->cast<GoBackNStateObject>();
			myso->localState = cs_error;
			myso->remoteState = cs_error;
			flowState->decOutFloatingPackets(myso->sendBuffer.size());
			flowState->decOutFloatingPackets(myso->retrBuffer.size());
			myso->sendBuffer.clear();
			myso->retrBuffer.clear();
		}
		flowState->setErrorState(this, CFlowState::e_reset);
		flowState->setOperationalState(this, CFlowState::s_stale);
		break;
	}
	default: {
		DBG_ERROR(FMT("%1%: receive unknown message type") % getId());
		throw EUnhandledMessage();
		break;
	}
	}
}

const std::string & Bb_ArqGoBackN::getId() const
{
	return BB_ARQGOBACKN_ID;
}

void Bb_ArqGoBackN::sendNextPackets(boost::shared_ptr<CFlowState> flowState, boost::shared_ptr<GoBackNStateObject> so)
{
	unsigned int i = 0;
	while ((so->retrBuffer.size() < so->windowSize) && // flight size
			(so->sendBuffer.size() > 0) &&
			(so->localState == cs_rdy || so->localState == cs_syn || so->localState == cs_fin))
	{
		shared_ptr<IMessage> msg = so->sendBuffer.front()->clone();
		sendMessage(msg);

		so->retrBuffer.push_back(so->sendBuffer.front());
		so->sendBuffer.pop_front();

		i++;
	}

	FLOWSTATE_FLOATOUT_INC(flowState, i, "gobackn:sendNextPackets", nena->getSysTime());

	if (i > 1) {
		if (so->lastRetrTimer.get() == NULL) {
			so->retrCount = 0;
			so->nextRetrTimeout = RETRYTIMEOUT;
			so->retrTimeoutStart = nena->getSysTime();
			so->lastRetrTimer = shared_ptr<RetrTimer>(new RetrTimer(this, flowState, so));
			scheduler->setTimer(so->lastRetrTimer);

		}

	}

//	if (i > 1)
//		DBG_DEBUG(FMT("%1%: sent %2% packets from sendBuffer (size %3%)") % getId() % i % so->sendBuffer.size());

	flowState->notify(this, CFlowState::ev_flowControl);
}

void Bb_ArqGoBackN::retransmitPackets(boost::shared_ptr<CFlowState> flowState, boost::shared_ptr<GoBackNStateObject> so)
{
	if (so->retrCount < RETRYLIMIT) {
		unsigned int i = 0;
		list<shared_ptr<CMessageBuffer> >::iterator it = so->retrBuffer.begin();
		while (it != so->retrBuffer.end() &&
				(so->localState == cs_rdy || so->localState == cs_syn || so->localState == cs_fin))
		{
			shared_ptr<IMessage> msg = it->get()->clone();
			sendMessage(msg);
			it++;
			i++;
		}

		if (i > 0) {
			FLOWSTATE_FLOATOUT_INC(flowState, i, "gobackn:retr", nena->getSysTime());

			DBG_DEBUG(FMT("%1%(%2%): resent %3% packets from retrBuffer") %
					getId() % flowState->getFlowId() % i);

			so->retrCount++;
			so->retrTimeoutStart = nena->getSysTime();
			so->nextRetrTimeout = RETRYTIMEOUT * so->retrCount * so->retrCount;
			if (so->lastRetrTimer.get() == NULL) {
				so->lastRetrTimer = shared_ptr<RetrTimer>(new RetrTimer(this, flowState, so));
				scheduler->setTimer(so->lastRetrTimer);

			}
		}

	} else {
		DBG_DEBUG(FMT("%1%(%2%): retry-limit exceeded") % getId() % flowState->getFlowId());
		flowState->decOutFloatingPackets(so->sendBuffer.size());
		flowState->decOutFloatingPackets(so->retrBuffer.size());
		so->sendBuffer.clear();
		so->retrBuffer.clear();
		so->localState = cs_error;
		flowState->setErrorState(this, CFlowState::e_reset);
		flowState->setOperationalState(this, CFlowState::s_stale);

	}
}

}
}
}
