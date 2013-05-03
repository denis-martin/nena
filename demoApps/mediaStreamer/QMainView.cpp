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

QMainView::QMainView(QWidget* parent) : QWidget(parent)
{
    incomingBuffer = NULL;
    incomingSize = 0;
    readSize = 0;
    header[0] = 0;
    header[1] = 0;
    sendTimer = NULL;
    socket = NULL;
    droppedFrames = 0;

    sendButton = NULL;

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
    socket->setReadBufferSize((300000 * BPP + 4 * sizeof(uint32_t)) * dropFrameLimit);
    connect(socket, SIGNAL(connected()), this, SLOT(connectToNodearch()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(socket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(displayError(QLocalSocket::LocalSocketError)));
    connect(socket, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)), this, SLOT(socketState(QLocalSocket::LocalSocketState)));
	// ...and send timer
    sendTimer = new QTimer(this);
    grabTimer = new QTimer(this);
    connect(sendTimer, SIGNAL(timeout()), this, SLOT(sendData()));
    connect(grabTimer, SIGNAL(timeout()), this, SLOT(grabFrame()));

    initializeLayout();

    setWindowTitle(tr("Node Architecture Media Streamer"));
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
                } else if(key == "aviPath") {
                    aviPath = value.remove("\"");
                    aviPath = QDir::cleanPath((QDir::current().absolutePath() + "/" + aviPath));
                    qDebug() << "video:" << aviPath;
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
    targetNameEdit = new QLineEdit(defaultTarget);

    connectButton = new QPushButton(tr("Connect"));
    connect(connectButton, SIGNAL(clicked()), this, SLOT(toggleSocket()));

    sendButton = new QPushButton(tr("Start Streaming"));
    connect(sendButton, SIGNAL(clicked()), this, SLOT(toggleSend()));

    connectionState = new QLabel();
    connectionState->setPixmap(pDisconnected);

    videoView = new QLabel();

    // setup window layout
    QVBoxLayout* controlsLayout = new QVBoxLayout();
    controlsLayout->addWidget(identifierNameLabel);
    controlsLayout->addWidget(identifierNameEdit);
    controlsLayout->addWidget(targetNameLabel);
    controlsLayout->addWidget(targetNameEdit);
    controlsLayout->addWidget(connectButton);
    controlsLayout->addWidget(sendButton);
    controlsLayout->addWidget(connectionState);
    controlsLayout->addStretch(1);

    QHBoxLayout* mainLayout = new QHBoxLayout();
    mainLayout->addStretch(1);
    mainLayout->addWidget(videoView);
    mainLayout->addLayout(controlsLayout);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    // initialize aviStreamer
    streamer = new QStreamer();
    if(streamer->playFile(aviPath)) {
        // start grab timer
        grabTimer->start(1000 / 30);
    }
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
        connectButton->setText(tr("Connect"));
        sendButton->setText(tr("Start Streaming"));
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

void QMainView::toggleSend()
{
   if(sendTimer->isActive()) {
        sendTimer->stop();
        sendButton->setText(tr("Start Streaming"));
        socketState(socket->state());
    } else if(socket->state() == QLocalSocket::ConnectedState) {
        sendTimer->start(1000 / FPS);
        sendButton->setText(tr("Stop Streaming"));
        connectionState->setPixmap(pPlay);
    }
}

void QMainView::connectToNodearch()
{
    header[0] = MSG_TYPE_ID;
    header[1] = identifierNameEdit->text().toAscii().size();
    qDebug() << "Send identifier:" << identifierNameEdit->text();
    socket->write((char*)header, 2 * sizeof(uint32_t));
    socket->write(identifierNameEdit->text().toAscii().data(), identifierNameEdit->text().toAscii().size());

    header[0] = MSG_TYPE_TARGET;
    header[1] = targetNameEdit->text().toAscii().size();
    qDebug() << "Connect to:" << targetNameEdit->text();
    socket->write((char*)header, 2 * sizeof(uint32_t));
    socket->write(targetNameEdit->text().toAscii().data(), targetNameEdit->text().toAscii().size());
    socket->flush();
    connectButton->setText(tr("Disconnect"));
}

void QMainView::sendData()
{
    if(socket->state() == QLocalSocket::ConnectedState) {
        if (socket->bytesToWrite() < dropFrameLimit * (300000 * BPP + 4 * sizeof(uint32_t))) {
            
            uint32_t width = (uint32_t)currentFrame.width();
            uint32_t height = (uint32_t)currentFrame.height();
            uint32_t size = width * height * BPP;

            header[0] = MSG_TYPE_DATA;
            header[1] = size + 2 * sizeof(uint32_t);

            socket->write((char*)header, 2 * sizeof(uint32_t));
            socket->write((char*)&width, sizeof(uint32_t));
            socket->write((char*)&height, sizeof(uint32_t));
            socket->write((char*)currentFrame.bits(), size);
            socket->flush();
        } else {
            qDebug() << "Dropped frames:" << (++droppedFrames);
        }
    } else {
        sendTimer->stop();
        sendButton->setText(tr("Start Streaming"));
    }
}

void QMainView::grabFrame()
{
    streamer->getNextFrame();
    uint32_t width = (uint32_t)streamer->pCodecCtx->width;
    uint32_t height = (uint32_t)streamer->pCodecCtx->height;
    width -= width % 8;
    height -= height % 8;
    currentFrame = QImage(width, height, QImage::Format_RGB888);
    for(uint32_t i = 0; i < height; ++i) {
        memcpy(currentFrame.scanLine(i), (uchar*)streamer->pFrameRGB->data[0] + i * streamer->pFrameRGB->linesize[0], width * 3);
    }
    videoView->setPixmap(QPixmap::fromImage(currentFrame, Qt::AutoColor));
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
        connectButton->setText(tr("Connect"));
    }
    sendTimer->stop();
    sendButton->setText(tr("Start Streaming"));
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
