	/*
 * net_plugins.cpp
 *
 *  Created on: 26 Jan 2012
 *      Author: denis
 */

#include "dlfcn.h"

#include "net.h"
#include "net_errors.h"
#include "net_plugin.h"
#include "net_debug.h"

#include "net_nena.h"

#include <list>
#include <map>

namespace tmnet {
namespace plugin {

using namespace std;
using namespace tmnet;

class _handle
{
public:
	void* dlh;
	interface* plugin;

	_handle() : dlh(NULL), plugin(NULL) {};
	virtual ~_handle() {};
};

std::list<_handle> _plugins;
std::map<std::string, interface*> _scheme_handlers;

tmnet::error load(const std::string& filename)
{
	if (filename == "tmnet::nena") {
		interface* p = static_cast<interface*>(nena::get_instance());
		if (!p) return eUnsupported;

		error e = p->init();
		if (e != eOk) return e;

		_handle h;
		h.plugin = p;
		_plugins.push_back(h);
		return eOk;

	}

	void* dl = dlopen(filename.c_str(), RTLD_LAZY | RTLD_LOCAL);
	if (dl) {
		_get_interface* get_interface = (_get_interface*) dlsym(dl, "get_interface");
		if (!get_interface) {
			log("Symbol error loading " + filename + ": " + dlerror());
			dlclose(dl);
			return tmnet::eUnsupported;

		}

		interface* p = get_interface();
		if (!p) {
			dlclose(dl);
			return eUnsupported;

		}

		error e = p->init();
		if (e != TMNET_OK) {
			dlclose(dl);
			return e;

		}

		_handle h;
		h.dlh = dl;
		h.plugin = p;
		_plugins.push_back(h);

	} else {
		log("Error loading " + filename + ": " + dlerror());
		return eUnsupported;

	}

	return eOk;
}

tmnet::error register_for_scheme(const std::string& scheme, interface* plugin)
{
	if (scheme.empty() || plugin == NULL)
		return eInvalidParameter;

	map<string, interface*>::iterator it = _scheme_handlers.find(scheme);
	if (it != _scheme_handlers.end()) {
		tmnet::log("Cannot register " + plugin->name()  + " for " + scheme + ": scheme already used by " + it->second->name());
		return eSchemeAlreadyRegistered;
	}

	_scheme_handlers[scheme] = plugin;
	tmnet::log("URI scheme " + scheme + " registered by " + plugin->name());
	return eOk;
}

interface* get_plugin(const std::string& scheme)
{
	if (scheme.empty()) return NULL;

	map<string, interface*>::iterator it = _scheme_handlers.find(scheme);
	if (it == _scheme_handlers.end()) {
		it = _scheme_handlers.find("default");
		if (it != _scheme_handlers.end())
			return it->second;

		return NULL;

	} else {
		return it->second;

	}
}

tmnet::error get_plugin_from_uri(const std::string& uri, interface*& p)
{
	if (uri.empty()) return eInvalidParameter;

	size_t c = uri.find(':');
	if (c == string::npos) return eInvalidParameter;

	string scheme = uri.substr(0, c);
	if (scheme.empty()) return eInvalidParameter;

	p = get_plugin(scheme);
	if (!p) return eUnsupported;

	return eOk;
}

tmnet::error set_option(const std::string& plugin, const std::string& name, const std::string& value)
{
	list<_handle>::iterator it;
	for (it = _plugins.begin(); it != _plugins.end(); it++) {
		if (it->plugin->name() == plugin) {
			return it->plugin->set_plugin_option(name, value);
		}
	}

	return eInvalidParameter;
}

} // namespace plugin
} // namespace tmnet
