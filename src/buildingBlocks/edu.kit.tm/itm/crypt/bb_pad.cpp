/** @file
 * bb_pad.cpp
 *
 * @brief Crypt Test Padding Building Block
 *
 *  Created on: Apr 18, 2010
 *      Author: Benjamin Behringer
 */

#include "bb_pad.h"
#include "messageBuffer.h"

namespace edu_kit_tm {
namespace itm {
namespace crypt {

using boost::shared_ptr;
using namespace std;

const string padClassName = "bb://edu.kit.tm/itm/crypt/Bb_Pad";

Bb_Pad::Bb_Pad(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const string id) :
	IBuildingBlock(nodeArch, sched, netlet, id)
{
	className += "::Bb_Pad";
}

Bb_Pad::~Bb_Pad()
{
}

void Bb_Pad::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

void Bb_Pad::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled timer!");
	throw EUnhandledMessage();
}

void Bb_Pad::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL)
	{
		DBG_ERROR("Unhandled outgoing message!");
		throw EUnhandledMessage();
	}

	uint32_t padsize;

	// pad payload to a multiple of 128 bit, including last 4 byte which is an integer holding the pad length
	padsize = (16 - (pkt->size ()+sizeof(uint32_t)) % 16) % 16;

	// append padding and padding size
	shared_ptr<CMessageBuffer> padding(new CMessageBuffer(padsize + sizeof(uint32_t)));
	memset(padding->getBuffer().at(0).mutable_data(), 0, padsize);
	padding->set_cursor(padsize);
	padding->push_ulong(padsize);

	pkt->push_back(padding);

	DBG_DEBUG(boost::format("%1%: padded an outgoing message with %2% bytes!") % className % padsize);

	pkt->setFrom(this);
	pkt->setTo(next);
	sendMessage(pkt);

}

void Bb_Pad::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL)
	{
		DBG_ERROR("Unhandled incoming message!");
		throw EUnhandledMessage();
	}

	assert(pkt->getBuffer().length() == 1);
	assert(pkt->size() - pkt->get_cursor() - sizeof(uint32_t) > 0);

	message_t payload = pkt->pop_buffer(pkt->size() - pkt->get_cursor() - sizeof(uint32_t));
	uint32_t padsize = pkt->pop_ulong();
	message_t realpayload = payload(0, payload.size() - padsize);

	pkt->setBuffer(realpayload);
	
	/// TODO: make this automatic
	pkt->set_cursor(0);

	DBG_DEBUG(boost::format("%1%: got an incoming message and removed %2% bytes of padding!") % className % padsize);

	pkt->setFrom(this);
	pkt->setTo(prev);
	sendMessage(pkt);
}

}
}
}
