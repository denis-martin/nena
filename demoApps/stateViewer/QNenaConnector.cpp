/*
 * QNenaConnector.cpp
 *
 *  Created on: Jul 16, 2012
 *      Author: denis
 */

#include "QNenaConnector.h"

#include "tmnet/net.h"
#include "tmnet/net_plugin.h"

#include <QDebug>

#define BUF_SIZE 2048

char readBuffer[BUF_SIZE];

using namespace std;

QNenaConnector::QNenaConnector(const std::string& nenaSocketPath) :
		nenaSocketPath(nenaSocketPath)
{
}

QNenaConnector::~QNenaConnector()
{

}

void QNenaConnector::run()
{
	tmnet::error e = tmnet::init();
	if (e != tmnet::eOk) {
		qDebug() << "Error initializing tmnet: " << e;
		exit(1);
	}

	if (!nenaSocketPath.empty())
		tmnet::plugin::set_option("tmnet::nena", "ipcsocket", nenaSocketPath);

	qDebug() << "QNenaConnector is running";

	exec();
}

void QNenaConnector::refreshNenaStatus()
{
	QString msg;
	tmnet::pnhandle h;
	tmnet::error e;
	size_t size;


	// architectures

	e = tmnet::get(h, "nena://localhost/stateViewer/architectures");
	if (e != tmnet::eOk) {
		qDebug() << "Error tmnet::get():" << e;
		return;
	}

	size = BUF_SIZE;
	e = tmnet::read(h, readBuffer, size);
	if (e != tmnet::eOk) {
		qDebug() << "Error tmnet::read():" << e;
		tmnet::close(h);
		return;
	}

	tmnet::close(h);
	msg = QString(string(readBuffer, size).c_str());
	emit dataReady(msg);


	// netlets

	e = tmnet::get(h, "nena://localhost/stateViewer/netlets");
	if (e != tmnet::eOk) {
		qDebug() << "Error tmnet::get():" << e;
		return;
	}

	size = BUF_SIZE;
	e = tmnet::read(h, readBuffer, size);
	if (e != tmnet::eOk) {
		qDebug() << "Error tmnet::read():" << e;
		tmnet::close(h);
		return;
	}

	tmnet::close(h);
	msg = QString(string(readBuffer, size).c_str());
	emit dataReady(msg);


	// netAdapts

	e = tmnet::get(h, "nena://localhost/stateViewer/netAdapts");
	if (e != tmnet::eOk) {
		qDebug() << "Error tmnet::get():" << e;
		return;
	}

	size = BUF_SIZE;
	e = tmnet::read(h, readBuffer, size);
	if (e != tmnet::eOk) {
		qDebug() << "Error tmnet::read():" << e;
		tmnet::close(h);
		return;
	}

	tmnet::close(h);
	msg = QString(string(readBuffer, size).c_str());
	emit dataReady(msg);
}

void QNenaConnector::refreshAppStatus()
{
	QString msg;
	tmnet::pnhandle h;
	tmnet::error e;
	size_t size;

	// appFlowStates

	e = tmnet::get(h, "nena://localhost/stateViewer/appFlowStates");
	if (e != tmnet::eOk) {
		qDebug() << "Error tmnet::get():" << e;
		return;
	}

	size = BUF_SIZE;
	e = tmnet::read(h, readBuffer, size);
	if (e != tmnet::eOk) {
		qDebug() << "Error tmnet::read():" << e;
		tmnet::close(h);
		return;
	}

	tmnet::close(h);
	msg = QString(string(readBuffer, size).c_str());
	emit dataReady(msg);
}

