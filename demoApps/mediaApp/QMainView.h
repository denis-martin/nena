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
#include "QWebCam.h"
#include "QViewer.h"

class QWebCam;
class QViewer;

class QMainView : public QWidget
{
    Q_OBJECT

    public:
        QMainView(QWidget* parent = 0);
        ~QMainView();

    protected:
        QViewer* viewer;
        QWebCam* webcam;
    
    private:
        void getConfiguration();
        void initializeLayout();
        
        QTimer* sendTimer;
        QLocalSocket* socket;
        QLineEdit* identifierNameEdit;
        //QLineEdit* targetNameEdit;
        QComboBox* targetComboBox;
        QRadioButton* radioLow;
        QRadioButton* radioHigh;
        QPushButton* connectButton;
        QPushButton* evaluateButton;
        QPushButton* sendButton;
        QLabel* connectionState;

        QPixmap pConnected, pDisconnected, pPlay;

        MSG_TYPE incomingType;
        char* incomingBuffer;
        uint32_t incomingSize;
        uint32_t readSize;
        uint32_t header[2];
        int captureDevice;
        int droppedFrames;
        bool isClient;

    private slots:
        void readData();
        void sendData();
        void displayError(QLocalSocket::LocalSocketError socketError);
        void connectToNodearch();
        void reEvaluateConnection();
        void toggleSocket();
        void toggleSend();
        void toggleResolution();
        void socketState(QLocalSocket::LocalSocketState state);
};

#endif
