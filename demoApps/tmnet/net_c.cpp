/*
 * net_c.cpp
 *
 *  Created on: 26 Jan 2012
 *      Author: denis
 */

#include "net_c.h"

#include "net.h"
#include "net_plugin.h"

#include <iostream>

#include "string.h"

using namespace std;

struct tmnet_nhandle
{
	tmnet::pnhandle _h;
	string incoming;
};



extern "C"
int tmnet_init()
{
	return (int) tmnet::init();
}

extern "C"
int tmnet_bind(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req)
{
	if (!handle || !uri) return TMNET_INVALID_PARAMETER;

	*handle = NULL;
	tmnet::pnhandle h = NULL;
	tmnet::error e = tmnet::eOk;

	if (req) {
		tmnet::bind(h, uri, req);
	} else {
		tmnet::bind(h, uri);
	}

	if (e == tmnet::eOk) {
		*handle = new tmnet_nhandle;
		(*handle)->_h = h;
	}
	return (int) e;
}

extern "C"
int tmnet_wait(tmnet_pnhandle handle)
{
	if (!handle || !handle->_h) return TMNET_INVALID_PARAMETER;

	return (int) tmnet::wait(handle->_h);
}

extern "C"
int tmnet_accept(tmnet_pnhandle handle, tmnet_pnhandle* incoming)
{
	if (!handle || !handle->_h || incoming == NULL) return TMNET_INVALID_PARAMETER;

	*incoming = NULL;
	tmnet::pnhandle inc = NULL;
	tmnet::error e = tmnet::eOk;

	e = tmnet::accept(handle->_h, inc);
	if (e == tmnet::eOk) {
		*incoming = new tmnet_nhandle;
		(*incoming)->_h = inc;

	}

	return (int) e;
}

extern "C"
int tmnet_get_property(tmnet_pnhandle handle, const char* key, char** value)
{
	if (handle == NULL || value == NULL || key == NULL) {
		return TMNET_INVALID_PARAMETER;
	}
	std::string tmpVal;
	tmnet::error e = tmnet::get_property(handle->_h, key, tmpVal);
	*value = new char[tmpVal.size()+1];
	strncpy(*value, tmpVal.c_str(), tmpVal.size()+1);
	//cout << "get_property (C-API): " << tmpVal << " " << *value << "\n";
	return (int)e;

}

extern "C"
int tmnet_free_property(char* value)
{
	if (value == NULL) {
		return TMNET_INVALID_PARAMETER;
	}

	delete [] value;
	cout << "free_property: value deleted.\n";
	return TMNET_OK;

}


extern "C"
int tmnet_connect(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req)
{
	*handle = NULL;
	tmnet::pnhandle h = NULL;
	tmnet::error e = tmnet::eOk;

	if (req) {
		e = tmnet::connect(h, uri, req);
	} else {
		e = tmnet::connect(h, uri);
	}

	if (e == tmnet::eOk) {
		*handle = new tmnet_nhandle;
		(*handle)->_h = h;
	}
	return (int) e;
}

extern "C"
int tmnet_read(tmnet_pnhandle handle, char* buf, size_t* size)
{
	if (handle == NULL) return TMNET_INVALID_PARAMETER;
	return (int) tmnet::read(handle->_h, buf, *size);
}

extern "C"
int tmnet_write(tmnet_pnhandle handle, const char* buf, size_t size)
{
	if (handle == NULL) return TMNET_INVALID_PARAMETER;
	return (int) tmnet::write(handle->_h, buf, size);
}

extern "C"
int tmnet_readmsg(tmnet_pnhandle handle, char* buf, size_t* size)
{
	return TMNET_UNSUPPORTED;
}

extern "C"
int tmnet_writemsg(tmnet_pnhandle handle, const char* buf, size_t size)
{
	return TMNET_UNSUPPORTED;
}

extern "C"
int tmnet_close(tmnet_pnhandle handle)
{
	if (handle == NULL) return TMNET_INVALID_PARAMETER;

	tmnet::error e = tmnet::close(handle->_h);
	if (e == tmnet::eOk) {
		delete handle;
	}

	return (int) e;
}

// optional

extern "C"
int tmnet_get(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req)
{
	*handle = NULL;
	tmnet::pnhandle h = NULL;
	tmnet::error e = tmnet::eOk;

	if (req) {
		e = tmnet::get(h, uri, req);
	} else {
		e = tmnet::get(h, uri);
	}

	if (e == tmnet::eOk) {
		*handle = new tmnet_nhandle;
		(*handle)->_h = h;
	}
	return (int) e;
}

extern "C"
int tmnet_put(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req)
{
	*handle = NULL;
	tmnet::pnhandle h = NULL;
	tmnet::error e = tmnet::eOk;

	if (req) {
		e = tmnet::put(h, uri, req);
	} else {
		e = tmnet::put(h, uri);
	}

	if (e == tmnet::eOk) {
		*handle = new tmnet_nhandle;
		(*handle)->_h = h;
	}
	return (int) e;
}

extern "C"
int tmnet_set_option(tmnet_pnhandle handle, const char* name, const char* value)
{
	return (int) tmnet::eUnsupported;
}

extern "C"
int tmnet_set_plugin_option(const char* plugin, const char* name, const char* value)
{
	return (int) tmnet::plugin::set_option(plugin, name, value);
}

extern "C"
int tmnet_isEndOfStream(tmnet_pnhandle handle)
{
	if (handle == NULL) return true;

	bool eos = tmnet::isEndOfStream(handle->_h);
	return (int)eos;
}
