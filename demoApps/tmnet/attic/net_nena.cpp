/*
 * ani_nena.cpp
 *
 *  Created on: 7 Jun 2011
 *      Author: denis
 */

#include "net_nena.h"

#include "nenai.h"
#include "socketNenai.h"
#include "memNenai.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <sys/prctl.h>

namespace kit  {
namespace tm  {
namespace net {
namespace nena {

using namespace std;

static ipctype nena_ipctype = ipctype_socket;
static string nena_ipcsocket = "boost_sock";

/*****************************************************************************/

/**
 * @brief 	Stream buffer
 *
 * 			Based on http://forums.fedoraforum.org/showthread.php?t=222765
 */
class netbuf : public std::basic_streambuf<char, std::char_traits<char> >
{
public:
	netbuf(const string& id, const string& target, int buf_in_size = 256, int buf_out_size = 0);
	virtual ~netbuf();

	inline INenai* nenai() { return nenai_; };

	// receives data from the network stack
	// returns the amount of data read
	int recv(char *, int);

	// sends data to the network stack
	// returns the amount of data sent
	int send(const char *, int);

protected:
	virtual int underflow();
	virtual int overflow(int = _Tr::eof());
	virtual int sync();

private:
	// pointers to the buffers
	char* gbuf;
	char* pbuf;
	int gmax;
	int pmax;
	typedef std::char_traits<char> _Tr;

	INenai* nenai_;

	// async receive buffers
	boost::mutex rmutex;
	boost::condition_variable rcond;
	std::list<std::string> rbufs;
	std::size_t rptr; // current position in first string
	bool endOfStream;

	void nenai_callback(std::string payload, bool endOfStream);
};

/* ani_nena_netbuf ***********************************************************/

netbuf::netbuf(const string& id, const string& target, int buf_in_size, int buf_out_size) {
	this->gmax = buf_in_size;
	this->pmax = ((buf_out_size < 1) ? buf_in_size : buf_out_size);

	this->gbuf = new char[this->gmax];
	this->pbuf = new char[this->pmax];

	// set all pointers to the beginning of the array(s)
	setg(this->gbuf, this->gbuf, this->gbuf);
	setp(this->pbuf, this->pbuf+this->pmax-1);

	// receive buffers
	this->rptr = 0;
	this->endOfStream = false;

	boost::function<void(string payload, bool endOfStream)> f = boost::bind(&netbuf::nenai_callback, this, _1, _2);

	try {
		// TODO: not nice to throw exceptions in constructors...
		switch (nena_ipctype) {
		case ipctype_socket:
			nenai_ = new SocketNenai(id, target, nena_ipcsocket, f);
			break;
		case ipctype_memory:
			nenai_ = new MemNenai(id, target, f);
			break;
		default:
			assert(false);
			break;
		}

	} catch (std::exception) {
		delete [] this->gbuf;
		delete [] this->pbuf;
		this->gbuf = NULL;
		this->pbuf = NULL;

		throw;
	}

	nenai_->run();

//	cout << "nena::netbuf::netbuf()" << endl;
}

netbuf::~netbuf() {
	this->sync();

	delete nenai_;

	delete [] this->gbuf;
	delete [] this->pbuf;

//	cout << "nena::netbuf::~netbuf()" << endl;
}

int netbuf::underflow() {
//	cout << "nena::netbuf::underflow()" << endl;

	// if the end pointer is maxed out, reset the pointers to the start of the array.
	if (this->egptr() >= this->gbuf + this->gmax)
		setg(this->gbuf, this->gbuf, this->gbuf);

	// attempt to read as much data as possible from the source as we can fit in the array.
	int i = netbuf::recv(this->egptr(), this->gmax - (this->gptr()-this->gbuf));

	// set the end pointer to end+i;
	setg(this->eback(), this->gptr(), this->egptr() + i);

	// return the char now pointed to by the current pointer or eof.
	return (i <= 0 ? _Tr::eof() : *this->gptr());
}

int netbuf::overflow(int c) {
	int length = this->pptr()-this->pbuf;
	// assign c to the char pointed to by the current ptr
	if (c != _Tr::eof()) {
		*this->pptr() = c;
		++length;
	}
	// send all the data to 1 past the current pointer to the data sink
	netbuf::send(this->pbuf, length);

	// reset the pointers
	setp(this->pbuf, this->pbuf+this->pmax-1);

	// return 1 for success, EOF otherwise
	return c;
}

int netbuf::sync(void) {
	// if the output buffer has content, call overflow.
	if (this->pbuf != this->pptr())
		this->overflow();
	return 0;
}

void netbuf::nenai_callback(std::string payload, bool endOfStream)
{
//	cout << "nena::netbuf::nenai_callback()" << endl;

	if (payload.size() > 0) {
		boost::unique_lock<boost::mutex> lock(rmutex);
		rbufs.push_back(payload);
		this->endOfStream = endOfStream;
	}

//	cout << "nena::netbuf::nenai_callback(): notifying on " << payload.size() << " bytes " << endl;
	rcond.notify_one();
}

int netbuf::recv(char * buffer, int size)
{
//	cout << "nena::netbuf::recv()" << endl;

	boost::unique_lock<boost::mutex> lock(rmutex);

	// block until some data is availble
	while (rbufs.size() == 0 && !endOfStream)
		rcond.wait(lock);

	int ret = 0;
	int i = 0;
	while (size > 0) {
		if (rbufs.size() == 0)
			break;

		string& s = rbufs.front();

		buffer[i++] = s[rptr++];
		size--;
		ret++;

		if (rptr == s.size()) {
			rbufs.pop_front();
			rptr = 0;
		}
	}

//	cout << "nena::netbuf::recv(): received " << ret << " bytes" << endl;

	return ret;
}

int netbuf::send(const char * buffer, int size)
{
//	cout << "nena::netbuf::send()" << endl;

	string data(buffer, size);
	nenai_->sendData(data);

	return size;
}

/*********************************************************************/

void set_ipctype(ipctype type)
{
	nena_ipctype = type;
}

void set_ipcsocket(const std::string& filename)
{
	nena_ipcsocket = filename;
}

string get_prname()
{
	char name[17];
	memset(name, 0, 17);

	prctl(PR_GET_NAME, name);

	return string(name);
}

nstream* get(const std::string& uri, void* req)
{
	cout << "nena::get(\"" << uri << "\")" << endl;

	netbuf* buf = new netbuf("app://" + get_prname(), uri);
	buf->nenai()->initiateGet(uri);

	nstream* ret = new nstream(buf, 0);

	return ret;
}

nstream* put(const std::string& uri, void* req)
{
	cout << "nena::put(\"" << uri << "\")" << endl;

	netbuf* buf = new netbuf("app://" + get_prname(), uri);
	buf->nenai()->initiatePut(uri);

	nstream* ret = new nstream(buf, 0);

	return ret;
}

nstream* connect(const std::string& uri, void* req)
{
	cout << "nena::connect(\"" << uri << "\")" << endl;

	netbuf* buf = new netbuf("app://" + get_prname(), uri);
	buf->nenai()->initiateConnect(uri);

	nstream* ret = new nstream(buf, 0);

	return ret;
}

nhandle publish(const std::string& uri, void* req)
{
	cout << "nena::publish(\"" << uri << "\")" << endl;

	netbuf* buf = new netbuf("app://" + get_prname(), uri);
	buf->nenai()->initiateConnect(uri);

	// TODO
	assert(false);

	return 0;
}

}
}
}
}
