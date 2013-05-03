/******************************************************************************
 *   Copyright (C) 2009 Institut fuer Telematik, Universitaet Karlsruhe (TH)  *
 *                                                                            *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published by     *
 *   the Free Software Foundation; either version 2 of the License, or        *
 *   (at your option) any later version.                                      *
 *                                                                            *
 *   This program is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU General Public License for more details.                             *
 *                                                                            *
 *   You should have received a copy of the GNU General Public License        *
 *   along with this program; if not, write to the                            *
 *   Free Software Foundation, Inc.,                                          *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.                *
 ******************************************************************************/

/**
 * @file QMainThread.cpp
 * @author Helge Backhaus
 */

#include "QMainThread.h"

#include <map>

using namespace std;

QMainThread::QMainThread(QObject* parent) : QObject(parent)
{    
    incomingBuffer = NULL;
    incomingSize = 0;
    readSize = 0;
    header[0] = 0;
    header[1] = 0;
    sendTimer = NULL;
    socket = NULL;
    droppedFrames = 0;

    // process options
    for(int i = 0; i < qApp->argc(); ++i) {
        if(QString(qApp->argv()[i]).toLower() == "--file" || QString(qApp->argv()[i]).toLower() == "-f") {
            if(qApp->argc() > i + 1) {
                configFile = QString(qApp->argv()[i + 1]).remove("\"");
                configFile = QDir::cleanPath((QDir::current().absolutePath() + "/" + configFile));
            }
        }
    }

    getConfiguration();
    
    // initialize local socket...
    socket = new QLocalSocket(this);
    connect(socket, SIGNAL(connected()), this, SLOT(connectToNodearch()));
    connect(socket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(displayError(QLocalSocket::LocalSocketError)));
    connect(socket, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)), this, SLOT(socketState(QLocalSocket::LocalSocketState)));
    connect(socket, SIGNAL(readyRead()), this, SLOT(readData()));
    // ...and send timer
    sendTimer = new QTimer(this);
    connect(sendTimer, SIGNAL(timeout()), this, SLOT(sendData()));

    // connect local socket
    qDebug() << "Connect to Nodearch:" << socketPath;
    socket->connectToServer(socketPath);
    
    uint32_t r;
    uint32_t rbufsize = socket->readBufferSize();
    socklen_t olen = sizeof(rbufsize);
    r = setsockopt(socket->socketDescriptor(), SOL_SOCKET, SO_RCVBUF, &rbufsize, sizeof(rbufsize));
    qDebug() << "Set recv buffer size to" << rbufsize << "(" << r << errno << ")";
    rbufsize = 0;
    r = getsockopt(socket->socketDescriptor(), SOL_SOCKET, SO_RCVBUF, &rbufsize, &olen);
    qDebug() << "Recv buffer size" << rbufsize << "(" << r << errno << olen << ")";
}

void QMainThread::getConfiguration()
{
    socketPath = QDir::cleanPath((QDir::current().absolutePath() + "/" + socketPath));

    QFile qfConf;
    bool opened = false;
    
    if(!configFile.isEmpty()) {
        qfConf.setFileName(configFile);
        if(!qfConf.exists()) {
            qDebug() << "WARNING:" << configFile << "not found!";
        } else if(qfConf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Processing" << configFile;
            opened = true;
        }
    }
    
    if(!opened) {
        qfConf.setFileName(defaultFile);
        if(!qfConf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "WARNING:" << defaultFile << "not found!";
            return;
        } else {
            qDebug() << "Processing" << defaultFile;
        }
    }

    QTextStream streamIn(&qfConf);
    while (!streamIn.atEnd()) {
        QString line = streamIn.readLine().simplified();
        if(!line.startsWith("#") && !line.isEmpty()) {
            QStringList lineList = line.split("=");
            if(lineList.size() == 2) {
                QString key = lineList[0].simplified();
                QString value = lineList[1].simplified();

                if(key == "FPS") {
                    if(value.toUInt() > 0) {
                        FPS = value.toUInt();
                        qDebug() << "FPS:" << FPS;
                    }
                } else if(key == "dropFrameLimit") {
                    if(value.toUInt() > 0) {
                        dropFrameLimit = value.toUInt();
                        qDebug() << "dropFrameLimit:" << dropFrameLimit;
                    }
                } else if(key == "aviPath") {
                    aviPath = value.remove("\"");
                    aviPath = QDir::cleanPath((QDir::current().absolutePath() + "/" + aviPath));
                    qDebug() << "aviPath:" << aviPath;
                } else if(key == "identifier") {
                    defaultIdentifier = value.remove("\"");
                    qDebug() << "identifier:" << defaultIdentifier;
                } else if(key == "target") {
                    defaultTarget = value.remove("\"");
                    qDebug() << "target:" << defaultTarget;
                } else if(key == "socket") {
                    socketPath = value.remove("\"");
                    socketPath = QDir::cleanPath((QDir::current().absolutePath() + "/" + socketPath));
                    qDebug() << "socket:" << socketPath;
                } else if(key.startsWith("video://")) {
                	movieFiles[key] = value.remove("\"");

                }
            }
        }
    }
}

QMainThread::~QMainThread()
{
    if(sendTimer) {
        sendTimer->stop();
    }
    if(socket) {
        socket->abort();
    }
    delete socket;
    delete sendTimer;
    if(streamer) {
        delete streamer;
    }
    qDebug() << "Goodbye!";
}

void QMainThread::connectToNodearch()
{
	if (socket->state() == QLocalSocket::ConnectedState) {
		header[0] = MSG_TYPE_ID;
		header[1] = defaultIdentifier.toAscii().size();
		qDebug() << "Send identifier:" << defaultIdentifier;
		socket->write((char*)header, 2 * sizeof(uint32_t));
		socket->write(defaultIdentifier.toAscii().data(), defaultIdentifier.toAscii().size());

		header[0] = MSG_TYPE_TARGET;
		header[1] = defaultTarget.toAscii().size();
		qDebug() << "Publishing" << defaultTarget;
		socket->write((char*)header, 2 * sizeof(uint32_t));
		socket->write(defaultTarget.toAscii().data(), defaultTarget.toAscii().size());
		socket->flush();

		header[0] = MSG_TYPE_PUBLISH;
		header[1] = 0;
		socket->write((char*)header, 2 * sizeof(uint32_t));
		socket->flush();

		// initialize aviStreamer
		streamer = new QStreamer();
//		if (streamer->playFile(aviPath)) {
//			// start send timer
//			sendTimer->start(1000 / FPS);
//		}
	}
}

void QMainThread::sendData()
{
    if(socket->state() == QLocalSocket::ConnectedState) {
        if(socket->bytesToWrite() < dropFrameLimit * (streamer->pCodecCtx->width * streamer->pCodecCtx->height * BPP + 4 * sizeof(uint32_t))) {
            streamer->getNextFrame();
            uint32_t width = (uint32_t)streamer->pCodecCtx->width;
            uint32_t height = (uint32_t)streamer->pCodecCtx->height;
            width -= width % 8;
            height -= height % 8;
            uint32_t size = width * height * BPP;

            header[0] = MSG_TYPE_DATA;
            header[1] = size + 2 * sizeof(uint32_t);

            socket->write((char*)header, 2 * sizeof(uint32_t));
            socket->write((char*)&width, sizeof(uint32_t));
            socket->write((char*)&height, sizeof(uint32_t));
            for(uint32_t i = 0; i < height; ++i) {
                socket->write((char*)streamer->pFrameRGB->data[0] + i * streamer->pFrameRGB->linesize[0], width * 3);
            }
            socket->flush();
        } else {
            qDebug() << "Dropped frames:" << (++droppedFrames);
        }
    } else {
        sendTimer->stop();
    }
}

void QMainThread::readData()
{
    if (socket->state() == QLocalSocket::ConnectedState) {
        if (socket->bytesAvailable() >= (qint64) (2*sizeof(uint32_t))) {
        	socket->read((char*) header, 2*sizeof(uint32_t));
        	switch(header[0]) {
        	case MSG_TYPE_USER_1: {
        		QString videoId = QString(socket->read(header[1]));
        		qDebug() << "Received video ID:" << videoId << header[1];
        		map<QString, QString>::iterator it = movieFiles.find(videoId);
        		sendTimer->stop();
        		if (it != movieFiles.end()) {
					if (streamer->playFile(it->second)) {
						// start send timer
						sendTimer->start(1000 / FPS);
					}
        		} else {
        			qDebug() << "Video ID unknown";
        		}
        		break;
        	}
        	default: {
        		qDebug() << "Unhandled message from NENA: " << header[0];
        	}
        	}

        }

    }
}

void QMainThread::displayError(QLocalSocket::LocalSocketError socketError)
{
    switch(socketError) {
        case QLocalSocket::ServerNotFoundError:
            qDebug() << "Nodearch not found.";
        break;
        case QLocalSocket::ConnectionRefusedError:
            qDebug() << "Nodearch connection was refused.";
        break;
        case QLocalSocket::PeerClosedError:
        break;
        default:
            qDebug() << QString("The following error occurred: %1").arg(socket->errorString());
    }
    if(socket) {
        socket->abort();
    }
    sendTimer->stop();
}

void QMainThread::socketState(QLocalSocket::LocalSocketState state)
{
    switch(state) {
        case QLocalSocket::UnconnectedState:
            qDebug() << "Socket state: Unconnected";
            break;
        case QLocalSocket::ConnectingState:
            qDebug() << "Socket state: Connecting";
            break;
        case QLocalSocket::ConnectedState:
            qDebug() << "Socket state: Connected";
            break;
        case QLocalSocket::ClosingState:
            qDebug() << "Socket state: Closing";
            break;
    }
}

