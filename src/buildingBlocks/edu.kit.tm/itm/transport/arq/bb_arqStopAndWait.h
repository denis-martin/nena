/*
 * bb_arqStopAndWait.h
 *
 *  Created on: Jan 29, 2010
 *      Author: denis
 */

#ifndef BB_ARQSTOPANDWAIT_H_
#define BB_ARQSTOPANDWAIT_H_

#include "composableNetlet.h"
#include "mutexes.h"

#include "messageBuffer.h"

#include <boost/shared_ptr.hpp>

namespace edu_kit_tm {
namespace itm {
namespace transport {

extern const std::string arqStopAndWaitClassName;

class Bb_ArqStopAndWait: public IBuildingBlock
{
private:
	class QueueItem
	{
	public:
		ushort seqNo;
		boost::shared_ptr<CMessageBuffer> pkt;
		int retries;

		QueueItem() : seqNo(0), retries(0) {}
		QueueItem(ushort seqNo, boost::shared_ptr<CMessageBuffer> pkt) : seqNo(seqNo), pkt(pkt), retries(0) {}
		QueueItem (const QueueItem & rhs)
		{
			seqNo = rhs.seqNo;
			pkt = rhs.pkt;
			retries = rhs.retries;
		}
		QueueItem & operator=(const QueueItem & rhs)
		{
			if (this != &rhs)
			{
				seqNo = rhs.seqNo;
				pkt = rhs.pkt;
				retries = rhs.retries;
			}
			return *this;
		}

		virtual ~QueueItem() {}
	};

	/**
	 * @brief ACK timer.
	 *
	 * Single shot timer.
	 */
	class AckTimer : public CTimer
	{
	public:
		bool isCancled;

		AckTimer(IMessageProcessor *proc) : CTimer(0.5, proc), isCancled(false) {}
		virtual ~AckTimer() {}
	};

	boost::shared_ptr<IMutex> seqNoMutex;
	ushort localSeqNo;
	boost::shared_ptr<IMutex> queueMutex;
	std::list<QueueItem> outgoingQueue;
	boost::shared_ptr<AckTimer> lastTimer;

	void sendNextPacket();

public:
	Bb_ArqStopAndWait(CNena *nena, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~Bb_ArqStopAndWait();

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
};

} // transport
} // itm
} // edu.kit.tm

#endif /* BB_ARQSTOPANDWAIT_H_ */
