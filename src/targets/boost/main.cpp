/**
 * Node Architecture Daemon - T1
 */

#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>


#include "systemBoost.h"
#include "nena.h"
#include "debug.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>


using namespace std;


/// end main loop or not
bool fQuit = false;

/// handle sigterm signal
void sigfunc (int sig)
{
	if (sig == SIGTERM)
	{
		DBG_DEBUG ("main: caught SIGTERM, shutting down.");
		fQuit = true;
	}
	else
		throw exception ();
}


void printHelp ()
{
	cout << "Nodearchitecture Help:" << endl;
	cout << endl;
	cout << "Usage: nad [options]" << endl;
	cout << endl;
	cout << "Options:" << endl;
	cout << "\t --version\t\t print Version" << endl;
	cout << "\t --help\t\t\t print this help" << endl;
	cout << "\t --name \"node://domain/nodeName\"\t name of this node instance" << endl;
	cout << "\t --port X\t\t set port of default net adapt (default: 50779 or taken from nenaconf.xml)" << endl;
	cout << "\t --threads (auto|off|1...n)\t Number of threads used. (default: off)" << endl;
	cout << "\t --v\t\t\t set Verbosity to Fail messages (default)" << endl;
	cout << "\t --vv\t\t\t set Verbosity to Error messages" << endl;
	cout << "\t --vvv\t\t\t set Verbosity to Warning messages" << endl;
	cout << "\t --vvvv\t\t\t set Verbosity to Debug messages" << endl;
	cout << "\t --vvvvv\t\t set Verbosity to Info messages" << endl;
	cout << "\t --daemon\t\t excecute as daemon" << endl;
}

void printVersion ()
{
	cout << "Version: pre-0.1 Alpha" << endl;
}

void printParseError ()
{
	cout << "Parse Error!" << endl;

	printHelp ();
}

int main(int argc, char *argv[])
{
	string nodeName = "node://null";
	int port = 50779;
	int nthreads = 1;
	bool fDaemon = false;
	string input;
	IDebugChannel::VerbosityLevel vlevel;

	/// enable logging
	//logTo ("nodearch.log");

	/// parse arguments
	for (int i=1; i < argc; i++)
	{
		if (strcmp (argv[i], "--help") == 0)
		{
			printHelp ();
			return 0;
		}
		else if (strcmp (argv[i], "--version") == 0)
		{
			printVersion ();
			return 0;
		}
		else if (strcmp (argv[i], "--v") == 0)
			vlevel = IDebugChannel::FAIL;
		else if (strcmp (argv[i], "--vv") == 0)
			vlevel = IDebugChannel::ERROR;
		else if (strcmp (argv[i], "--vvv") == 0)
			vlevel = IDebugChannel::WARNING;
		else if (strcmp (argv[i], "--vvvv") == 0)
			vlevel = IDebugChannel::DEBUG;
		else if (strcmp (argv[i], "--vvvvv") == 0)
			vlevel = IDebugChannel::INFO;
		else if (strcmp (argv[i], "--unlink") == 0 && argc > ++i)
		{
			// nothing (feature removed, now deleted automatically)
		}
		else if (strcmp (argv[i], "--name") == 0)
		{
			nodeName = argv[++i];
		}
		else if (strcmp (argv[i], "--port") == 0 && argc > ++i)
		{
			port = atoi (argv[i]);
		}
		else if (strcmp (argv[i], "--threads") == 0 && argc > ++i)
		{
			if (strcmp (argv[i], "auto") == 0)
			{
				nthreads = boost::thread::hardware_concurrency();
			}
			else if (strcmp (argv[i], "off") == 0)
			{
				nthreads = 0;
			}
			else
			{
				nthreads = atoi (argv[i]);
			}
		}
		else if (strcmp (argv[i], "--daemon") == 0)
		{
			fDaemon = true;
			vlevel = IDebugChannel::QUIET;
		}
		else
		{
			printParseError ();
			return 0;
		}
	}

	if (fDaemon)
	{
		pid_t pid, sid;

		/* Fork off the parent process */
		pid = fork();
		if (pid < 0)
		{
			return -1;
		}
		/* If we got a good PID, then
		   we can exit the parent process. */
		if (pid > 0)
		{
			return 0;
		}

		/* Change the file mode mask */
		umask(0);

		/* Create a new SID for the child process */
		sid = setsid();
		if (sid < 0)
		{
			/* Log any failure */
			return -1;
		}

		/* Close out the standard file descriptors */
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);


	}

	//DBG_INFO("Creating System Wrapper.");
	CSystemBoost system(vlevel, nthreads);
	system.setNodeName(nodeName);
	system.setNodePort(port);
	DBG_INFO("Creating Node Architecture.");
	CNena nodeArch(&system);

	DBG_DEBUG("Initializing Node Architecture.");
	nodeArch.init();
	DBG_DEBUG("Starting Node Architecture.");
	nodeArch.run();
	
	if (fDaemon)
	{
		/// write our pid to a file, so that everyone can send us signals
		ofstream pidfile("pidfile", ios::trunc | ios::out);
		pidfile << getpid ();
		pidfile.close();

		/// handle sigterm signal, so we can end the program gratefully
		signal (SIGTERM, sigfunc);

		while (!fQuit)
		{
			sleep (1);
		}

		/// unlink the pid file, as we don't need it anymore and it could lead to errors
		unlink ("pidfile");
	}
	else
	{
		while (!fQuit)
		{
			cout << "Type s to stop: ";
			cin >> input;

			if (input == "s")
				fQuit = true;
		}
	}
	
	nodeArch.stop ();

	return 0;
}
