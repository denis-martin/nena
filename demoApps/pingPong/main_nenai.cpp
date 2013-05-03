/** @file
 *
 * Simple tool for testing communication via the Node Architecture
 *
 */

#include "../nenai/socketNenai.h"

#include <iostream>
#include <iterator>

#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>

using namespace std;
using namespace boost;

const string& APP_ID = "app://tm.kit.edu/itm/nena/demoApp/pingPong";

// configuration
string socketPath = "../../build/targets/boost/boost_sock";
unsigned int interval = 1000;
string destination = "node://dummy"; // dummy value (should be removed for future app connectors)
bool server = false;
int statInterval = 10; // seconds

// run-time variables
unsigned short seqNo = 0;
map<unsigned short, posix_time::ptime> sentMap;

// the application interface handle
INenai* nenai = NULL;

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
 * @brief	Handle packet reception
 */
void handleReceive(string& payload, bool endOfStream)
{
//	cout << "Received " << payload.size() << " bytes" << endl;

	if (server) {
		if (payload.size() > 0) {
			assert(payload.size() == sizeof(PingHeader));

			PingHeader* hdr = (PingHeader*) payload.c_str();
			cout << payload.size() << " bytes from " << destination << ": seq=" << hdr->seqNo << endl;
			stat_rcvd++;

			// send reply
			nenai->sendData(payload);

		}

	} else {
		if (payload.size() > 0) {
			assert(payload.size() == sizeof(PingHeader));

			PingHeader* hdr = (PingHeader*) payload.c_str();
			cout << payload.size() << " bytes from " << destination << ": seq=" << hdr->seqNo;
			stat_rcvd++;

			posix_time::ptime now = posix_time::microsec_clock::local_time();
			posix_time::time_duration diff = now - sentMap[hdr->seqNo];

			cout << " time=" << diff.total_milliseconds() << "ms" << endl;

		}

	}
}

/**
 * @brief	Server loop
 */
void doServer()
{
	cout << "PING server mode" << endl;
	stat_timeStart = posix_time::microsec_clock::local_time();

	for (;;) {
		boost::this_thread::sleep(boost::posix_time::seconds(statInterval));
		printServerStats();

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

	for (;;) {
		PingHeader hdr;
		hdr.seqNo = ++seqNo;

		string payload((char*) &hdr, sizeof(hdr));
		assert(payload.size() == sizeof(hdr));

		posix_time::ptime timeSent = posix_time::microsec_clock::local_time();
		sentMap[seqNo] = timeSent;

		nenai->sendData(payload);
		stat_sent++;

		boost::this_thread::sleep(boost::posix_time::milliseconds(interval));

		posix_time::ptime now = posix_time::microsec_clock::local_time();
		posix_time::time_duration diffStat = now - lastStatTime;
		if (diffStat.total_seconds() >= statInterval) {
			printPingStats();
			lastStatTime = now;

		}
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

	// open connection to NENA
	nenai = new SocketNenai(APP_ID, socketPath, handleReceive);
	nenai->run();
	nenai->setTarget(destination);

	if (server) {
		doServer();

	} else {
		doPing();

	}
}
