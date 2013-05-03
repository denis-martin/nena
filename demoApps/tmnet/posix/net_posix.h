/*
 * net_posix.h
 *
 *  Created on: 26 Jan 2012
 *      Author: denis
 */

#ifndef NET_POSIX_H_
#define NET_POSIX_H_

#include "net_plugin.h"

namespace tmnet {

class posix : public plugin::interface
{
protected:
	std::string get_scheme(const std::string& uri);
	std::string get_domain(const std::string& uri);
	std::string get_port(const std::string& uri);

public:
	posix();
	virtual ~posix();

	virtual std::string name() const;

	virtual tmnet::error init();
	virtual tmnet::error bind(pnhandle& handle, const std::string& uri, const req_t& req);

	virtual tmnet::error listen(pnhandle handle, pnhandle& incoming);
	virtual tmnet::error listen(pnhandle handle);

	virtual tmnet::error accept(pnhandle handle, pnhandle& incoming);

	virtual tmnet::error connect(pnhandle& handle, const std::string& uri, const req_t& req = req_t());

	virtual tmnet::error close(pnhandle handle);

	virtual tmnet::error read(pnhandle handle, char* buf, size_t& size);
	virtual tmnet::error write(pnhandle handle, const char* buf, size_t size);

	virtual tmnet::error get(pnhandle& handle, const std::string& uri, const req_t& req = req_t());
	virtual tmnet::error put(pnhandle& handle, const std::string& uri, const req_t& req = req_t());

	virtual tmnet::error get_metadata(pnhandle handle, const std::string& key, std::string& value);

	// ...
};

} // namespace tmnet

#endif /* NET_POSIX_H_ */
