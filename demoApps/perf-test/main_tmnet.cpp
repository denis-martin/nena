/** @file
 *
 * Simple tool for testing communication via the NENA
 *
 */

#include "../tmnet/net_c.h"

#include <iostream>
#include <iterator>

#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>

using namespace std;
using namespace boost;

const string& APP_ID = "app://tm.kit.edu/itm/nena/demoApp/perf-test";

// configuration
string socketPath = "/tmp/nena_socket_default";
unsigned int interval = 1000000; // packet interval (micro seconds)
int stat_interval = 1; // stat interval (s)
bool server = false;
bool reliable = false;
bool unreliable = false;
size_t packetSize = 1024;

// run-time variables
string destination;

// statistics
posix_time::ptime stat_timeStart;
unsigned long stat_sent = 0;
unsigned long stat_rcvd = 0;
unsigned long stat_bytes_sent = 0;
unsigned long stat_bytes_rcvd = 0;
double stat_rtt_min = 0.0;
double stat_rtt_avg = 0.0;
double stat_rtt_max = 0.0;
double stat_rtt_mdev = 0.0;
double stat_rtt_sum = 0.0;
double stat_rate_sent = 0;
double stat_rate_rcvd = 0;

/**
 * @brief	Print some statistics from the ping server's point of view
 */
void printServerStats()
{
	posix_time::ptime now = posix_time::microsec_clock::local_time();
	posix_time::time_duration diff = now - stat_timeStart;

	stat_rate_rcvd = (double) stat_bytes_rcvd / (double) diff.total_microseconds() * 1000000;

	stat_bytes_rcvd = 0;
	stat_timeStart = now;

	cout << "--- server statistics ---" << endl;
	cout << stat_rcvd << " packets received, ";
	cout << "rate " << stat_rate_rcvd << " byte/s" << endl;
}

/**
 * @brief	Print some statistics about the ping session
 */
void printClientStats()
{
	posix_time::ptime now = posix_time::microsec_clock::local_time();
	posix_time::time_duration diff = now - stat_timeStart;

	stat_rate_sent = (double) stat_bytes_sent / (double) diff.total_microseconds() * 1000000;

	stat_bytes_sent = 0;
	stat_timeStart = now;

	cout << "--- " << destination << " statistics ---" << endl;
	cout << stat_sent << " packets transmitted, ";
	cout << "rate " << stat_rate_sent << " byte/s" << endl;
}

/**
 * @brief	Server loop
 */
void doServer()
{
	cout << "PING server mode" << endl;
	stat_timeStart = posix_time::microsec_clock::local_time();
	posix_time::ptime lastStatTime = stat_timeStart;

	tmnet_pnhandle handle;
	int e = tmnet_bind(&handle, "app://localhost.node02/perf_tmnet", NULL);
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
				cout << "New client connection" << endl;
				size_t s;
				char buf[packetSize];
				for (;;) {
					s = packetSize;
					e = tmnet_read(inch, buf, &s);
					if (e == TMNET_OK) {
						stat_rcvd++;
						stat_bytes_rcvd += s;

						posix_time::ptime now = posix_time::microsec_clock::local_time();
						posix_time::time_duration diffStat = now - lastStatTime;
						if (diffStat.total_seconds() >= stat_interval) {
							printServerStats();
							lastStatTime = now;

						}

					} else if (e == TMNET_NETWORKERROR) {
						cerr << "Error (read): " << e << " (network error)" << endl;
						break; // inner for

					} else {
						cerr << "Error (read): " << e << endl;
						break; // inner for

					}

				}

				tmnet_close(inch);

			} else {
				cerr << "Error (get): " << e << endl;

			}


		} else {
			cerr << "Error (wait): " << e << endl;
			break; // outer for

		}

	}

	e = tmnet_close(handle);
	if (e != TMNET_OK)
		cerr << "Error (close): " << e << endl;
}

/**
 * @brief	Ping loop
 */
void doClient()
{
	cout << "PERF " << destination << " " << packetSize << " bytes of data." << endl;
	stat_timeStart = posix_time::microsec_clock::local_time();
	posix_time::ptime lastStatTime = stat_timeStart;

	string req;
	if (reliable)
		req = "{ \"reliable\": 1 }";
	else if (unreliable)
		req = "{ \"reliable\": 0 }";

	tmnet_pnhandle handle;
	int e = tmnet_connect(&handle, destination.c_str(), (const tmnet_req_t) req.c_str());
	if (e != TMNET_OK) {
		cerr << "Error (connect): " << e << endl;
		return;

	}

	char payload[packetSize];

	for (;;) {
		posix_time::ptime timeSent = posix_time::microsec_clock::local_time();

		e = tmnet_write(handle, payload, packetSize);
		if (e != TMNET_OK) {
			cerr << "Error sending packet (write): " << e << endl;
			break;
		}

		stat_sent++;
		stat_bytes_sent += packetSize;

		posix_time::ptime now = posix_time::microsec_clock::local_time();
		posix_time::time_duration diffStat = now - lastStatTime;
		if (diffStat.total_seconds() >= stat_interval) {
			printClientStats();
			lastStatTime = now;

		}

		posix_time::time_duration intv = now - timeSent;
		if (intv.total_microseconds() < interval) {
			boost::this_thread::sleep(boost::posix_time::microseconds(interval - intv.total_microseconds()));

		}

	}

	tmnet_close(handle);
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
		("interval,i", program_options::value<unsigned int>(), "Set packet interval to <arg> (micro seconds)")
		("destination,d", program_options::value<string>(), "Name of the destination node")
		("server,s", "Act as server")
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
		doClient();

	}

	return 0;
}
