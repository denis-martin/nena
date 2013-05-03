/*
 * bb_restCommands.h
 *
 *  Created on: Jan 29, 2010
 *      Author: denis
 */

#ifndef BB_RESTCOMMANDS_H_
#define BB_RESTCOMMANDS_H_

#include "composableNetlet.h"
#include "mutexes.h"

#include "messages.h"
#include "messageBuffer.h"
#include "flowState.h"
#include "appConnector.h"

#include <boost/shared_ptr.hpp>

#include <string>

namespace edu_kit_tm {
namespace itm {
namespace sig {

static const std::string BB_RESTCOMMANDS_ID = "bb://edu.kit.tm/itm/sig/restCommands";

class Bb_RestCommands: public IBuildingBlock
{
private:
	class RestCommandsStateObject : public CFlowState::StateObject
	{
	public:
		bool requestSent;

		RestCommandsStateObject() : CFlowState::StateObject(), requestSent(false)
		{
			setId(BB_RESTCOMMANDS_ID);
		}

		virtual ~RestCommandsStateObject()
		{}
	};

	boost::shared_ptr<CMessageBuffer> createCtrlPacket(boost::shared_ptr<CFlowState> flowState,
		boost::shared_ptr<RestCommandsStateObject> so = boost::shared_ptr<RestCommandsStateObject>(),
		IAppConnector::method_t msgType = IAppConnector::method_none,
		std::string requestUri = std::string());

public:
	Bb_RestCommands(CNena *nena, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~Bb_RestCommands();

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

} // sig
} // itm
} // edu.kit.tm

#endif /* BB_RESTCOMMANDS_H_ */
