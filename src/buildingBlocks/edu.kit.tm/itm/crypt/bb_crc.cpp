/** @file
 * bb_crc.cpp
 *
 * @brief Crypt Test CRC Building Block
 *
 *  Created on: Apr 18, 2010
 *      Author: Benjamin Behringer
 */

#include "bb_crc.h"
#include "messageBuffer.h"

#include <cryptopp/crc.h>

namespace edu_kit_tm {
namespace itm {
namespace crypt {

using namespace std;
using CryptoPP::CRC32;

const string crcClassName = "bb://edu.kit.tm/itm/crypt/Bb_CRC";

Bb_CRC::Bb_CRC(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const string id) :
	IBuildingBlock(nodeArch, sched, netlet, id)
{
	className += "::Bb_CRC";
}

Bb_CRC::~Bb_CRC()
{
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_CRC::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_CRC::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled timer!");
	throw EUnhandledMessage();
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void Bb_CRC::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL)
	{
		DBG_ERROR("Unhandled outgoing message!");
		throw EUnhandledMessage();
	}

	// create digest buffer
	shared_buffer_t digest(crc.DigestSize());

	// flatten message buffer
	shared_buffer_t buffer = pkt->getBuffer().linearize();

	// now hash the data
	crc.CalculateDigest(digest.mutable_data(), buffer.data(), buffer.size());

	// and add the digest to the buffer
	pkt->push_back(digest);

	DBG_DEBUG(boost::format("%1%: got an outgoing message with crc %2$02X%3$02X%4$02X%5$02X!") % className
			% static_cast<int> (digest[0])
			% static_cast<int> (digest[1])
			% static_cast<int> (digest[2])
		    % static_cast<int> (digest[3]) );

	pkt->setFrom(this);
	pkt->setTo(next);
	sendMessage(pkt);

}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void Bb_CRC::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL)
	{
		DBG_ERROR("Unhandled incoming message!");
		throw EUnhandledMessage();
	}

	message_t payload = pkt->pop_buffer(pkt->size() - pkt->get_cursor() - crc.DigestSize());
	assert(payload.length() == 1);

	message_t digest = pkt->pop_buffer(crc.DigestSize());
	assert(digest.length() == 1);

	if (crc.VerifyDigest (digest.at(0).data(), payload.at(0).data(), payload.size()))
	{
		DBG_DEBUG(boost::format("%1%: got an incoming message with CRC32 %2$02X%3$02X%4$02X%5$02X!") % className
					% static_cast<int> (digest.at(0)[0])
					% static_cast<int> (digest.at(0)[1])
					% static_cast<int> (digest.at(0)[2])
				    % static_cast<int> (digest.at(0)[3]) );
	}
	else
	{
		byte realdigest[crc.DigestSize()];
		crc.CalculateDigest(realdigest, payload.at(0).data(), payload.size());

		throw ECRCMissmatch((boost::format("%1%: CRC32 %2$02X%3$02X%4$02X%5$02X not matching message! CRC32 should be %6$02X%7$02X%8$02X%9$02X") % className
				% static_cast<int> (digest.at(0)[0])
				% static_cast<int> (digest.at(0)[1])
				% static_cast<int> (digest.at(0)[2])
			    % static_cast<int> (digest.at(0)[3])
			    % static_cast<int> (realdigest[0])
			    % static_cast<int> (realdigest[1])
			    % static_cast<int> (realdigest[2])
			    % static_cast<int> (realdigest[3])).str());
	}

	pkt->setBuffer(payload);
		
	/// outch:
	pkt->set_cursor(0);


	pkt->setFrom(this);
	pkt->setTo(prev);
	sendMessage(pkt);
}

}
}
}
