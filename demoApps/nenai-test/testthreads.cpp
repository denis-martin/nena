#include "testthreads.h"
#include <QDebug>

#include <memNenai.h>
#include <socketNenai.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/random.hpp>
#include <boost/date_time.hpp>

#include <string>
#include <sstream>

using std::string;
using boost::bind;
using boost::function;

using boost::posix_time::microsec_clock;

/* ------------------------------------------------------------------------- */

TestThread::TestThread ():
		tobyte(0, 255),
		die (rng, tobyte),
		nc(NULL),
		testSize(0),
		packetSize(0),
		packetCount(0),
		recvCount(0),
		extTest(false),
		className("Test")
{}

TestThread::~TestThread ()
{
	if (nc != NULL)
		delete nc;
}

void TestThread::run ()
{
	boost::posix_time::ptime startTime, endTime;
	bool result;

	qDebug () << "Entered TestThread run.";

	if (nc == NULL)
		throw ENullPointer();

	if (testSize == 0 || packetSize == 0)
		throw EParameter();


	qDebug () << "Checks are fine.";

	if (extTest)
	{
		nc->setExtTestMode(true);
		qDebug () << "Setting to extended test mode.";
	}
	else
	{
		nc->setTestMode(true);
		qDebug () << "Setting to test mode.";
	}

	packetCount = testSize * 1024 * 1024 / packetSize;

//	size_t y_size = packetCount/16 + ( packetCount % 16 == 0 ? 0 : 1);

	recvPos = new boost::dynamic_bitset<>(packetCount);

	qDebug () << "Will now send and receive " << packetCount << " packets of size " << packetSize << "bytes.";

	startTime = microsec_clock::universal_time();

	string data (packetSize, '\0');

	for (size_t i=0; i < packetCount; i++)
	{
		for (size_t n=0; n < sizeof(size_t); n++)
			data[n] = reinterpret_cast<char *> (&i)[n];

		for (size_t j=sizeof(size_t); j < packetSize; j++)
			data[j] = static_cast<unsigned char> (die());

		assert(data.size () == packetSize);

		nc->sendData(data);
	}


	qDebug () << "Sending finished, waiting for all Data to be received.";
	result = endSem.tryAcquire(1, 60000);

	endTime = microsec_clock::universal_time();

	qDebug () << "Stopping MemNenai.";
	nc->stop();
	delete nc;
	nc = NULL;

	std::ostringstream output;
	QString report;

	output << className << " test results:\r\n";

	if (result)
	{
		qDebug () << packetCount << " packets of size " << packetSize << "bytes in " << (endTime-startTime).total_milliseconds() << "ms - " << (float)testSize*1024/(endTime-startTime).total_milliseconds()<< "MB/s";
		output << "\tPackets sent:\t\t" << packetCount << "\r\n";
		output << "\tPackets received:\t\t" << recvCount << "\r\n";
		output << "\tPacket size:\t\t\t" << packetSize << "\r\n";
		output << "\tTime:\t\t\t\t" << (endTime-startTime).total_milliseconds() << "ms\r\n";
		output << "\tThroughput:\t\t\t" << (float)testSize*1024/(endTime-startTime).total_milliseconds()<< "MB/s\r\n";
	}
	else
	{
		qDebug () << "Test could not be finished. Only " << recvCount << " of " << packetCount << " packets received!";
		output << "\tTest could not be finished, only " << recvCount << " of " << packetCount << " packets received!\r\n";
		output << "\tReceive Matrix:";

		for (size_t i=0; i < packetCount; i++)
		{
			if (i % 32 == 0)
				output << "\r\n\t";

			output << recvPos->test(i);
		}
	}

	report = QString::fromStdString(output.str());
	emit testFinished(report);
	recvCount = 0;
	delete recvPos;
	recvPos = NULL;
}

void TestThread::callback (string & payload, bool endOfStream)
{	

	size_t num;
	for (size_t i=0; i < sizeof(size_t); i++)
		reinterpret_cast<char *> (&num)[i] = payload[i];

	if (packetSize!=payload.size())
		qDebug () << "Error on packet number " << num << ", count " << recvCount << " with size " << payload.length();

	assert(packetSize==payload.size());

	recvPos->set(num);

	recvCount++;

	if (recvCount==packetCount-1)
	{
		endSem.release();
	}
}

/* ------------------------------------------------------------------------- */

SocketTestThread::SocketTestThread ():
	TestThread ()
{
	socketPath ="boost_sock";
	className += "::Socket";
}

void SocketTestThread::setSocketPath (string path)
{
	socketPath = path;
}

void SocketTestThread::run ()
{

	function<void(string payload, bool endOfStream)> f = bind(&TestThread::callback, this, _1, _2);
	nc = new SocketNenai ("nenai-test", "node2", socketPath, f);

	nc->run();

	qDebug () << "Started SocketNenai.";

	TestThread::run ();
}

/* ------------------------------------------------------------------------- */

SharedMemoryTestThread::SharedMemoryTestThread():
		TestThread()
{
	className += "::Memory";
}

void SharedMemoryTestThread::run ()
{
	qDebug () << "Entered SharedMemoryTestThread run.";

	function<void(string payload, bool endOfStream)> f = bind(&TestThread::callback, this, _1, _2);
	nc = new MemNenai ("nenai-test", "node2", f);

	nc->run();

	qDebug () << "Started MemNenai.";

	TestThread::run ();
}
