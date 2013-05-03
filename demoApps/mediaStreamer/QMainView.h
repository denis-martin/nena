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
 * @file QMainView.h
 * @author Helge Backhaus
 */

#ifndef QMAINVIEW_H
#define QMAINVIEW_H

#include <QtGui>
#include <QObject>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QTimer>
#include <QColor>
#include <QLocalSocket>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdint.h>

#include "definitions.h"
#include "QStreamer.h"

class QWebCam;
class QViewer;

class QMainView : public QWidget
{
    Q_OBJECT

    public:
        QMainView(QWidget* parent = 0);
        ~QMainView();

    protected:        
        QStreamer* streamer;
    
    private:
        void getConfiguration();
        void initializeLayout();
        
        QTimer* sendTimer;
        QTimer* grabTimer;
        QLocalSocket* socket;
        QLineEdit* identifierNameEdit;
        QLineEdit* targetNameEdit;
        QPushButton* connectButton;
        QPushButton* sendButton;
        QLabel* connectionState;
        QLabel* videoView;
        QImage currentFrame;

        QPixmap pConnected, pDisconnected, pPlay;

        char* incomingBuffer;
        uint32_t incomingSize;
        uint32_t readSize;
        uint32_t header[2];
        int droppedFrames;

    private slots:
        void sendData();
        void grabFrame();
        void displayError(QLocalSocket::LocalSocketError socketError);
        void connectToNodearch();
        void toggleSocket();
        void toggleSend();
        void socketState(QLocalSocket::LocalSocketState state);
};

#endif
