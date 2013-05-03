/*
 * pingPong.h
 *
 *  Created on: Jul 21, 2009
 *      Author: denis
 */

#ifndef PINGPONG_H_
#define PINGPONG_H_

#include "../appConnectorOmnet.h"

#define APP_OMNET_PINGPONG_NAME "application://edu.kit.tm/itm/test/pingpong"

class CNena;

/**
 * @brief Ping pong example application for OMNeT.
 *
 * Note: In Omnet, every app can inherited directly from the app connector.
 * On real system, one may want to consider the connector only as a proxy
 * class.
 */
class CPingPong : public CAppConnectorOmnet
{
private:
	CNena* nodeArch;

	std::string pingName;
	std::string pongName;

public:
	CPingPong(CNena* nodeArch, IMessageScheduler* sched);
	virtual ~CPingPong();

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

	/**
	 * @brief return a unique name string for this MessageProcessor
	 */
	virtual const std::string & getId () const;

	virtual void sendUser1(boost::shared_ptr<CMessageBuffer> message);

	/**
	 * @brief	Send an event to the application
	 *
	 * @param event		Event ID
	 * @param param		Parameter for the event
	 */
	virtual void sendEvent(event_t event, const std::string& param);

	// own methods

	void handlePingTimer();
};

#endif /* PINGPONG_H_ */
