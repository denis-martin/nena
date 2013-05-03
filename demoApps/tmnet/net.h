/*
 * net.h
 *
 *  Created on: 7 Jun 2011
 *      Author: denis
 */

#ifndef TMNET_H_
#define TMNET_H_

#include <string>

#include "net_errors.h"

namespace tmnet {

enum error
{
	eOk 						= TMNET_OK,
	eInvalidParameter 			= TMNET_INVALID_PARAMETER,
	eUnsupported 				= TMNET_UNSUPPORTED,
	eSchemeAlreadyRegistered 	= TMNET_SCHEME_ALREADY_REGISTERED,
	eSystemError 				= TMNET_SYSTEM_ERROR,
	eEndOfStream 				= TMNET_ENDOFSTREAM,
	eNetworkError 				= TMNET_NETWORKERROR
};

enum event
{
	evNone 			= 0x00,
	evRequest 		= 0x10,
	evReqConnect 	= evRequest | 0x01,
	evReqGet 		= evRequest | 0x02,
	evReqPut 		= evRequest | 0x03,
};

typedef std::string req_t;

struct request_t
{
	event type;
	std::string uri;
	std::string remote_uri;
};

struct event_t
{
	event type;
};

class nhandle;
typedef nhandle* pnhandle;

///**
// * @brief	Network stream
// */
//class nstream : public std::iostream
//{
//protected:
//	nhandle handle_;
//	std::streambuf* streambuf_;
//
//public:
//	nstream(std::streambuf* buf, nhandle handle) : std::iostream(buf), handle_(handle), streambuf_(buf)  {};
//	virtual ~nstream() { delete streambuf_; };
//
//	inline nhandle handle() { return handle_; };
//};

tmnet::error init();

tmnet::error bind(pnhandle& handle, const std::string& uri, const req_t& req = req_t());
tmnet::error wait(pnhandle handle);
tmnet::error accept(pnhandle handle, pnhandle& incoming);
tmnet::error get_property(pnhandle handle, const std::string& key, std::string& value);

tmnet::error connect(pnhandle& handle, const std::string& uri, const req_t& req = req_t());

tmnet::error close(pnhandle handle);

tmnet::error read(pnhandle handle, char* buf, size_t& size);
tmnet::error write(pnhandle handle, const char* buf, size_t size);

tmnet::error get(pnhandle& handle, const std::string& uri, const req_t& req = req_t());
tmnet::error put(pnhandle& handle, const std::string& uri, const req_t& req = req_t());

//tmnet::error readmsg(pnhandle handle, char* buf, size_t& size);
//tmnet::error writemsg(pnhandle handle, const char* buf, size_t size);

bool isEndOfStream(pnhandle handle);

} // namespace tmnet

#endif /* TMNET_H_ */
