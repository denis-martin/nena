/** @file
 * bb_enc.h
 *
 * @brief Crypt Test Encode/Decode Building Block
 *
 *  Created on: Apr 18, 2010
 *      Author: Benjamin Behringer
 */

#ifndef _BB_ENC_H_
#define _BB_ENC_H_

#include "composableNetlet.h"
#include "messages.h"
#include "nena.h"

#include <map>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

namespace edu_kit_tm {
namespace itm {
namespace crypt {

extern const std::string encClassName;

class CryptEngine
{
	public:
	boost::scoped_ptr<CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption> enc;
	boost::scoped_ptr<CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption> dec;

	CryptEngine ();
	CryptEngine (const CryptEngine &);
	~CryptEngine ();

	CryptEngine & operator= (const CryptEngine &);

};


class Bb_Enc : public IBuildingBlock
{
	private:
	std::map<LocalConnId, boost::shared_ptr<CryptEngine> > engineMap;

	public:
	Bb_Enc(CNena * nena, IMessageScheduler * sched, IComposableNetlet * netlet, const std::string id);
	virtual ~Bb_Enc();

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

#endif /* _BB_ENC_H_ */
