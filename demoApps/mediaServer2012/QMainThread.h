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
 * @file QMainThread.h
 * @author Helge Backhaus
 */

#ifndef QMAINTHREAD_H
#define QMAINTHREAD_H

#include <QApplication>
#include <QObject>
#include <QDebug>
#include <QTimer>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QByteArray>

#include "QNenaServer.h"

#ifndef INT64_C
  #define INT64_C(c) (c ## LL)
  #define UINT64_C(c) (c ## ULL)
#endif

extern "C" {
  #include <avcodec.h>
  #include <avformat.h>
}

class QMainThread : public QObject
{
  Q_OBJECT

  public:
    QMainThread(QObject* parent = 0);
    ~QMainThread();

  private:       
    QTimer* sendTimer;
    QString nenaSocketPath;
    QString videoName;
    QString ctrlName;
    QNenaDataServer* videoSocket;
    QNenaCtrlServer* ctrlSocket;
    QByteArray currentFrame;

    AVFormatContext* pFormatCtx;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;
    AVPacket packet;
    int videoStream;
    int64_t currentTime;
    bool isStreaming;
    
    void getConfiguration();
    bool streamFile(QString);
    void seekStream(int, bool absolute = false);
    int updateCurrentFrame(bool flush = false);
    
    
  public slots:
    void gotVideoRequest(QString);
    void gotCommand(char);
    void sendData();
    void resetServer();
    
  signals:
    void writeData(QByteArray);
    void resetData();
    void resetControl();
};

#endif
