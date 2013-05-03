/*
 * net_nena.h
 *
 *  Created on: 26 Jan 2012
 *      Author: Florian Richter, Denis Martin
 */

#ifndef NET_NENA_H_
#define NET_NENA_H_

#include "net_plugin.h"

namespace tmnet {

class nena : public plugin::interface
{
public:
	class _event_t;		// not for external use
	class _request_t;	// not for external use
	class _nhandle; 	// not for external use

protected:
	std::string get_prname();

public:
	nena();
	virtual ~nena();

	static nena* get_instance();

	virtual std::string name() const;

	virtual tmnet::error init();

	virtual tmnet::error bind(pnhandle& handle, const std::string& uri, const req_t& req);
	virtual tmnet::error wait(pnhandle handle);
	virtual tmnet::error accept(pnhandle handle, pnhandle& incoming);
	virtual tmnet::error get_property(pnhandle handle, const std::string& key, std::string& value);

	virtual tmnet::error connect(pnhandle& handle, const std::string& uri, const req_t& req = req_t());

	virtual tmnet::error close(pnhandle handle);

	virtual tmnet::error read(pnhandle handle, char* buf, size_t& size);
	virtual tmnet::error write(pnhandle handle, const char* buf, size_t size);

	virtual tmnet::error get(pnhandle& handle, const std::string& uri, const req_t& req = req_t());
	virtual tmnet::error put(pnhandle& handle, const std::string& uri, const req_t& req = req_t());

	virtual tmnet::error get_metadata(pnhandle handle, const std::string& key, std::string& value);

	virtual tmnet::error set_plugin_option(const std::string& name, const std::string& value);

	// ...
	virtual bool isEndOfStream(pnhandle handle);
};

} // namespace tmnet

#endif /* NET_NENA_H_ */
