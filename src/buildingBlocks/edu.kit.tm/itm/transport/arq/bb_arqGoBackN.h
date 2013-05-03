/*
 * bb_arqGoBackN.h
 *
 *  Created on: Jan 29, 2010
 *      Author: denis
 */

#ifndef BB_ARQGOBACKN_H_
#define BB_ARQGOBACKN_H_

#include "composableNetlet.h"
#include "mutexes.h"

#include "messages.h"
#include "messageBuffer.h"
#include "flowState.h"

#include <boost/shared_ptr.hpp>

namespace edu_kit_tm {
namespace itm {
namespace transport {

static const std::string BB_ARQGOBACKN_ID = "bb://edu.kit.tm/itm/transport/arq/goBackN";

class Bb_ArqGoBackN: public IBuildingBlock
{
public:
	typedef ushort SeqNo;

	typedef enum {
		mt_data = 0,
		mt_ack = 1,
		mt_syn = 2,
		mt_fin = 3,
		mt_rst = 4
	} MsgType;

	typedef enum {
		cs_none,
		cs_syn,
		cs_fin,
		cs_rdy,
		cs_error
	} ConnState;

private:
	class AckTimer;
	class RetrTimer;

	class GoBackNStateObject : public CFlowState::StateObject
	{
	public:
		// out bound state

		SeqNo windowSize; // N
		SeqNo seqNoAcked; // last ack'ed sequence number
		SeqNo localSeqNo; // next sequence number for outgoing packets
		ConnState localState;

		std::list<boost::shared_ptr<CMessageBuffer> > sendBuffer; // send queue
		std::list<boost::shared_ptr<CMessageBuffer> > retrBuffer; // retransmission buffer (flying packets)

		boost::shared_ptr<RetrTimer> lastRetrTimer;
		double retrTimeoutStart;
		double nextRetrTimeout;
		unsigned int retrCount;

		// in bound state

		SeqNo remoteSeqNo; // next expected sequence number
		ConnState remoteState;
		boost::shared_ptr<AckTimer> lastAckTimer;

		GoBackNStateObject() : CFlowState::StateObject(), retrTimeoutStart(0), retrCount(0)
		{
			setId(BB_ARQGOBACKN_ID);
		}

		virtual ~GoBackNStateObject()
		{}

		void cleanUp()
		{
			sendBuffer.clear();
			retrBuffer.clear();
			lastRetrTimer.reset();
			lastRetrTimer.reset();
		}
	};

	/**
	 * @brief ACK timer for delayed ACKs
	 *
	 * Single shot timer.
	 */
	class AckTimer : public CTimer
	{
	public:
		bool isCancled;
		boost::shared_ptr<CFlowState> flowState;
		boost::shared_ptr<GoBackNStateObject> so;

		AckTimer(IMessageProcessor *proc, boost::shared_ptr<CFlowState> flowState,
				boost::shared_ptr<GoBackNStateObject> so,
				double delay = 0.1)
			: CTimer(delay, proc), isCancled(false), flowState(flowState), so(so) {}
		virtual ~AckTimer() {}
	};

	/**
	 * @brief Retransmission timer for retransmission timeout
	 *
	 * Single shot timer.
	 */
	class RetrTimer : public CTimer
	{
	public:
		bool isCancled;
		boost::shared_ptr<CFlowState> flowState;
		boost::shared_ptr<GoBackNStateObject> so;

		RetrTimer(IMessageProcessor *proc, boost::shared_ptr<CFlowState> flowState,
				boost::shared_ptr<GoBackNStateObject> so,
				double delay = 0.2)
			: CTimer(delay, proc), isCancled(false), flowState(flowState), so(so) {}
		virtual ~RetrTimer() {}
	};

	void sendNextPackets(boost::shared_ptr<CFlowState> flowState, boost::shared_ptr<GoBackNStateObject> so);
	void retransmitPackets(boost::shared_ptr<CFlowState> flowState, boost::shared_ptr<GoBackNStateObject> so);

	boost::shared_ptr<CMessageBuffer> createCtrlPacket(boost::shared_ptr<CFlowState> flowState,
			boost::shared_ptr<GoBackNStateObject> so, MsgType msgType, SeqNo seqNo);

public:
	Bb_ArqGoBackN(CNena *nena, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~Bb_ArqGoBackN();

	// from IMessageProcessor

	/**
	 * @brief Process an event message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	 * @brief Process a timer message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	 * @brief Process an outgoing message directed towards the network.
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

	// from IBuildingBlock

	virtual const std::string & getId() const;
};

} // transport
} // itm
} // edu.kit.tm

#endif /* BB_ARQGOBACKN_H_ */
