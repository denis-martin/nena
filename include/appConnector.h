/**@file
 *
 * appInterface.h
 *
 *  Created on: Jul 21, 2009
 *      Author: denis
 */

#ifndef APPCONNECTOR_H_
#define APPCONNECTOR_H_

#include "messages.h"
#include "messageBuffer.h"
#include "netlet.h"

#include <string>

#include <stdint.h>

// Local connection id
typedef unsigned int LocalConnId;

/**
 * @brief Stub for Application Interfaces. There is one connector per connection.
 */
class IAppConnector : public IMessageProcessor
{
public:
	typedef enum
	{
		method_none, method_get, method_put, method_connect, method_bind, method_dget, method_dput
	} method_t;

	typedef enum
	{
		userreq_ok = 1, userreq_cancel = 2, userreq_yes = 4, userreq_no = 8, userreq_okcancel = userreq_ok
				| userreq_cancel, userreq_yesno = userreq_yes | userreq_no
	} userreq_t;

	typedef enum
	{
		event_none, event_incoming,
	} event_t;

	class Message_UserRequestAnswer : public IMessage
	{
	public:
		uint32_t requestId;
		userreq_t answer;

		Message_UserRequestAnswer(uint32_t requestId, userreq_t answer, IMessageProcessor *from = NULL,
				IMessageProcessor *to = NULL) :
				IMessage(from, to, IMessage::t_message), requestId(requestId), answer(answer)
		{
		}

		virtual ~Message_UserRequestAnswer()
		{
		}
	};

protected:
	IAppConnector *parent;		///< parent app connector (may be NULL)
	std::string identifier; 	///< ID of the application/connection/service
	INetlet* netlet;			///< associated Netlet
	boost::shared_ptr<CFlowState> flowState; ///< Flow state information (might be NULL)

	/// switches
	bool registered;			///< app connector is registered within the node architecture
	bool endOfLife;				///< app connector has reached end of life

	std::string remoteURI;		///< TODO not always available
	std::string optString;		///< Options string (XML)

	std::string requirements;	///< Requirements string

	method_t method;			///< Command mode

public:
	IAppConnector(IMessageScheduler* sched) :
			IMessageProcessor(sched), parent(NULL), netlet(NULL), registered(false), endOfLife(false), method(method_none)
	{
		className += "::IAppConnector";
	}

	virtual ~IAppConnector()
	{
	}

	/**
	 * @brief	Return parent app connector (may be NULL)
	 */
	virtual IAppConnector* getParent() const
	{
		return parent;
	}

	/**
	 * @brief	Set parent app connector
	 */
	virtual void setParent(IAppConnector *p)
	{
		parent = p;
	}

	/**
	 * @brief	Return ID of the application/connection/service
	 */
	virtual std::string getIdentifier() const
	{
		return identifier;
	}

	/**
	 * @brief	Set ID of the application/connection/service
	 */
	virtual void setIdentifier(const std::string& id)
	{
		identifier = id;
	}

	/**
	 * @brief 	Return flow state information (only locally valid)
	 */
	virtual boost::shared_ptr<CFlowState> getFlowState() const
	{
		return flowState;
	}

	/**
	 * @brief 	Set flow state information (only locally valid)
	 */
	virtual void setFlowState(boost::shared_ptr<CFlowState> state)
	{
		flowState = state;
	}

	/**
	 * @brief	Resets (deletes) flow state.
	 */
	virtual void resetFlowState()
	{
		flowState.reset();
	}

	/**
	 * @brief 	Return Netlet associated with the app connector. NULL if none
	 * 			is associated.
	 */
	virtual INetlet* getNetlet() const
	{
		return netlet;
	}

	/**
	 * @brief 	Set Netlet association.
	 */
	virtual void setNetlet(INetlet* netlet)
	{
		this->netlet = netlet;
	}

	/**
	 * @brief	Return ID of the application/connection/service
	 */
	virtual std::string getRemoteURI() const
	{
		return remoteURI;
	}

	/**
	 * @brief	Set ID of the application/connection/service
	 */
	virtual void setRemoteURI(const std::string& remoteURI)
	{
		this->remoteURI = remoteURI;
	}

	/**
	 * @brief	Return options string including application requirements
	 */
	virtual std::string getOptString() const
	{
		return optString;
	}

	/**
	 * @brief	Set options string including application requirements
	 */
	virtual void setOptString(const std::string& opts)
	{
		optString = opts;
	}

	/**
	 * @brief	Return requirements string TODO: remove optString?
	 */
	virtual std::string getRequirements() const
	{
		return requirements;
	}

	/**
	 * @brief	Set requirements string TODO: remove optString?
	 */
	virtual void setRequirements(const std::string& reqs)
	{
		requirements = reqs;
	}

	/**
	 * @brief	Set if app connector is registered at nodearch
	 */
	virtual void setRegistered(bool reg)
	{
		registered = reg;
	}

	/**
	 * @brief	get registration state
	 */
	virtual bool getRegistered() const
	{
		return registered;
	}

	/**
	 * @brief	Set command mode
	 */
	virtual void setMethod(method_t mode)
	{
		this->method = mode;
	}

	/**
	 * @brief	Get command mode
	 */
	virtual method_t getMethod() const
	{
		return method;
	}

	/**
	 * @brief	Present installation request to the user
	 *
	 * @param msgstr		String to present to the user
	 * @param requestType	Sum/arithmetic OR of reply options, see IAppConnector::userreq_t
	 * @param replyTo		Message processor to reply to
	 * @param requestId		Optional request ID
	 */
	virtual void sendUserRequest(const std::string& msgstr, userreq_t requestType, IMessageProcessor *replyTo,
			uint32_t requestId)
	{
	}

	/**
	 * @brief	Send USER_1 signal; temporarily added
	 *
	 * @param message 	User message
	 */
	virtual void sendUser1(boost::shared_ptr<CMessageBuffer> message) = 0;

	/**
	 * @brief	Send an event to the application
	 *
	 * @param event		Event ID
	 * @param param		Parameter for the event
	 */
	virtual void sendEvent(event_t event, const std::string& param) = 0;

	/**
	 * @brief	Release / close app connector
	 */
	virtual void release() = 0;

};

/**
 * @brief Interface for a Application Interface Server. Spawns IAppConnectors.
 */

class IAppServer : public IMessageProcessor
{
public:
	IAppServer(IMessageScheduler* sched) :
			IMessageProcessor(sched)
	{
		className += "IAppServer";
	}
	virtual ~IAppServer()
	{
	}

	/// start all servers
	virtual void start() = 0;
	/// stop server
	virtual void stop() = 0;
	/// start all spawned app connectors
	virtual void startAppConnectors() = 0;
	/// stop all spawned app connectors
	virtual void stopAppConnectors() = 0;

	virtual std::string getClassName() const
	{
		return className;
	}
};

#endif /* APPCONNECTOR_H_ */
