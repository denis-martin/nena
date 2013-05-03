/** @file
 * msg.h
 *
 * @brief Provides General Message Information for all Communication between App and Nodearch
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Aug 30, 2009
 *      Author: Benjamin Behringer
 */

#ifndef _MSG_H_
#define _MSG_H_

#include <string>

/// Message Types
enum MSG_TYPE {
	MSG_TYPE_NONE,
	MSG_TYPE_DATA,					///< payload
	MSG_TYPE_CONNECTORTEST,
	MSG_TYPE_EXTCONNECTORTEST,
	MSG_TYPE_TARGET,				///< default target (deprecated)
	MSG_TYPE_ID,					///< application ID (optional)
	MSG_TYPE_END,					///< end of stream
	MSG_TYPE_META,					///< meta data
	MSG_TYPE_GET,					///< set GET method
	MSG_TYPE_PUT,					///< set PUT method
	MSG_TYPE_DGET,
	MSG_TYPE_DPUT,
	MSG_TYPE_BIND,					///< set BIND method
	MSG_TYPE_CONNECT,				///< set CONNECT method
	MSG_TYPE_USERREQ,
	MSG_TYPE_USER_1,
	MSG_TYPE_CONNECTIONID,
	MSG_TYPE_EVENT_INCOMING,		///< incoming request (only for bind()-handles)
	MSG_TYPE_REQ,					///< requirements
	MSG_TYPE_ERR,					///< (network) error
};


#endif
