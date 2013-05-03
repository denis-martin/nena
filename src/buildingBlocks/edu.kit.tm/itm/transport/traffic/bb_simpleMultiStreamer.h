/*
 * bb_simpleMultiStreamer.h
 *
 *  Created on: Jan 29, 2010
 *      Author: denis
 */

#ifndef BB_SIMPLEMULTISTREAMER_H_
#define BB_SIMPLEMULTISTREAMER_H_

#include "composableNetlet.h"

#include "messageBuffer.h"
#include "appConnector.h"

namespace edu_kit_tm {
namespace itm {
namespace transport {

#define SIMPLEMULTISTREAMER_REGISTER_MAX_RETRIES 20

extern const std::string simpleMultiStreamerClassName;

class Bb_SimpleMultiStreamer: public IBuildingBlock
{
private:
	/**
	 * Just in case we need further information later on.
	 */
	class ClientItem
	{
	public:
		std::string nodeName;
		std::string requestedId;

		ClientItem(std::string nodeName = std::string(), std::string requestedId = std::string()) :
			nodeName(nodeName), requestedId(requestedId) {}
		virtual ~ClientItem() {}
	};

	/**
	 * Just in case we need further information later on.
	 */
	class AppItem
	{
	public:
		std::string remoteNode; // node://...
		std::string remoteId; // video://... or node://...
		std::string appIdentifier; // TODO: this is not unique...
		IAppConnector* appConn;
		int retriesLeft;
		bool registerAcknowledged;
		bool unregistering;
		IAppConnector::method_t method;

		AppItem(std::string remoteNode = std::string(),
				std::string remoteId = std::string(),
				std::string appIdentifier = std::string(),
				IAppConnector* appConn = NULL) :
			remoteNode(remoteNode), remoteId(remoteId),
			appIdentifier(appIdentifier), appConn(appConn),
			retriesLeft(SIMPLEMULTISTREAMER_REGISTER_MAX_RETRIES),
			registerAcknowledged(false),
			unregistering(false)
		{
			if (appConn) {
				method = appConn->getMethod();
			}
		}

		virtual ~AppItem() {}
	};

	/**
	 * @brief Register retransmission timer.
	 *
	 * Single shot timer.
	 */
	class Timer : public CTimer
	{
	public:
		bool isCancled;
		IAppConnector* appConn;

		Timer(double timeout, IMessageProcessor *proc, IAppConnector* appConn)
			: CTimer(timeout, proc), isCancled(false), appConn(appConn)
		{}
		virtual ~Timer() {}
	};

	/**
	 * @brief Unregister retransmission timer.
	 *
	 * Single shot timer.
	 */
	class UnregisterTimer : public CTimer
	{
	public:
		bool isCancled;
		std::string appId;

		UnregisterTimer(double timeout, IMessageProcessor *proc, std::string appId)
			: CTimer(timeout, proc), isCancled(false), appId(appId)
		{}
		virtual ~UnregisterTimer() {}
	};

	std::list<ClientItem> clients;
	std::list<AppItem> registeredApps;

public:
	enum MsgTypes {
		msgType_Data = 0,
		msgType_Register = 1,
		msgType_Unregister = 2,
		msgType_Ack = 3
	};

	Bb_SimpleMultiStreamer(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~Bb_SimpleMultiStreamer();

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

} // transport
} // itm
} // edu.kit.tm

#endif /* BB_SIMPLEMULTISTREAMER_H_ */
