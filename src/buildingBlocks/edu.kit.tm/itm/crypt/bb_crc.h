/** @file
 * bb_crc.h
 *
 * @brief Crypt Test CRC Building Block
 *
 *  Created on: Apr 18, 2010
 *      Author: Benjamin Behringer
 */

#ifndef _BB_CRC_H_
#define _BB_CRC_H_

#include "composableNetlet.h"

#include "nena.h"

#include <cryptopp/crc.h>

#include <exception>

namespace edu_kit_tm {
namespace itm {
namespace crypt {

extern const std::string crcClassName;

class Bb_CRC : public IBuildingBlock
{
	public:
	class ECRCMissmatch : public std::exception
	{
		protected:
		std::string msg;

		public:
		ECRCMissmatch() throw () : msg(std::string()) {};
		ECRCMissmatch(const std::string& msg) throw () : msg(msg){};
		virtual ~ECRCMissmatch() throw () {};

		virtual const char* what() const throw () { return msg.c_str(); };
	};


	private:
	CryptoPP::CRC32 crc;

	public:
	Bb_CRC(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~Bb_CRC();

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

} // maam
} // itm
} // edu.kit.tm

#endif /* _BB_CRC_H_ */
