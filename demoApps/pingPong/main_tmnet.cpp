/** @file
 *
 * Simple tool for testing communication via the Node Architecture
 *
 */

#include "../tmnet/net_c.h"

#include <iostream>
#include <iterator>

#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>

using namespace std;
using namespace boost;

const string& APP_ID = "app://tm.kit.edu/itm/nena/demoApp/pingPong";

// configuration
string socketPath = "/tmp/nena_socket_default";
unsigned int interval = 1000;
bool server = false;
bool reliable = false;
bool unreliable = false;
int statInterval = 10; // seconds

// run-time variables
string destination = "app://localhost.node02/ping_tmnet";
unsigned short seqNo = 0;
map<unsigned short, posix_time::ptime> sentMap;

// statistics
posix_time::ptime stat_timeStart;
unsigned long stat_sent = 0;
unsigned long stat_rcvd = 0;
double stat_rtt_min = 0.0;
double stat_rtt_avg = 0.0;
double stat_rtt_max = 0.0;
double stat_rtt_mdev = 0.0;
double stat_rtt_sum = 0.0;

/**
 * @brief	Ping header
 */
typedef struct {
	unsigned short seqNo;
} __attribute__((packed)) PingHeader;

/**
 * @brief	Print some statistics from the ping server's point of view
 */
void printServerStats()
{
	posix_time::ptime now = posix_time::microsec_clock::local_time();
	posix_time::time_duration diff = now - stat_timeStart;

	cout << "--- server statistics ---" << endl;
	cout << stat_rcvd << " packets received, ";
	cout << "time " << diff.total_milliseconds() << "ms" << endl;
}

/**
 * @brief	Print some statistics about the ping session
 */
void printPingStats()
{
	posix_time::ptime now = posix_time::microsec_clock::local_time();
	posix_time::time_duration diff = now - stat_timeStart;

	cout << "--- " << destination << " ping statistics ---" << endl;
	cout << stat_sent << " packets transmitted, ";
	cout << stat_rcvd << " received, ";
	cout << 100 - stat_rcvd*100 / stat_sent << "% packet loss, ";
	cout << "time " << diff.total_milliseconds() << "ms" << endl;
}

/**
 * @brief	Server loop
 */
void doServer()
{
	cout << "PING server mode" << endl;
	stat_timeStart = posix_time::microsec_clock::local_time();

	tmnet_pnhandle handle;
	int e = tmnet_bind(&handle, destination.c_str(), NULL);
	if (e != TMNET_OK) {
		cerr << "Error (bind): " << e << endl;

	}

	for (;;) {
		e = tmnet_wait(handle);
		if (e == TMNET_OK) {
			tmnet_pnhandle inch = NULL;
			e = tmnet_accept(handle, &inch);
			if (e == TMNET_OK) {
				destination = "node://localhost/node01"; // TODO
				PingHeader* hdr;
				size_t s = sizeof(1024);
				char buf[s];
				e = tmnet_read(inch, buf, &s);
				if (e == TMNET_OK) {
					hdr = (PingHeader *) buf;
					cout << s << " bytes from " << destination << ": seq=" << hdr->seqNo << endl;
					stat_rcvd++;

					e = tmnet_write(inch, buf, sizeof(PingHeader));
					if (e != TMNET_OK) {
						cerr << "Error (write): " << e << endl;
					}

					printServerStats();

				} else if (e == TMNET_NETWORKERROR) {
					cerr << "Error (read): " << e << " (network error)" << endl;

				} else {
					cerr << "Error (read): " << e << endl;

				}

				tmnet_close(inch);

			} else {
				cerr << "Error (get): " << e << endl;

			}


		} else {
			cerr << "Error (wait): " << e << endl;
			exit(1);

		}

	}

}

/**
 * @brief	Ping loop
 */
void doPing()
{
	cout << "PING " << destination << " " << sizeof(PingHeader) << " bytes of data." << endl;
	stat_timeStart = posix_time::microsec_clock::local_time();
	posix_time::ptime lastStatTime = stat_timeStart;

	tmnet_pnhandle handle;
	string req;
	if (reliable)
		req = "{ \"reliable\": 1 }";
	else if (unreliable)
		req = "{ \"reliable\": 0 }";

	for (;;) {
		int e = tmnet_connect(&handle, destination.c_str(), (const tmnet_req_t) req.c_str());
		if (e != TMNET_OK) {
			cerr << "Error (connect): " << e << endl;
			return;

		}

		PingHeader hdr;
		hdr.seqNo = ++seqNo;

		string payload((char*) &hdr, sizeof(PingHeader));
		assert(payload.size() == sizeof(PingHeader));

		posix_time::ptime timeSent = posix_time::microsec_clock::local_time();
		sentMap[seqNo] = timeSent;

		e = tmnet_write(handle, payload.c_str(), payload.size());
		if (e != TMNET_OK) {
			cerr << "Error sending ping request (write): " << e << endl;
		}

		stat_sent++;

		// TODO: read with timeout
		size_t s = sizeof(hdr);
		e = tmnet_read(handle, (char*) &hdr, &s);
		if (e != TMNET_OK || s != sizeof(hdr)) {
			cerr << "Error waiting for ping-echo (read): " << e << endl;

		} else {
			cout << s << " bytes from " << destination << ": seq=" << hdr.seqNo;
			stat_rcvd++;

			posix_time::ptime now = posix_time::microsec_clock::local_time();
			posix_time::time_duration diff = now - sentMap[hdr.seqNo];

			cout << " time=" << diff.total_milliseconds() << "ms" << endl;

		}

		posix_time::ptime now = posix_time::microsec_clock::local_time();
		posix_time::time_duration diffStat = now - lastStatTime;
		if (diffStat.total_seconds() >= statInterval) {
			printPingStats();
			lastStatTime = now;

		}

		posix_time::time_duration intv = now - timeSent;
		if (intv.total_milliseconds() < interval) {
			boost::this_thread::sleep(boost::posix_time::milliseconds(interval - intv.total_milliseconds()));

		}

		tmnet_close(handle);

	}
}

/**
 * @brief	main() function
 */
int main(int argc, char** argv)
{
	// declare the supported command line options

	program_options::options_description cmdopt_desc("Options");
	cmdopt_desc.add_options()
		("help,h", "Show help message")
		("socket-path,p", program_options::value<string>(), "Path to boost_sock socket file")
		("interval,i", program_options::value<unsigned int>(), "Set ping interval to <arg> (ms)")
		("destination,d", program_options::value<string>(), "Name of the destination node")
		("server,s", "Act as server (reply to pings)")
		("reliable,r", "Require reliable transport")
		("unreliable,u", "Require unreliable transport")
	;

	program_options::positional_options_description cmdopt_pdesc;
	cmdopt_pdesc.add("destination", -1);

	// parse command line options

	program_options::variables_map vm;
	program_options::store(program_options::command_line_parser(argc, argv)
		.options(cmdopt_desc).positional(cmdopt_pdesc).run(), vm);
	program_options::notify(vm);

	if (vm.count("help") || (!vm.count("destination") && !vm.count("server"))) {
		cout << cmdopt_desc << endl;
		return 1;
	}

	if (vm.count("destination"))
		destination = vm["destination"].as<string>();

	if (vm.count("socket-path"))
		socketPath = vm["socket-path"].as<string>();

	if (vm.count("interval"))
		interval = vm["interval"].as<unsigned int>();

	if (vm.count("server"))
		server = true;

	if (vm.count("reliable"))
		reliable = true;

	if (vm.count("unreliable"))
		unreliable = true;

	// open connection to NENA
	int e = tmnet_init();
	if (e != TMNET_OK) {
		cerr << "Error initializing tmnet: " << e << endl;
		return 1;
	}

	e = tmnet_set_plugin_option("tmnet::nena", "ipcsocket", socketPath.c_str());
	if (e != TMNET_OK) {
		cerr << "Error setting NENA IPC socket path: " << e << endl;
		return 1;
	}

	if (server) {
		doServer();

	} else {
		doPing();

	}

	return 0;
}
