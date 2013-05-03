#include "nenai.h"
#include "../../src/targets/boost/msg.h"

#include <string>

using std::string;

void INenai::sendData (const string & payload)
{
	rawSend (MSG_TYPE_DATA, payload);
}

void INenai::sendMetadata (const string & metadata)
{
	rawSend (MSG_TYPE_META, metadata);
}

void INenai::sendRequirements (const string & requirements)
{
	rawSend (MSG_TYPE_REQ, requirements);
}

void INenai::sendEndOfStream()
{
	string s;
	rawSend (MSG_TYPE_END, s);
}

void INenai::setTestMode (bool mode)
{
	string metadata;
	metadata.append(1, static_cast<char> (mode));
	rawSend (MSG_TYPE_CONNECTORTEST, metadata);
}

void  INenai::setExtTestMode (bool mode)
{
	string metadata;
	metadata.append(1, static_cast<char> (mode));
	rawSend (MSG_TYPE_EXTCONNECTORTEST, metadata);
}

/**
 * @deprecated
 */
void INenai::setTarget (const string & target)
{
	rawSend (MSG_TYPE_TARGET, target);
}

void INenai::setID (const string & id)
{
	rawSend (MSG_TYPE_ID, id);
}

/**
 * @brief initiate GET command
 */
void INenai::initiateGet(const std::string & target)
{
	rawSend(MSG_TYPE_GET, target);
}

/**
 * @brief initiate PUT command
 */
void INenai::initiatePut(const std::string & target)
{
	rawSend(MSG_TYPE_PUT, target);
}

/**
 * @brief initiate CONNECT command
 */
void INenai::initiateConnect(const std::string & target)
{
	rawSend(MSG_TYPE_CONNECT, target);
}

/**
 * @brief initiate BIND command
 */
void INenai::initiateBind(const std::string & target)
{
	rawSend(MSG_TYPE_BIND, target);
}
