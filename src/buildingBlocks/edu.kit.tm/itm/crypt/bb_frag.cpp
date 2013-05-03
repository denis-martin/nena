/** @file
 * bb_frag.cpp
 *
 * @brief Fragmentation Building Block
 *
 *  Created on: Apr 18, 2010
 *      Author: Benjamin Behringer
 */

#include "bb_frag.h"
#include "messageBuffer.h"

namespace edu_kit_tm {
namespace itm {
namespace crypt {
	
using namespace std;

const string fragClassName = "bb://edu.kit.tm/itm/crypt/Bb_Frag";

Bb_Frag::Bb_Frag(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const string id) :
	IBuildingBlock(nodeArch, sched, netlet, id)
{
	className += "::Bb_Frag";
}

Bb_Frag::~Bb_Frag()
{
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_Frag::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_Frag::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled timer!");
	throw EUnhandledMessage();
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void Bb_Frag::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL)
	{
		DBG_ERROR("Unhandled outgoing message!");
		throw EUnhandledMessage();
	}

	DBG_DEBUG(boost::format("%1%: got an outgoing message!") % className);

	// TODO: fragment if necessary

	pkt->setFrom(this);
	pkt->setTo(next);
	sendMessage(pkt);

}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void Bb_Frag::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL)
	{
		DBG_ERROR("Unhandled incoming message!");
		throw EUnhandledMessage();
	}

	DBG_DEBUG(boost::format("%1%: got an incoming message!") % className);

	// TODO: reassemble packet

	pkt->setFrom(this);
	pkt->setTo(prev);
	sendMessage(pkt);
}

}
}
}
