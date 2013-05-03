/*
 * net_nena.cpp
 *
 *  Created on: 26 Jan 2012
 *      Author: Florian Richter, Denis Martin
 */

#include "net_nena.h"

#include "net_debug.h"

#include "nenai.h"
#include "socketNenai.h"

#include <sys/prctl.h>

#include <boost/format.hpp>

namespace tmnet {

using namespace std;

static nena _nena;

// only needed when implemented as shared library
//extern "C"
//plugin::interface* get_interface()
//{
//	return &_nena;
//}

class nena::_request_t : public request_t
{
public:
	event _type;
	std::string _uri;
	std::string _ruri;

	_request_t() : _type(evNone) {}
	_request_t(event type, std::string uri, std::string remoteUri) :
		_type(type), _uri(uri), _ruri(remoteUri) {};

	virtual event request_type() { return _type; }
	virtual std::string request_uri() { return _uri; }
	virtual std::string remote_uri() { return _ruri; }
};

class nena::_event_t : public event_t
{
public:
	MSG_TYPE _type;
	std::string _param;

	_event_t() : _type(MSG_TYPE_NONE) {};
	_event_t(MSG_TYPE type, std::string param) : _type(type), _param(param) {};
	virtual ~_event_t() {};

	virtual bool is_request()
	{
		switch (_type) {
		case MSG_TYPE_GET:
		case MSG_TYPE_PUT:
		case MSG_TYPE_CONNECT:
			return true;
		default:
			return false;
		}
	}

	virtual request_t get_request()
	{
		_request_t r;

		switch (_type) {
		case MSG_TYPE_GET: r._type = evReqGet; break;
		case MSG_TYPE_PUT: r._type = evReqPut; break;
		case MSG_TYPE_CONNECT: r._type = evReqConnect; break;
		default: r._type = evReqConnect; break;
		}

		if (r.request_type() != evNone) {
			r._uri = _param;
		}

		return static_cast<request_t>(r);
	}
};

class nena::_nhandle : public nhandle
{
protected:
	nena* p;

public:
	std::string uri;
	bool closed;
	SocketNenai* nenai;
	bool endOfStream;
	std::string readbuffer;
	long leftInReadbuffer;
	std::string metadata;
	std::string requirements;
	int error; // TODO: Change to proper error type
	std::string networkErrorMsg;

	std::list<_event_t> pendingEvents;

	_nhandle(nena* p) : p(p), closed(false), nenai(NULL), endOfStream(false),
			leftInReadbuffer(0), error(0) {};

	virtual plugin::interface* get_plugin() { return p; };
};

static std::string nena_ipcsocket; // if empty use default socket filename

void nenai_data_callback(nena::_nhandle *h, std::string payload, bool endOfStream)
{
	h->readbuffer = payload;
	h->endOfStream = endOfStream;
	h->leftInReadbuffer = payload.size();

	// unblock
	h->nenai->blocking_stop();
}

void nenai_event_callback(nena::_nhandle *h, MSG_TYPE type, std::string payload)
{
	switch(type) {
	case MSG_TYPE_META: { // deprecated?
		h->metadata = payload;
		break;
	}
	case MSG_TYPE_REQ: { // requirements received
		h->requirements = payload;
		break;
	}
	case MSG_TYPE_ERR: { // requirements received
		h->networkErrorMsg = payload;
		break;
	}
	default: {
//		log((boost::format("tmnet::nenai_event_callback: adding event %1%") % type).str());
		h->pendingEvents.push_back(nena::_event_t(type, payload));
		break;
	}
	}

	// unblock
	h->nenai->blocking_stop();
}

nena::nena()
{

}

nena::~nena()
{

}

nena* nena::get_instance()
{
	return &_nena;
}

std::string nena::get_prname()
{
	char name[17];
	memset(name, 0, 17);

	prctl(PR_GET_NAME, name);

	return string(name);
}

std::string nena::name() const
{
	return "tmnet::nena";
}

tmnet::error nena::init()
{
	log("tmnet::nena::init()");

	plugin::register_for_scheme("default", this);

	return eOk;
}

tmnet::error nena::bind(pnhandle& handle, const std::string& uri, const req_t& req)
{
	log("tmnet::nena::bind()");

	_nhandle *newh = new _nhandle(this);

	// bind first variable of callback to this handle
	boost::function<void(std::string payload, bool endOfStream)> df = boost::bind(&nenai_data_callback, newh, _1, _2);
	boost::function<void(MSG_TYPE type, std::string payload)> mf = boost::bind(&nenai_event_callback, newh, _1, _2);

	try {
		newh->nenai = new SocketNenai("app://" + get_prname(), nena_ipcsocket, df, mf);

	} catch (std::exception& e) {
		log(string("tmnet::nena::bind() failed: ") + e.what());
		delete newh;

		return eSystemError;

	}

	newh->closed = false;
	newh->endOfStream = 0;
	newh->nenai->initiateBind(uri);
	newh->readbuffer = "";
	newh->leftInReadbuffer = 0;
	newh->error = 0;

	newh->requirements = req; // set local requirements

	handle = newh;
	return eOk;
}

tmnet::error nena::wait(pnhandle handle)
{
	log("tmnet::nena::wait()");
	if (!handle) return eInvalidParameter;

	_nhandle *h = static_cast<_nhandle *>(handle);

	// wait for events
	while (h->error == 0 && h->pendingEvents.empty() && h->networkErrorMsg.empty() && !h->endOfStream && h->nenai->getError() == 0) {
		h->nenai->blocking_run();
	}

	if (h->nenai->getError() != 0) {
		return eSystemError;
	}

	if (!h->networkErrorMsg.empty()) {
		return eNetworkError;

	} else {
		return eOk;

	}
}

tmnet::error nena::accept(pnhandle handle, pnhandle& incoming)
{
	log("tmnet::nena::accept()");
	if (!handle) return eInvalidParameter;

	_nhandle *h = static_cast<_nhandle *>(handle);

	if (h->pendingEvents.empty())
		return eInvalidParameter;

	_event_t ev = h->pendingEvents.front();
	h->pendingEvents.pop_front();
	while (ev._type != MSG_TYPE_EVENT_INCOMING && !h->pendingEvents.empty()) {
		cerr << "tmnet::nena::accept(): Error: Unexpected event " << ev._type << endl;
		ev = h->pendingEvents.front();
		h->pendingEvents.pop_front();
	}

	if (ev._type == MSG_TYPE_EVENT_INCOMING) {
		cout << "tmnet::nena::accept(): Got incoming event: " << ev._param << endl;
		return get(incoming, ev._param); // get incoming request

	} else {
		cerr << "tmnet::nena::accept(): Error: Missing incoming event" << endl;
		return eSystemError;

	}
}

tmnet::error nena::get_property(pnhandle handle, const std::string& key, std::string& value)
{
	if (key == "requirements") { // DUPLICATE CODE: get metadata TODO: change?
		log("nena::get_property() - get requirement request");
		_nhandle* h = static_cast<_nhandle *>(handle);
		value = h->requirements;
		return eOk;
	}
	log("nena::get_property() - unknown request");
	return eUnsupported;
}

tmnet::error nena::connect(pnhandle& handle, const std::string& uri, const req_t& req)
{
	log("tmnet::nena::connect()");

	_nhandle *newh = new _nhandle(this);

	// bind first variable of callback to this handle
	boost::function<void(std::string payload, bool endOfStream)> df = boost::bind(&nenai_data_callback, newh, _1, _2);
	boost::function<void(MSG_TYPE type, std::string payload)> mf = boost::bind(&nenai_event_callback, newh, _1, _2);

	newh->closed = false;
	newh->endOfStream = 0;
	newh->nenai = new SocketNenai("app://" + get_prname(), nena_ipcsocket, df, mf);

	// set handle parameters
	newh->requirements = "";
	newh->readbuffer = "";
	newh->error = 0;
	newh->leftInReadbuffer = 0;

	// send messages down to NENA
	newh->nenai->sendRequirements(req); // send requirements
	newh->nenai->initiateConnect(uri);		// initiate CONNECT request

	// wait for requirements/properties from NENA or error
	while (newh->error == 0 && newh->requirements == "" && newh->networkErrorMsg.empty() && !newh->endOfStream) {
		log("tmnet::nena::connect() blocking run");
		newh->nenai->blocking_run();
	}

	//log("tmnet::nena::connect() got requirements or error. Requirements: " + newh->requirements);

	if (newh-> error != 0) { // an error occured, close connection
		log("tmnet::nena::connect() ERROR!");

		delete newh->nenai;
		delete newh;

		return eSystemError;
	}

	if (!newh->networkErrorMsg.empty()) { // an error occured, close connection
		log("tmnet::nena::connect() ERROR!");

		delete newh->nenai;
		delete newh;

		return eNetworkError;
	}

	if (newh->endOfStream) { // an error occured, close connection
		log("tmnet::nena::connect() ERROR!");

		delete newh->nenai;
		delete newh;

		return eEndOfStream;
	}


	handle = newh;
	return eOk;
}

tmnet::error nena::close(pnhandle handle)
{
	if (!handle) return eInvalidParameter;

	_nhandle* h = static_cast<_nhandle *>(handle);
	log("tmnet::nena::close()");

	try {
		h->nenai->sendEndOfStream();
	} catch (...) { // TODO: do it properly!
		delete h->nenai;
		delete h;
		return eSystemError;
	}

	delete h->nenai;
	delete h;

	handle = NULL;

	return eOk;
}

tmnet::error nena::read(pnhandle handle, char* buf, size_t& size)
{
//	log("tmnet::nena::read()");

	_nhandle *h = static_cast<_nhandle *>(handle);

	if (h->leftInReadbuffer > 0) {
		// ok, we've still some data in the buffer, send that first
		size_t offset = h->readbuffer.size() - h->leftInReadbuffer;
		size_t amount_copied = h->readbuffer.copy((char*)buf, size, offset);
		size = amount_copied;
		h->leftInReadbuffer = h->readbuffer.size() - (offset + amount_copied);
		return eOk;
	}

	if (h->endOfStream) {
		// that's it guys
		size = 0;
		return eEndOfStream;
	}

	// wait for data
	while (h->error == 0 && h->networkErrorMsg.empty() && h->leftInReadbuffer == 0 && !h->endOfStream && h->nenai->getError() == 0) {
		h->nenai->blocking_run();
	}

	if (h->nenai->getError() != 0) {
		return eSystemError;
	}

	if (!h->networkErrorMsg.empty())
		return eNetworkError;

	if (h->error != 0)
		return eSystemError;

	if (h->endOfStream) {
		assert(h->leftInReadbuffer == 0);
		size = 0;
		return eEndOfStream;

	}

	// nenai_callback released us from our sleep
	// it should have left us some data
	// deliver them to the application
	long amount_copied = h->readbuffer.copy((char*)buf, size);
	h->leftInReadbuffer = h->readbuffer.size() - amount_copied;
	size = amount_copied;
	return eOk;
}

tmnet::error nena::write(pnhandle handle, const char* buf, size_t size)
{
//	log("tmnet::nena::write()");

	_nhandle *h = static_cast<_nhandle *>(handle);

	if (h == NULL) return eInvalidParameter;
	if (buf == NULL) return eInvalidParameter;
	if (size == 0) return eOk;

	if (!h->networkErrorMsg.empty())
		return eNetworkError;

	if (h->error != 0)
		return eSystemError;

	if (h->endOfStream) {
		assert(h->leftInReadbuffer == 0); // necessary?
		size = 0;
		return eEndOfStream;

	}

	const string payload(buf, size);
	try {
		h->nenai->sendData(payload);
	} catch (...) { // TODO: do it properly!
		return eSystemError;
	}

	if (h->nenai->getError() != 0) {
		return eSystemError;
	}

	return eOk;
}

tmnet::error nena::get(pnhandle& handle, const std::string& uri, const req_t& req)
{
	log("tmnet::nena::get()");

	_nhandle *newh = new _nhandle(this);

	// bind first variable of callback to this handle
	boost::function<void(std::string payload, bool endOfStream)> df = boost::bind(&nenai_data_callback, newh, _1, _2);
	boost::function<void(MSG_TYPE type, std::string payload)> mf = boost::bind(&nenai_event_callback, newh, _1, _2);

	newh->closed = false;
	newh->endOfStream = 0;
	newh->nenai = new SocketNenai("app://" + get_prname(), nena_ipcsocket, df, mf);

	// set handle parameters
	newh->requirements = "";
	newh->readbuffer = "";
	newh->error = 0;
	newh->leftInReadbuffer = 0;

	// send messages down to NENA
	newh->nenai->sendRequirements(req); // send requirements
	newh->nenai->initiateGet(uri);		// initiate GET request

	// wait for requirements/properties from NENA or error
	while (newh->error == 0 && newh->requirements == "" && newh->networkErrorMsg.empty() && !newh->endOfStream) {
		log("tmnet::nena::get() blocking run");
		newh->nenai->blocking_run();
	}

	//log("tmnet::nena::get() got requirements or error. Requirements: " + newh->requirements);

	if (newh-> error != 0) { // an error occured, close connection
		log("tmnet::nena::get() ERROR!");

		delete newh->nenai;
		delete newh;

		return eSystemError;
	}

	if (!newh->networkErrorMsg.empty()) { // an error occured, close connection
		log("tmnet::nena::get() ERROR!");

		delete newh->nenai;
		delete newh;

		return eNetworkError;
	}

	if (newh->endOfStream) { // an error occured, close connection
		log("tmnet::nena::get() ERROR!");

		delete newh->nenai;
		delete newh;

		return eEndOfStream;
	}

	handle = newh;
	return eOk;
}

tmnet::error nena::put(pnhandle& handle, const std::string& uri, const req_t& req)
{
	log("tmnet::nena::put()");

	_nhandle *newh = new _nhandle(this);

	// bind first variable of callback to this handle
	boost::function<void(std::string payload, bool endOfStream)> df = boost::bind(&nenai_data_callback, newh, _1, _2);
	boost::function<void(MSG_TYPE type, std::string payload)> mf = boost::bind(&nenai_event_callback, newh, _1, _2);

	newh->closed = false;
	newh->endOfStream = 0;
	newh->nenai = new SocketNenai("app://" + get_prname(), nena_ipcsocket, df, mf);
	newh->nenai->initiatePut(uri);
	newh->readbuffer = "";
	newh->leftInReadbuffer = 0;
	newh->error = 0;

	handle = newh;
	return eOk;
}

tmnet::error nena::get_metadata(pnhandle handle, const std::string& key, std::string& value)
{
	if (key == "requirements") { // DUPLICATE CODE: get property TODO: change?
		_nhandle* h = static_cast<_nhandle *>(handle);
		value = h->requirements;
		return eOk;
	}
	return eUnsupported;
}

tmnet::error nena::set_plugin_option(const std::string& name, const std::string& value)
{
	if (name == "ipcsocket") {
		nena_ipcsocket = value;
		return eOk;

	} else {
		return eInvalidParameter;

	}
}

bool nena::isEndOfStream(pnhandle handle)
{
	//if (!handle) return eInvalidParameter;
	if (!handle) {
		return true; // TODO: change?
	}
	_nhandle* h = static_cast<_nhandle *>(handle);
	//log("tmnet::nena::isEndOfStream()");

	return h->endOfStream;
}

} // namespace tmnet
