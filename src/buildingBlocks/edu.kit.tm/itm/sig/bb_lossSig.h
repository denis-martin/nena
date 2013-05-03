/*
 * bb_lossSig.h
 *
 * Signals current packet loss rate back to sender.
 *
 *  Created on: Jan 29, 2010
 *      Author: denis
 */

#ifndef BB_LOSSSIG_H_
#define BB_LOSSSIG_H_

#include "composableNetlet.h"
#include "mutexes.h"

#include "messages.h"
#include "messageBuffer.h"
#include "flowState.h"
#include "appConnector.h"

#include <boost/shared_ptr.hpp>

#define BB_LOSSSIG_SIGTIMER	1.0

namespace edu_kit_tm {
namespace itm {
namespace sig {

static const std::string BB_LOSSSIG_ID = "bb://edu.kit.tm/itm/sig/lossSig";

class Bb_LossSig: public IBuildingBlock
{
private:
	class LossSigStateObject : public CFlowState::StateObject
	{
	public:
		ushort lastSeqNoSent;
		ushort lastSeqNoRcvd;
		bool recvSeqNoValid;

		uint32_t lastPacketCount;
		uint32_t lastLossCount;
		double currLossRate;

		LossSigStateObject() : CFlowState::StateObject(),
				lastSeqNoSent(0), lastSeqNoRcvd(0), recvSeqNoValid(false),
				lastPacketCount(0), lastLossCount(0), currLossRate(0)
		{
			setId(BB_LOSSSIG_ID);
		}

		virtual ~LossSigStateObject()
		{}
	};

	class SigTimer : public CTimer
	{
	public:
		boost::shared_ptr<CFlowState> flowState;

		SigTimer(IMessageProcessor* proc, boost::shared_ptr<CFlowState> flowState) :
			CTimer(BB_LOSSSIG_SIGTIMER, proc), flowState(flowState) {};

		virtual ~SigTimer() {};
	};

public:
	Bb_LossSig(CNena *nena, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~Bb_LossSig();

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

} // sig
} // itm
} // edu.kit.tm

#endif /* BB_LOSSSIG_H_ */
