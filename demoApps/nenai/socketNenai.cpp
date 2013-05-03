#include "socketNenai.h"
#include "../../src/targets/boost/msg.h"
#include <cstring>
#include <string>
#include <exception>

#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

#include <sys/types.h>

#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>
#include <boost/function.hpp>
#include <boost/thread/thread.hpp>

#define NENA_DEFAULT_SOCKET	"/tmp/nena_socket_default"

/// after tests, 128K is better (max throughput)
#define BOOSTSOCK_SYSRECVBUFSIZE	131072		///< local socket receive buffer size
#define BOOSTSOCK_SYSSENDBUFSIZE	131072		///< local socket send buffer size

using std::string;
using boost::function;

using boost::asio::local::stream_protocol;

void empty_event_fkt(MSG_TYPE type, string payload)
{
}


SocketNenai::SocketNenai (string id, string path, function<void(string payload, bool endOfStream)> data_fkt, function<void(MSG_TYPE type, string payload)> event_fkt):
	identifier(id), connection_id(0), error(0), buffer(NULL),
	ondata_callback(data_fkt), onevent_callback(event_fkt),
	s(io_service), worker_thread(NULL)
{
	boost::asio::socket_base::receive_buffer_size sopt_rbuf(BOOSTSOCK_SYSRECVBUFSIZE);
	boost::asio::socket_base::send_buffer_size sopt_sbuf(BOOSTSOCK_SYSSENDBUFSIZE);

	if (identifier.empty())
		identifier = "app://unknown";

	if (path.empty())
		path = NENA_DEFAULT_SOCKET;

	// TODO: exceptions in constructors are evil...
	if (ondata_callback == NULL) throw std::exception();
	if (onevent_callback == NULL) throw std::exception();

	// establish connection (might throw)
	s.connect(stream_protocol::endpoint(path));

	// set application identifier
	setID(identifier);

	// start async read
	boost::asio::async_read(s, boost::asio::buffer(reinterpret_cast<char *>(header), 2*sizeof(uint32_t)), boost::asio::transfer_all(),
			boost::bind (&SocketNenai::handle_header, this, boost::asio::placeholders::error));
}

void SocketNenai::rawSend (MSG_TYPE type, const string & payload)
{
	uint32_t * sheader = new uint32_t[2];
	sheader[0] = type;
	sheader[1] = static_cast<uint32_t> (payload.length ());
	if (s.is_open()) {
		s.send (boost::asio::buffer (sheader, 2*sizeof(uint32_t)));
		boost::asio::write (s, boost::asio::buffer(payload.c_str (), payload.length ()), boost::asio::transfer_all());
	} else {
		cerr << "SocketNenai::rawSend: Error, socket not open.\n";
		error = 1;
	}
}

/**
 * @brief receives header of incoming messages
 */
void SocketNenai::handle_header (const boost::system::error_code& error)
{
	if (!error) {
		buffer = new char[header[1]];

//		std::cout << "Received header from nad, indicating data with size " << header[1] << std::endl;

		boost::asio::async_read (s, boost::asio::buffer(buffer, header[1]), boost::asio::transfer_all(),
             boost::bind (&SocketNenai::handle_body, this, boost::asio::placeholders::error));

	} else {
		cerr << "Receive error (" << error.value() << "): " << error.message() << endl;
		this->error = error.value();
	}
}

/**
 * @brief receives body of incoming messages and processes them
 */
void SocketNenai::handle_body (const boost::system::error_code& error)
{
	if (!error) {
		switch (header[0]) {

		case MSG_TYPE_DATA:
		case MSG_TYPE_END: {
			string payload(buffer, header[1]);

			ondata_callback(payload, header[0] == MSG_TYPE_END);
			delete [] buffer;
			buffer = NULL;

			if (header[0] != MSG_TYPE_END) {
				boost::asio::async_read (s, boost::asio::buffer(reinterpret_cast<char *> (header), 2*sizeof(uint32_t)), boost::asio::transfer_all(),
					 boost::bind (&SocketNenai::handle_header, this, boost::asio::placeholders::error));

			}

			break;
		}
		case MSG_TYPE_EVENT_INCOMING:
		case MSG_TYPE_ERR:
		case MSG_TYPE_REQ:
		case MSG_TYPE_META: {
			string payload;

			if (header[1] > 0)
				payload.assign(buffer, header[1]);

			onevent_callback((MSG_TYPE) header[0], payload);
			delete [] buffer;
			buffer = NULL;

			boost::asio::async_read (s, boost::asio::buffer(reinterpret_cast<char *> (header), 2*sizeof(uint32_t)), boost::asio::transfer_all(),
					 boost::bind (&SocketNenai::handle_header, this, boost::asio::placeholders::error));

			break;
		}
		case MSG_TYPE_CONNECTIONID: {
			connection_id = *(unsigned int*) buffer;
			std::cout << "got connection id " << connection_id << "\n";

			delete [] buffer;
			buffer = NULL;

			boost::asio::async_read (s, boost::asio::buffer(reinterpret_cast<char *> (header), 2*sizeof(uint32_t)), boost::asio::transfer_all(),
					 boost::bind (&SocketNenai::handle_header, this, boost::asio::placeholders::error));

			break;
		}
		default: {
			cerr << "Received unrecognized command " << header[0] << endl;
			break;
		}

		}

	} else {
		cerr << "Receive error (" << error.value() << "): " << error.message() << endl;
		this->error = error.value();
	}
}

/// just starts io, blocks
void SocketNenai::blocking_run ()
{
	io_service.reset ();
	io_service.run ();
}

void SocketNenai::blocking_stop ()
{
	io_service.stop ();
}

void SocketNenai::run ()
{
	worker_thread = new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service));
}

void SocketNenai::stop ()
{
	io_service.stop();
	worker_thread->join ();
	delete worker_thread;
	worker_thread = NULL;
}

unsigned int SocketNenai::getConnectionId() {
	return connection_id;
}

unsigned int SocketNenai::getError() {
	return error;
}

SocketNenai::~SocketNenai ()
{
	//string payload;
	//this->rawSend (MSG_TYPE_END, payload);

	if (worker_thread != NULL)
		this->stop ();

	s.close();
}

