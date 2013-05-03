/*
 * bb_simpleSegment.h
 *
 *  Created on: Jan 29, 2010
 *      Author: denis
 */

#ifndef BB_SIMPLESEGMENT_H_
#define BB_SIMPLESEGMENT_H_

#include "composableNetlet.h"

#include "messages.h"
#include "messageBuffer.h"
#include "flowState.h"

#include <boost/shared_ptr.hpp>

namespace edu_kit_tm {
namespace itm {
namespace transport {

static const std::string BB_SIMPLESEGMENT_ID = "bb://edu.kit.tm/itm/transport/segment/simple";

class Bb_SimpleSegment: public IBuildingBlock
{
public:
	Bb_SimpleSegment(CNena *nena, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~Bb_SimpleSegment();

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

	// from IBuildingBlock

	virtual const std::string & getId() const;
};

} // transport
} // itm
} // edu.kit.tm

#endif /* BB_SIMPLESEGMENT_H_ */
