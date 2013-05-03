/*
 * net.cpp
 *
 *  Created on: 7 Jun 2011
 *      Author: denis
 */

#include "net.h"
#include "net_plugin.h"
#include "net_debug.h"

#include <iostream>
#include <fstream>

//#include <iostream>

namespace tmnet {

using namespace std;

std::string logFileName = "tmnet.log"; //filename of tmnet log
std::ofstream logFile; // output stream for tmnet log

void log(const std::string& message)
{
	//cerr << "tmnet: " << message << endl;
	if (logFile.good()) {
		logFile << "tmnet: " << message << endl;
	} else {
		cerr << "tmnet: " << message << endl;
	}
}

tmnet::error init()
{
	tmnet::error e;

	// discontinued
//	e = tmnet::plugin::load("libtmnetposix.so");
//	if (e != tmnet::eOk) {
//		tmnet::log("Error opening libtmnetposix.so");
//	}

	e = tmnet::plugin::load("tmnet::nena");
	if (e != tmnet::eOk) {
		tmnet::log("Error initializing tmnet::nena");
	}

	logFile.open(logFileName.c_str()); // TODO: clean way to close it?

	return eOk;
}

tmnet::error bind(pnhandle& handle, const std::string& uri, const req_t& req)
{
	plugin::interface* p;
	tmnet::error e = plugin::get_plugin_from_uri(uri, p);
	if (e != eOk) {
		return e;

	} else {
		return p->bind(handle, uri, req);

	}
}

tmnet::error wait(pnhandle handle)
{
	if (!handle) return eInvalidParameter;
	return handle->get_plugin()->wait(handle);
}

tmnet::error accept(pnhandle handle, pnhandle& incoming)
{
	if (!handle) return eInvalidParameter;
	return handle->get_plugin()->accept(handle, incoming);
}

tmnet::error get_property(pnhandle handle, const std::string& key, std::string& value)
{
	if (!handle || key.empty()) return eInvalidParameter;
	return handle->get_plugin()->get_property(handle, key, value);
}

tmnet::error connect(pnhandle& handle, const std::string& uri, const req_t& req)
{
	plugin::interface* p;
	tmnet::error e = plugin::get_plugin_from_uri(uri, p);
	if (e != eOk) {
		return e;

	} else {
		return p->connect(handle, uri, req);

	}
}

tmnet::error close(pnhandle handle)
{
	if (!handle) return eInvalidParameter;
	return handle->get_plugin()->close(handle);
}

tmnet::error read(pnhandle handle, char* buf, size_t& size)
{
	if (!handle) return eInvalidParameter;
	return handle->get_plugin()->read(handle, buf, size);
}

tmnet::error write(pnhandle handle, const char* buf, size_t size)
{
	if (!handle) return eInvalidParameter;
	return handle->get_plugin()->write(handle, buf, size);
}

tmnet::error readmsg(pnhandle handle, char* buf, size_t& size)
{
	return eUnsupported;
}

tmnet::error writemsg(pnhandle handle, const char* buf, size_t size)
{
	return eUnsupported;
}

tmnet::error get(pnhandle& handle, const std::string& uri, const req_t& req)
{
	plugin::interface* p;
	tmnet::error e = plugin::get_plugin_from_uri(uri, p);
	if (e != eOk) {
		return e;

	} else {
		return p->get(handle, uri, req);

	}
}

tmnet::error put(pnhandle& handle, const std::string& uri, const req_t& req)
{
	plugin::interface* p;
	tmnet::error e = plugin::get_plugin_from_uri(uri, p);
	if (e != eOk) {
		return e;

	} else {
		return p->put(handle, uri, req);

	}
}

bool isEndOfStream(pnhandle handle)
{
	if (!handle) return true;
	return handle->get_plugin()->isEndOfStream(handle);
}

}
