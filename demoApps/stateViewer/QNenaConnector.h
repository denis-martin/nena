/*
 * QNenaConnector.h
 *
 *  Created on: Jul 16, 2012
 *      Author: denis
 */

#ifndef QNENACONNECTOR_H_
#define QNENACONNECTOR_H_

#include <QObject>
#include <QThread>

class QNenaConnector : public QThread
{
	Q_OBJECT

private:
	std::string nenaSocketPath;

signals:
	void dataReady(QString msg);

private:
	void run();

public slots:
	void refreshNenaStatus();
	void refreshAppStatus();

public:
	QNenaConnector(const std::string& nenaSocketPath);
	virtual ~QNenaConnector();
};

#endif /* QNENACONNECTOR_H_ */
