/** @file
 * bb_header.h
 *
 * @brief Crypt Test Header Building Block
 *
 *  Created on: Apr 18, 2010
 *      Author: Benjamin Behringer
 */

#ifndef _BB_HEADER_H_
#define _BB_HEADER_H_

#include "composableNetlet.h"

#include "nena.h"
#include "messageBuffer.h"

#include <string>

namespace edu_kit_tm {
namespace itm {
namespace crypt {

extern const std::string headerClassName;

class CryptHeader: public IHeader {
public:

	std::string encMethod;
	uint32_t flowId;

	CryptHeader()
	{
	}

	CryptHeader(std::string encmethod, uint32_t id);
	CryptHeader(const CryptHeader & rhs);
	virtual ~CryptHeader()
	{
	}

	/**
	 * @brief Serialize all relevant data into a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const;

	/**
	 * @brief De-serialize all relevant data from a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer);
};

class Bb_Header: public IBuildingBlock {
public:
	Bb_Header(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~Bb_Header();

	// from IMessageProcessor
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
};

} // maam
} // itm
} // edu.kit.tm

#endif /* _BB_HEADER_H_ */
