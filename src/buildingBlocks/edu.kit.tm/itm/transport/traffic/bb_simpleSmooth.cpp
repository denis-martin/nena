/*
 * bb_simpleSmooth.cpp
 *
 *  Created on: Jan 29, 2010
 *      Author: denis
 */

#include "bb_simpleSmooth.h"

#include "nena.h"

namespace edu_kit_tm {
namespace itm {
namespace transport {

using std::string;
using std::list;
using boost::shared_ptr;

// TODO: need to generalise (config file?)
// milliseconds
#define BB_SIMPLESMOOTH_INITIALDELAY	0
#define BB_SIMPLESMOOTH_WINDOW			0.040
#define BB_SIMPLESMOOTH_STEPS			4
#define BB_SIMPLESMOOTH_STEPDELAY		(BB_SIMPLESMOOTH_WINDOW/BB_SIMPLESMOOTH_STEPS)

const std::string simpleSmoothClassName = "bb://edu.kit.tm/itm/transport/traffic/simpleSmooth";

Bb_SimpleSmooth::Bb_SimpleSmooth(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const string id)
	: IBuildingBlock(nodeArch, sched, netlet, id)
{
	className += "::Bb_SimpleSmooth";
}

Bb_SimpleSmooth::~Bb_SimpleSmooth()
{
	if (outgoingQueue.size() > 0)
		DBG_WARNING(FMT("%1% is left with %2% unsent packets") % getClassName() % outgoingQueue.size());

	outgoingQueue.clear();
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleSmooth::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleSmooth::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<Timer> timer = msg->cast<Timer>();
	if (timer == NULL) {
		DBG_ERROR("Unhandled timer!");
		throw EUnhandledMessage();

	}

	lastTimer.reset();

	if (outgoingQueue.size() > 0) {
		if (timer->packetsPerStep == 0) {
			// first occurrence in this series
			int pktsPerStep = outgoingQueue.size() / BB_SIMPLESMOOTH_STEPS;
			if (outgoingQueue.size() % BB_SIMPLESMOOTH_STEPS)
				pktsPerStep++;

			// send out first bunch of packets
			for (int i = 0; i < pktsPerStep && outgoingQueue.size() > 0; i++) {
				sendNextPacket();
			}

			// schedule next iteration
			if (outgoingQueue.size() > 0) {
				lastTimer = shared_ptr<Timer>(new Timer(BB_SIMPLESMOOTH_STEPDELAY, this, 1, pktsPerStep, outgoingQueue.size()));
				scheduler->setTimer<Timer>(lastTimer);
			}

//			if (pktsPerStep > 200)
//				DBG_DEBUG(FMT("WARNING smooth: 1. bunch (pktsPerStep %1%, queuesize %2%)")
//					% pktsPerStep % outgoingQueue.size());

//			DBG_DEBUG(FMT("smooth: 1. bunch (pktsPerStep %1%, queuesize %2%)")
//				% pktsPerStep % outgoingQueue.size());

		} else {
			// check whether the current queue size changed between calls
			bool restartTimer = outgoingQueue.size() != timer->queueSize;

			// send out next bunch of packets
			for (int i = 0; i < timer->packetsPerStep && outgoingQueue.size() > 0; i++) {
				sendNextPacket();
			}

			timer->step++;

//			DBG_DEBUG(FMT("smooth: %3%. bunch (pktsPerStep %1%, queuesize %2%)")
//				% timer->packetsPerStep % outgoingQueue.size() % timer->step);

			if (outgoingQueue.size() > 0) {
				if (timer->step < BB_SIMPLESMOOTH_STEPS && !restartTimer) {
					// schedule next iteration
					lastTimer = shared_ptr<Timer>(new Timer(BB_SIMPLESMOOTH_STEPDELAY, this, timer->step, timer->packetsPerStep, outgoingQueue.size()));
					scheduler->setTimer(lastTimer);

				} else {
					// this series is over, so schedule next series
					// schedule an immediate timer (well, almost immediately)
					lastTimer = shared_ptr<Timer>(new Timer(BB_SIMPLESMOOTH_INITIALDELAY, this));
					if (BB_SIMPLESMOOTH_INITIALDELAY > 0) {
						scheduler->setTimer(lastTimer);

					} else {
						// immediate self-message
						lastTimer->setFrom(this);
						lastTimer->setTo(this);
						sendMessage(lastTimer);

					}

				}

			}

		}

	}
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleSmooth::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	assert(pkt.get() != NULL);

	if (BB_SIMPLESMOOTH_STEPS == 1) {
		// smoothing disabled
		pkt->setFrom(this);
		pkt->setTo(getNext());
		sendMessage(pkt);

	} else {
		outgoingQueue.push_back(QueueItem(pkt));
		if (lastTimer == NULL) {
			// schedule an immediate timer (well, almost immediately)
			lastTimer = shared_ptr<Timer>(new Timer(BB_SIMPLESMOOTH_INITIALDELAY, this));
			if (BB_SIMPLESMOOTH_INITIALDELAY > 0) {
				scheduler->setTimer(lastTimer);

			} else {
				// immediate self-message
				lastTimer->setFrom(this);
				lastTimer->setTo(this);
				sendMessage(lastTimer);

			}
		}

	}
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleSmooth::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// hand to upper entity
	msg->setFrom(this);
	msg->setTo(getPrev());
	sendMessage(msg);
}

void Bb_SimpleSmooth::sendNextPacket()
{
	if (outgoingQueue.size() > 0) {
		shared_ptr<CMessageBuffer> pkt = outgoingQueue.front().pkt;
		pkt->setFrom(this);
		pkt->setTo(getNext());
		sendMessage(pkt);
		outgoingQueue.pop_front();
	}
}

}
}
}
