/*
 * net_plugins.h
 *
 *  Created on: 26 Jan 2012
 *      Author: denis
 */

#ifndef NET_PLUGINS_H_
#define NET_PLUGINS_H_

#include "net.h"

#include <string>

namespace tmnet {
namespace plugin {

class interface
{
public:
	interface() {};
	virtual ~interface() {};

	virtual std::string name() const = 0;

	virtual tmnet::error init() = 0;

	virtual tmnet::error bind(pnhandle& handle, const std::string& uri, const req_t& req) = 0;
	virtual tmnet::error wait(pnhandle handle) = 0;
	virtual tmnet::error accept(pnhandle handle, pnhandle& incoming) = 0;
	virtual tmnet::error get_property(pnhandle handle, const std::string& key, std::string& value) = 0;

	virtual tmnet::error connect(pnhandle& handle, const std::string& uri, const req_t& req = req_t()) = 0;

	virtual tmnet::error close(pnhandle handle) = 0;

	virtual tmnet::error read(pnhandle handle, char* buf, size_t& size) = 0;
	virtual tmnet::error write(pnhandle handle, const char* buf, size_t size) = 0;

	virtual tmnet::error get(pnhandle& handle, const std::string& uri, const req_t& req = req_t()) = 0;
	virtual tmnet::error put(pnhandle& handle, const std::string& uri, const req_t& req = req_t()) = 0;

	virtual tmnet::error set_plugin_option(const std::string& name, const std::string& value) = 0;

	// ...
	virtual bool isEndOfStream(pnhandle handle) = 0;
};

typedef interface* (_get_interface)();

tmnet::error load(const std::string& filename);
tmnet::error register_for_scheme(const std::string& scheme, interface* plugin);
interface* get_plugin(const std::string& scheme);
tmnet::error get_plugin_from_uri(const std::string& uri, interface*& p);

tmnet::error set_option(const std::string& plugin, const std::string& name, const std::string& value);

} // namespace plugin
} // namespace tmnet

namespace tmnet {

class nhandle
{
public:
	virtual plugin::interface* get_plugin() = 0;
};

}

#endif /* NET_PLUGINS_H_ */
