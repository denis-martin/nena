/*
 * cbr.h
 *
 *  Created on: May 28, 2010
 *      Author: denis
 */

#ifndef CBR_H_
#define CBR_H_

#include "../appConnectorOmnet.h"

#define APP_OMNET_CBR_NAME "application://edu.kit.tm/itm/test/omnet/cbr"

class CNena;

/**
 * @brief CBR application for OMNeT.
 *
 * Note: In Omnet, every app can inherited directly from the app connector.
 * On real system, one may want to consider the connector only as a proxy
 * class.
 */
class CCbr : public CAppConnectorOmnet
{
private:
	CNena* nodeArch;

	std::string sourceNode;
	std::string destinationNode;
	uint32_t packetSize;
	double interval;
	double jitter;
	double startTime;
	double stopTime;

public:
	CCbr(CNena* nodeArch, IMessageScheduler* sched);
	virtual ~CCbr();

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

	void handleTimer();
};

#endif /* CBR_H_ */
