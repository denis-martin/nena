/*
 * net_posix.cpp
 *
 *  Created on: 26 Jan 2012
 *      Author: denis
 */

#include "net_posix.h"

#include <iostream>
#include <sstream>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

namespace tmnet {

using namespace std;

static posix _posix;

extern "C"
plugin::interface* get_interface()
{
	return &_posix;
}

class _nhandle : public nhandle
{
protected:
	posix* p;

public:
	std::string uri;

	_nhandle(posix* p) : p(p) {};

	virtual plugin::interface* get_plugin() { return p; };
};

posix::posix()
{

}

posix::~posix()
{

}

std::string posix::get_scheme(const std::string& uri)
{
	// TODO: replace by regex

	if (uri.empty()) return string();

	size_t c = uri.find(':');
	if (c == string::npos) return string();

	string scheme = uri.substr(0, c);
	return scheme;
}

std::string posix::get_domain(const std::string& uri)
{
	// TODO: replace by regex

	if (uri.empty()) return string();

	size_t c = uri.find(':');
	if (c == string::npos) return string();

	if (uri[c+1] == '/' && uri[c+2] == '/')
		c += 2;

	string s = uri.substr(c+1);
	c = s.find(':'); // TODO: broken for IPv6 [::] notation
	if (c == string::npos) return s;

	s = s.substr(0, c);
	return s;
}

std::string posix::get_port(const std::string& uri)
{
	// TODO: replace by regex

	if (uri.empty()) return 0;

	size_t c = uri.find(':');
	if (c == string::npos) return 0;

	if (uri[c+1] == '/' && uri[c+2] == '/')
		c += 2;

	string s = uri.substr(c+1);
	c = s.find(':'); // TODO: broken for IPv6 [::] notation
	if (c == string::npos) return 0;

	s = s.substr(c+1);
	c = s.find('/');
	if (c == string::npos) return atoi(s.c_str());

	return s.substr(0, c);;
}

std::string posix::name() const
{
	return "tmnet::posix";
}

tmnet::error posix::init()
{
	log("tmnet::posix::init()");

	plugin::register_for_scheme("udp", this);
	plugin::register_for_scheme("tcp", this);

	return eOk;
}

tmnet::error posix::bind(pnhandle& handle, const std::string& uri, const req_t& req)
{
	string scheme = get_scheme(uri);
	string domain = get_domain(uri);
	string port = get_port(uri);

	struct addrinfo hints;
	struct addrinfo* ai = NULL;

	memset(&hints, 0, sizeof(hints));

	if (scheme == "tcp") {
		hints.ai_family = AF_UNSET; // v4 or v6
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

	} else if (scheme == "udp") {
		hints.ai_family = AF_UNSET; // v4 or v6
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;

	}

	int r = getaddrinfo(domain.c_str(), port.c_str(), &hints, &ai);
	if (r != 0) {
		if (ai) {
			freeaddrinfo(ai);

		}

		stringstream ss;
		ss << "getaddrinfo() failed: " << r << " (" << gai_strerror(r) << ")" << endl;
		log(ss.str());
		return eUnsupported;

	}


	if (scheme == "tcp") {
		sd = socket(PF_INET, SOCK_STREAM, 0);

	}

	_nhandle* h = new _nhandle(this);
	h->uri = uri;
	handle = h;

	// TODO
	stringstream ss;
	ss << "posix::bind(): " << scheme << ", " << domain << ", " << port;
	log(ss.str());

	return eOk;
}

tmnet::error posix::listen(pnhandle handle, pnhandle& incoming)
{
	return eUnsupported;
}

tmnet::error posix::listen(pnhandle handle)
{
	return eUnsupported;
}

tmnet::error posix::accept(pnhandle handle, pnhandle& incoming)
{
	return eUnsupported;
}

tmnet::error posix::connect(pnhandle& handle, const std::string& uri, const req_t& req)
{
	return eUnsupported;
}

tmnet::error posix::close(pnhandle handle)
{
	if (!handle) return eInvalidParameter;

	_nhandle* h = (_nhandle*) handle;
	log("posix::close()");

	delete h;

	return eOk;
}

tmnet::error posix::read(pnhandle handle, char* buf, size_t& size)
{
	return eUnsupported;
}

tmnet::error posix::write(pnhandle handle, const char* buf, size_t size)
{
	return eUnsupported;
}

tmnet::error posix::get(pnhandle& handle, const std::string& uri, const req_t& req)
{
	return eUnsupported;
}

tmnet::error posix::put(pnhandle& handle, const std::string& uri, const req_t& req)
{
	return eUnsupported;
}

tmnet::error posix::get_metadata(pnhandle handle, const std::string& key, std::string& value)
{
	return eUnsupported;
}

} // namespace tmnet
