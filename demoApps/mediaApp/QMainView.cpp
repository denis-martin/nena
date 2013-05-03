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
 * @file QMainView.cpp
 * @author Helge Backhaus
 */

#include "QMainView.h"

#include <arpa/inet.h>

#include "../../src/targets/boost/msg.h"

#include <QMessageBox>

namespace nena {

typedef enum {
	userreq_ok = 1, userreq_cancel = 2, userreq_yes = 4, userreq_no = 8,
	userreq_okcancel = userreq_ok | userreq_cancel,
	userreq_yesno = userreq_yes | userreq_no
} userreq_t;

}

QMainView::QMainView(QWidget* parent) : QWidget(parent)
{
	incomingType = MSG_TYPE_DATA;
    incomingBuffer = NULL;
    incomingSize = 0;
    readSize = 0;
    header[0] = 0;
    header[1] = 0;
    sendTimer = NULL;
    socket = NULL;
    droppedFrames = 0;

    radioLow = NULL;
    radioHigh = NULL;
    sendButton = NULL;

    isClient = false;
    captureDevice = -1;

    // process options
    for(int i = 0; i < qApp->argc(); ++i) {
        if(QString(qApp->argv()[i]).toLower() == "--client" || QString(qApp->argv()[i]).toLower() == "-c") {
            isClient = true;
        }
        if(QString(qApp->argv()[i]).toLower() == "--device" || QString(qApp->argv()[i]).toLower() == "-d") {
            if(qApp->argc() > i + 1) {
                captureDevice = QString(qApp->argv()[i + 1]).toUInt();
            }
        }
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
    socket->setReadBufferSize((stdResolutionX * stdResolutionY * BPP + 4 * sizeof(uint32_t)) * dropFrameLimit);
    connect(socket, SIGNAL(connected()), this, SLOT(connectToNodearch()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(socket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(displayError(QLocalSocket::LocalSocketError)));
    connect(socket, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)), this, SLOT(socketState(QLocalSocket::LocalSocketState)));
	// ...and send timer
    if(!isClient) {
        sendTimer = new QTimer(this);
        connect(sendTimer, SIGNAL(timeout()), this, SLOT(sendData()));
    }

    initializeLayout();

    setWindowTitle(tr("NENA Media Player"));
}

void QMainView::getConfiguration()
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

    defaultTargets << "node://";
    QTextStream streamIn(&qfConf);
    while (!streamIn.atEnd()) {
        QString line = streamIn.readLine().simplified();
        if(!line.startsWith("#") && !line.isEmpty()) {
            QStringList lineList = line.split("=");
            if(lineList.size() == 2) {
                QString key = lineList[0].simplified();
                QString value = lineList[1].simplified();

                if(key == "stdResolutionX") {
                    if(value.toUInt() > 0) {
                        stdResolutionX = value.toUInt();
                        qDebug() << "stdResolutionX:" << stdResolutionX;
                    }
                } else if(key == "stdResolutionY") {
                    if(value.toUInt() > 0) {
                        stdResolutionY = value.toUInt();
                        qDebug() << "stdResolutionY:" << stdResolutionY;
                    }
                } else if(key == "altResolutionX") {
                    if(value.toUInt() > 0) {
                        altResolutionX = value.toUInt();
                        qDebug() << "altResolutionX:" << altResolutionX;
                    }
                } else if(key == "altResolutionY") {
                    if(value.toUInt() > 0) {
                        altResolutionY = value.toUInt();
                        qDebug() << "altResolutionY:" << altResolutionY;
                    }
                } else if(key == "camFPS") {
                    if(value.toUInt() > 0) {
                        camFPS = value.toUInt();
                        qDebug() << "camFPS:" << camFPS;
                    }
                } else if(key == "dropFrameLimit") {
                    if(value.toUInt() > 0) {
                        dropFrameLimit = value.toUInt();
                        qDebug() << "dropFrameLimit:" << dropFrameLimit;
                    }
                } else if(key == "identifier") {
                    defaultIdentifier = value.remove("\"");
                    qDebug() << "identifier:" << defaultIdentifier;
                } else if(key == "target") {
                	defaultTargets << value.remove("\"");
                    qDebug() << "target:" << defaultTargets.last();
                } else if(key == "socket") {
                    socketPath = value.remove("\"");
                    socketPath = QDir::cleanPath((QDir::current().absolutePath() + "/" + socketPath));
                    qDebug() << "socket:" << socketPath;
                }
            }
        }
    }
}

void QMainView::initializeLayout()
{
    // load graphics
    pConnected.load("./images/connected.png");
    pDisconnected.load("./images/disconnected.png");
    pPlay.load("./images/play.png");
        
    // setup controls
    QLabel* identifierNameLabel = new QLabel(tr("Identifier:"));
    identifierNameEdit = new QLineEdit(defaultIdentifier);

    QLabel* targetNameLabel = new QLabel(tr("Target:"));
    //targetNameEdit = new QLineEdit(defaultTarget);
    
    targetComboBox = new QComboBox();
    for(int i = 0; i < defaultTargets.size(); ++i) {
    	targetComboBox->addItem(defaultTargets.at(i));
    }
    targetComboBox->setEditable(true);

    connectButton = new QPushButton(tr("Get"));
    connect(connectButton, SIGNAL(clicked()), this, SLOT(toggleSocket()));

    evaluateButton = new QPushButton(tr("(Re)Evaluate"));
    connect(evaluateButton, SIGNAL(clicked()), this, SLOT(reEvaluateConnection()));

    QLabel* resolutionNameLabel = NULL;
    if(!isClient) {
        sendButton = new QPushButton(tr("Start Cam"));
        connect(sendButton, SIGNAL(clicked()), this, SLOT(toggleSend()));

        resolutionNameLabel = new QLabel(tr("Resolution:"));

        radioLow = new QRadioButton(QString("%1x%2").arg(altResolutionX).arg(altResolutionY));
        connect(radioLow, SIGNAL(clicked()), this, SLOT(toggleResolution()));

        radioHigh = new QRadioButton(QString("%1x%2").arg(stdResolutionX).arg(stdResolutionY));
        radioHigh->setChecked(true);
        connect(radioHigh, SIGNAL(clicked()), this, SLOT(toggleResolution()));
    }

    connectionState = new QLabel();
    connectionState->setPixmap(pDisconnected);

    // setup window layout
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    //controlsLayout->addWidget(identifierNameLabel);
    //controlsLayout->addWidget(identifierNameEdit);
    controlsLayout->addWidget(targetNameLabel);
    controlsLayout->addWidget(targetComboBox);
    controlsLayout->addStretch(1);
    controlsLayout->addWidget(connectButton);
    //controlsLayout->addWidget(evaluateButton);
    if(!isClient) {
        controlsLayout->addWidget(sendButton);
        controlsLayout->addWidget(resolutionNameLabel);
        controlsLayout->addWidget(radioLow);
        controlsLayout->addWidget(radioHigh);
    }
    //controlsLayout->addWidget(connectionState);
    //controlsLayout->addStretch(1);
    
    QHBoxLayout* mediaLayout = new QHBoxLayout();
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addLayout(controlsLayout);
    mainLayout->addStretch(1);
    mediaLayout->addStretch(1);
    if(!isClient) {
        webcam = new QWebCam(captureDevice);
        mediaLayout->addWidget(webcam);
    } else {
        viewer = new QViewer();
        mediaLayout->addWidget(viewer);
    }
    mediaLayout->addStretch(1);
    mainLayout->addLayout(mediaLayout);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}

QMainView::~QMainView()
{
    if(sendTimer) {
        sendTimer->stop();
    }
    if(socket) {
        socket->abort();
    }
    delete socket;
    delete sendTimer;
}

void QMainView::toggleSocket() {
    if(socket->state() == QLocalSocket::ConnectedState) {
        if(sendTimer) {
            sendTimer->stop();
        }
        socket->abort();
        connectButton->setText(tr("Get"));
        if(!isClient) {
            sendButton->setText(tr("Start Cam"));
        }
    } else {
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
}

void QMainView::reEvaluateConnection() {
	toggleSocket();
	toggleSocket();
}

void QMainView::toggleSend()
{
   if(sendTimer->isActive()) {
        sendTimer->stop();
        sendButton->setText(tr("Start Cam"));
        socketState(socket->state());
    } else if(webcam->getReady() && socket->state() == QLocalSocket::ConnectedState) {
        sendTimer->start(1000 / camFPS);
        sendButton->setText(tr("Stop Cam"));
        connectionState->setPixmap(pPlay);
    }
}

void QMainView::toggleResolution()
{
    if(radioLow->isChecked()) {
        webcam->setResolution(QSize(altResolutionX, altResolutionY));
    } else if(radioHigh->isChecked()) {
        webcam->setResolution(QSize(stdResolutionX, stdResolutionY));
    }
    qDebug() << "Image size:" << webcam->getResolution().width() << "x" << webcam->getResolution().height();
}

void QMainView::connectToNodearch()
{
    header[0] = MSG_TYPE_ID;
    header[1] = identifierNameEdit->text().toAscii().size();
    qDebug() << "Send identifier:" << identifierNameEdit->text();
    socket->write((char*)header, 2 * sizeof(uint32_t));
    socket->write(identifierNameEdit->text().toAscii().data(), identifierNameEdit->text().toAscii().size());

    header[0] = MSG_TYPE_TARGET;
    header[1] = targetComboBox->currentText().toAscii().size();
    qDebug() << "Connect to:" << targetComboBox->currentText();
    socket->write((char*)header, 2 * sizeof(uint32_t));
    socket->write(targetComboBox->currentText().toAscii().data(), targetComboBox->currentText().toAscii().size());
    socket->flush();
    connectButton->setText(tr("Stop"));
}

void QMainView::readData()
{
    if(incomingSize == 0) {
        if(socket->bytesAvailable() < 2 * sizeof(uint32_t)) {
            return;
        }
        uint32_t type;
        socket->read((char*)&type, sizeof(uint32_t));
        socket->read((char*)&incomingSize, sizeof(uint32_t));
        //qDebug() << "Header from daemon received, type" << type << ", size" << incomingSize;

        incomingType = static_cast<MSG_TYPE>(type);
        if (incomingType != MSG_TYPE_DATA && incomingType != MSG_TYPE_USERREQ) {
            qDebug() << "WARNING: Invalid message type, disconnecting";
            incomingSize = 0;
            if (socket->state() == QLocalSocket::ConnectedState)
				toggleSocket();
            return;
        }
        incomingBuffer = new char[incomingSize];
    }
    while(socket->bytesAvailable()) {
        qint64 temp = socket->read(&incomingBuffer[readSize], incomingSize);
        //if(temp > 500000) {
        //    qDebug() << "Burst-read" << temp << "bytes";
        //}
        readSize += temp;
        incomingSize -= temp;
        if(incomingSize == 0) {
        	if (incomingType == MSG_TYPE_DATA) {
				//qDebug() << "Frame completed, read" << readSize << "(" << *(int*)incomingBuffer << "," << *(int*)(incomingBuffer + 4) << ")";
				uint32_t width, height;
				width = *(uint32_t*)incomingBuffer;
				height = *(uint32_t*)(incomingBuffer + sizeof(uint32_t));
				if (width > 2048 || height > 2048 || width*height*BPP != (readSize - sizeof(uint32_t)*2)) {
					qDebug() << "WARNING: Invalid message format from daemon:" << width << height << BPP << readSize << "frame dropped, disconnecting";
					if (socket->state() == QLocalSocket::ConnectedState)
						toggleSocket();

				} else {
					viewer->setResolution(QSize(width, height));
					viewer->updateView(incomingBuffer + sizeof(uint32_t)*2);

				}

        	} else if (incomingType == MSG_TYPE_USERREQ) {
        		uint32_t reqId, reqType, msgSize;
        		QString msg;
        		reqId   = ntohl(*(uint32_t*)incomingBuffer);
        		reqType = ntohl(*(uint32_t*)(incomingBuffer + sizeof(uint32_t)));
        		msgSize = ntohl(*(uint32_t*)(incomingBuffer + sizeof(uint32_t)*2));

        		if (msgSize == readSize - 3*sizeof(uint32_t))
        			msg = QString::fromUtf8(incomingBuffer + sizeof(uint32_t)*3, msgSize);

        		qDebug() << "User request: (reqId, reqType, msgSize, msg) ="
        		        				<< reqId << reqType << msgSize << msg;

        		if (!msg.isEmpty()) {
        			QMessageBox::StandardButtons btns;
        			if (reqType & (uint32_t) nena::userreq_ok)     btns |= QMessageBox::Ok;
        			if (reqType & (uint32_t) nena::userreq_cancel) btns |= QMessageBox::Cancel;
        			if (reqType & (uint32_t) nena::userreq_yes)    btns |= QMessageBox::Yes;
        			if (reqType & (uint32_t) nena::userreq_no)     btns |= QMessageBox::No;

        			int btn = QMessageBox::question(this, "NENA", msg, btns);

        			if (btn == QMessageBox::Ok)          reqType = (uint32_t) nena::userreq_ok;
        			else if (btn == QMessageBox::Cancel) reqType = (uint32_t) nena::userreq_cancel;
        			else if (btn == QMessageBox::Yes)    reqType = (uint32_t) nena::userreq_yes;
        			else if (btn == QMessageBox::No)     reqType = (uint32_t) nena::userreq_no;

        			uint32_t reply[4];
        			reply[0] = MSG_TYPE_USERREQ;
        			reply[1] = 2*sizeof(uint32_t);
        			reply[2] = htonl(reqId);
        			reply[3] = htonl((uint32_t) reqType);
        			socket->write((const char*) &reply[0], (qint64) sizeof(reply));
        			socket->flush();

        		}



        	}

            delete[] incomingBuffer;
            incomingBuffer = NULL;
            readSize = 0;
            break;
        }
    }
}

void QMainView::sendData()
{
    if(socket->state() == QLocalSocket::ConnectedState) {
        if (socket->bytesToWrite() < dropFrameLimit * (webcam->getResolution().width() * webcam->getResolution().height() * BPP + 4 * sizeof(uint32_t))) {
            uint32_t x = (uint32_t)webcam->getResolution().width();
            uint32_t y = (uint32_t)webcam->getResolution().height();
            uint32_t size = x * y * BPP;

            header[0] = MSG_TYPE_DATA;
            header[1] = size + 2 * sizeof(uint32_t);

            socket->write((char*)header, 2 * sizeof(uint32_t));
            socket->write((char*)&x, sizeof(uint32_t));
            socket->write((char*)&y, sizeof(uint32_t));
            socket->write((char*)webcam->getData(), size);
            socket->flush();
        } else {
            qDebug() << "Dropped frames:" << (++droppedFrames);
        }
    } else {
        sendTimer->stop();
        sendButton->setText(tr("Start Cam"));
    }
}

void QMainView::displayError(QLocalSocket::LocalSocketError socketError)
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
        connectButton->setText(tr("Get"));
    }
    if(!isClient) {
        sendTimer->stop();
        sendButton->setText(tr("Start Cam"));
    }
}

void QMainView::socketState(QLocalSocket::LocalSocketState state)
{
    switch(state) {
        case QLocalSocket::UnconnectedState:
            qDebug() << "Socket state: Unconnected";
            connectionState->setPixmap(pDisconnected);
        break;
        case QLocalSocket::ConnectingState:
            qDebug() << "Socket state: Connecting";
            connectionState->setPixmap(pDisconnected);
        break;
        case QLocalSocket::ConnectedState:
            qDebug() << "Socket state: Connected";
            connectionState->setPixmap(pConnected);
        break;
        case QLocalSocket::ClosingState:
            qDebug() << "Socket state: Closing";
            connectionState->setPixmap(pDisconnected);
        break;
    }
}
