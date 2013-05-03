/*
 * bb_simpleSmooth.h
 *
 *  Created on: Jan 29, 2010
 *      Author: denis
 */

#ifndef BB_SIMPLESMOOTH_H_
#define BB_SIMPLESMOOTH_H_

#include "composableNetlet.h"

#include "messageBuffer.h"

namespace edu_kit_tm {
namespace itm {
namespace transport {

extern const std::string simpleSmoothClassName;

class Bb_SimpleSmooth: public IBuildingBlock
{
private:
	/**
	 * Just in case we need further information per packet later on.
	 */
	class QueueItem
	{
	public:
		boost::shared_ptr<CMessageBuffer> pkt;

		QueueItem() {}
		QueueItem(boost::shared_ptr<CMessageBuffer> pkt) : pkt(pkt) {}
		virtual ~QueueItem() {}
	};

	/**
	 * @brief Timer.
	 *
	 * Single shot timer.
	 */
	class Timer : public CTimer
	{
	public:
		bool isCancled;
		int step;
		int packetsPerStep;
		std::size_t queueSize;

		Timer(double timeout, IMessageProcessor *proc, int step = 0, int packetsPerStep = 0, int queueSize = 0)
			: CTimer(timeout, proc), isCancled(false), step(step), packetsPerStep(packetsPerStep), queueSize(queueSize)
		{}
		virtual ~Timer() {}
	};

	ushort localSeqNo;
	std::list<QueueItem> outgoingQueue;
	boost::shared_ptr<Timer> lastTimer;

	void sendNextPacket();

public:
	Bb_SimpleSmooth(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~Bb_SimpleSmooth();

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

#endif /* BB_SIMPLESMOOTH_H_ */
