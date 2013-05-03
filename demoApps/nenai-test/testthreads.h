#ifndef TESTTHREADS_H
#define TESTTHREADS_H

#include <QThread>
#include <QPlainTextEdit>
#include <QSemaphore>
#include <QMutex>
#include <QString>
#include <nenai.h>

#include <exception>
#include <string>
#include <boost/dynamic_bitset.hpp>


#include <boost/random.hpp>

class TestThread : public QThread
{
    Q_OBJECT

    protected:
	boost::mt19937 rng;
	boost::uniform_int<> tobyte;
	boost::variate_generator<boost::mt19937&, boost::uniform_int<> > die;

	virtual void run ();

    INenai * nc;

	size_t testSize;	// MB
	size_t packetSize;	// KB
	size_t packetCount;

	size_t recvCount;

	QSemaphore endSem;

	bool extTest;

	boost::dynamic_bitset<> * recvPos;

	std::string className;

    public:
	TestThread ();
	~TestThread ();

	void setTestSize (size_t size) { testSize = size; }
	void setPacketSize (size_t size) { packetSize = size; }
	void setExtTest (bool test) { extTest = test; }


	class ENullPointer : public std::exception
	{
	protected:
		std::string msg;

	public:
		ENullPointer() throw () : msg(std::string()) {};
		ENullPointer(const std::string& msg) throw () : msg(msg){};
		virtual ~ENullPointer() throw () {};

		virtual const char* what() const throw () { return msg.c_str(); };
	};

	class EParameter : public std::exception
	{
	protected:
		std::string msg;

	public:
		EParameter() throw () : msg(std::string()) {};
		EParameter(const std::string& msg) throw () : msg(msg){};
		virtual ~EParameter() throw () {};

		virtual const char* what() const throw () { return msg.c_str(); };
	};

	void callback (std::string & payload, bool endOfStream);

	signals:

	void testFinished (QString str);
};

class SocketTestThread : public TestThread
{
	private:
	std::string socketPath;

    public:
    SocketTestThread ();
	~SocketTestThread () {}

	void setSocketPath (std::string path);
	virtual void run ();
};

class SharedMemoryTestThread : public TestThread
{
    public:
	SharedMemoryTestThread ();
	~SharedMemoryTestThread () {}

	virtual void run ();
};

#endif // TESTTHREADS_H
