/*
 * @file crypt_message.h
 *
 * @brief extend the standard message buffer with crypt specific attributes
 *
 *  Created on: 16.05.2012
 *      Author: benjamin
 */

#ifndef CRYPT_MESSAGE_H_
#define CRYPT_MESSAGE_H_

#include <messageBuffer.h>

class CryptBuffer : public CMessageBuffer {
public:
	uint32_t flowId;

	/// all functions remain untouched

	CryptBuffer(IMessageProcessor* from = NULL, IMessageProcessor* to = NULL, const Type type = t_outgoing,
				std::size_t bufferSize = 0):
					CMessageBuffer (from, to, type, bufferSize),
					flowId(-1)
	{}

	CryptBuffer(buffer_t buffer):
		CMessageBuffer(buffer),
		flowId(-1)
	{}

	CryptBuffer(shared_buffer_t buffer):
		CMessageBuffer(buffer),
		flowId(-1)
	{}

	CryptBuffer(std::size_t size):
		CMessageBuffer(size),
		flowId(-1)
	{}

	CryptBuffer(const CryptBuffer & rhs):
		CMessageBuffer(rhs),
		flowId(rhs.flowId)
	{}

	CryptBuffer(const CMessageBuffer & rhs):
		CMessageBuffer(rhs),
		flowId(-1)
	{}

	virtual ~CryptBuffer()
	{}
};


#endif /* CRYPT_MESSAGE_H_ */
