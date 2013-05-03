/**
 * @file nodeconnector.h
 *
 * @brief Interface to Node Architecture
 *
 *  Created on: June 22, 2009
 *      Author: Benjamin Behringer
 *
 */

#ifndef _NODECONNECTOR_H_
#define _NODECONNECTOR_H_

#include "../../src/targets/boost/msg.h"

#include <string>

/**
 * @class INenai
 *
 * @brief Interface for Communication with Node Architecture
 *
 */

class INenai
{
private:
	/**
	 * @brief for easier handling...
	 */
	virtual void rawSend (MSG_TYPE type, const std::string & payload) = 0;

public:
	INenai () {}
	virtual ~INenai () {}

	/**
	 * @brief sendpayload to nodearch
	 */
	virtual void sendData (const std::string & payload);

	/**
	 * @brief send metadata to nodearch
	 */
	virtual void sendMetadata (const std::string & metadata);

	/**
	 * @brief send requirements to nodearch
	 */
	virtual void sendRequirements (const std::string & requirements);

	/**
	 * @brief normal termination of a connection
	 */
	virtual void sendEndOfStream();

	/**
	 * @brief sets connection test mode, will return messages immediately if true
	 */
	virtual void setTestMode (bool mode);

	/**
	 * @brief sets connection to extended test mode, involving the nodearch scheduler
	 */
	virtual void setExtTestMode (bool mode);

	/**
	 * @brief set target id
	 */
	virtual void setTarget (const std::string & target);

	/**
	 * @brief set own id
	 */
	virtual void setID (const std::string & id);
        
	/**
	 * @brief initiate GET command
	 */
	virtual void initiateGet(const std::string & target);

	/**
	 * @brief initiate PUT command
	 */
	virtual void initiatePut(const std::string & target);

	/**
	 * @brief initiate CONNECT command
	 */
	virtual void initiateConnect(const std::string & target);

	/**
	 * @brief initiate BIND command
	 */
	virtual void initiateBind(const std::string & target);

	/**
	 * @brief start I/O activity
	 */
	virtual void run () = 0;

	/**
	 * @brief end I/O activity
	 */
	virtual void stop () = 0;

	/**
	 * @brief return local connection id
	 */
	virtual unsigned int getConnectionId () = 0;

	/**
	 * @brief return socket error state
	 */
	virtual unsigned int getError () = 0;

};

#endif
