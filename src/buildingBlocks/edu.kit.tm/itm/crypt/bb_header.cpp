/** @file
 * bb_header.cpp
 *
 * @brief Crypt Test Header Building Block
 *
 *  Created on: Apr 18, 2010
 *      Author: Benjamin Behringer
 */

#include "bb_header.h"
#include "messageBuffer.h"
#include "crypt_buffer.h"

namespace edu_kit_tm {
namespace itm {
namespace crypt {

using std::string;
using boost::shared_ptr;

const string headerClassName ="bb://edu.kit.tm/itm/crypt/Bb_Header";

CryptHeader::CryptHeader(string encmethod, uint32_t id) :
	encMethod(encmethod),
	flowId(id)
{
}

CryptHeader::CryptHeader(const CryptHeader & rhs)
{
	encMethod = rhs.encMethod;
	flowId = rhs.flowId;
}

boost::shared_ptr<CMessageBuffer> CryptHeader::serialize() const
{
	shared_ptr<CMessageBuffer> buffer(
		new CMessageBuffer(6 * sizeof(char) + sizeof(uint32_t)));
	buffer->push_string(encMethod);
	buffer->push_ulong(flowId);
	return buffer;
}

void CryptHeader::deserialize(boost::shared_ptr<CMessageBuffer> buffer)
{
	encMethod = buffer->pop_string(6);
	flowId = buffer->pop_ulong();
}

/* ========================================================================= */

Bb_Header::Bb_Header(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const string id) :
	IBuildingBlock(nodeArch, sched, netlet, id)
{
	className += "::Bb_Header";
}

Bb_Header::~Bb_Header()
{
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_Header::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_Header::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled timer!");
	throw EUnhandledMessage();
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void Bb_Header::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CryptBuffer> pkt = msg->cast<CryptBuffer>();
	if (pkt.get() == NULL) {
		DBG_ERROR("Unhandled outgoing message!");
		throw EUnhandledMessage();
	}

	/// add new encryption Header to the message
	pkt->push_header(CryptHeader("AES128", pkt->flowId));

	DBG_INFO(FMT("%1%: Outgoing message, encoding method is AES128, flowId is %2%!") % className % pkt->flowId);

	pkt->setFrom(this);
	pkt->setTo(next);
	sendMessage(pkt);
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void Bb_Header::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL) {
		DBG_ERROR("Unhandled incoming message!");
		throw EUnhandledMessage();
	}

	CryptHeader header;
	pkt->pop_header(header);

	DBG_INFO(FMT("%1%: Incoming message, encoding method is %2%, flowId is %3%!") % className % header.encMethod, header.flowId);

	shared_ptr<CryptBuffer> crb(new CryptBuffer(*pkt));
	crb->flowId = header.flowId;

	crb->setFrom(this);
	crb->setTo(prev);
	sendMessage(crb);
}

}
}
}
