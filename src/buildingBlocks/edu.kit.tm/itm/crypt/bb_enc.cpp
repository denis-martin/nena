/** @file
 * bb_enc.cpp
 *
 * @brief Crypt Test Encode/Decode Building Block
 *
 *  Created on: Apr 18, 2010
 *      Author: Benjamin Behringer
 */

#include "bb_enc.h"
#include "messageBuffer.h"
#include "crypt_buffer.h"

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>

#include <map>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

using CryptoPP::AES;
using CryptoPP::CBC_Mode;

namespace edu_kit_tm {
namespace itm {
namespace crypt {

using CryptoPP::AES;
using CryptoPP::CBC_Mode;

using boost::shared_ptr;
using boost::scoped_ptr;

using namespace std;

const string encClassName = "bb://edu.kit.tm/itm/crypt/Bb_Enc";

byte key[] = "aRcw()%.,!=adsjc";
byte iv[] = "kLIHsRENKN/xII2?";

CryptEngine::CryptEngine ()
{
	enc.reset(new CBC_Mode<AES>::Encryption(key, sizeof(key)-1, iv));
	dec.reset(new CBC_Mode<AES>::Decryption(key, sizeof(key)-1, iv));
}

CryptEngine::CryptEngine (const CryptEngine & rhs)
{
	/**
	 * not implemented yet
	 */
	enc.reset(dynamic_cast<CBC_Mode<AES>::Encryption *> (rhs.enc->Clone ()));
	dec.reset(dynamic_cast<CBC_Mode<AES>::Decryption *> (rhs.enc->Clone ()));

}

CryptEngine::~CryptEngine ()
{
}

CryptEngine & CryptEngine::operator= (const CryptEngine & rhs)
{
	if (&rhs != this)
	{
		/**
		 * not implemented yet
		 */
		enc.reset(dynamic_cast<CBC_Mode<AES>::Encryption *> (rhs.enc->Clone ()));
		dec.reset(dynamic_cast<CBC_Mode<AES>::Decryption *> (rhs.enc->Clone ()));
	}

	return *this;
}

/* ========================================================================= */

Bb_Enc::Bb_Enc(CNena * nena, IMessageScheduler * sched, IComposableNetlet * netlet, const string id) :
	IBuildingBlock(nena, sched, netlet, id)
{
	className += "::Bb_Enc";
}

Bb_Enc::~Bb_Enc()
{
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_Enc::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_Enc::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled timer!");
	throw EUnhandledMessage();
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void Bb_Enc::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL)
	{
		DBG_ERROR("Unhandled outgoing message!");
		throw EUnhandledMessage();
	}

	shared_ptr<CryptBuffer> crb(new CryptBuffer(*pkt));
	crb->flowId = pkt->getFlowState()->getFlowId();

	DBG_DEBUG(boost::format("%1%: got an outgoing message with length %2%!") % className % crb->size());

	map<CFlowState::FlowId, shared_ptr<CryptEngine> >::iterator it;

	if ((it = engineMap.find(crb->flowId)) != engineMap.end() )
	{
		assert(crb->getBuffer().length() > 0);
		if (crb->getBuffer().length() > 1) {
			message_t payload;
			payload.push_back(crb->getBuffer().linearize());
			it->second->enc->ProcessData(payload.at(0).mutable_data(), payload.at(0).data(), payload.size());
			crb->setBuffer(payload);
		} else {
			it->second->enc->ProcessData(crb->getBuffer().at(0).mutable_data(), crb->getBuffer().at(0).data(), crb->size());
		}
	}
	else
	{
		engineMap[crb->flowId].reset(new CryptEngine());

		if (crb->getBuffer().length() > 1)
		{
			shared_buffer_t payload = crb->getBuffer().linearize();
			engineMap[crb->flowId]->enc->ProcessData(payload.mutable_data(), payload.data(), payload.size());
			message_t buf;
			buf.push_back(payload);
			crb->setBuffer(buf);
		}
		else
		{
			engineMap[crb->flowId]->enc->ProcessData(crb->getBuffer().at(0).mutable_data(), crb->getBuffer().at(0).data(), crb->size());
		}
	}

	crb->setFrom(this);
	crb->setTo(next);
	sendMessage(crb);

}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void Bb_Enc::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CryptBuffer> pkt = msg->cast<CryptBuffer>();
	if (pkt.get() == NULL)
	{
		DBG_ERROR("Unhandled incoming message!");
		throw EUnhandledMessage();
	}

	DBG_DEBUG(boost::format("%1%: got an incoming message!") % className);

	shared_buffer_t payload = pkt->pop_buffer().linearize();

	map<CFlowState::FlowId, shared_ptr<CryptEngine> >::iterator it;

	/// we do not use the "normal" flow id, as it might not exist
	if ((it = engineMap.find(pkt->flowId)) != engineMap.end() )
	{
		it->second->dec->ProcessData(payload.mutable_data(), payload.data(), payload.size());
	}
	else
	{
		engineMap[pkt->flowId].reset(new CryptEngine());
		engineMap[pkt->flowId]->dec->ProcessData(payload.mutable_data(), payload.data(), payload.size());
	}

	message_t tmp_msg;
	tmp_msg.push_back(payload);
	pkt->setBuffer(tmp_msg);
	
	pkt->set_cursor(0);
	
	pkt->setFrom(this);
	pkt->setTo(prev);
	sendMessage(pkt);
}

}
}
}
