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
#include <QColor>
#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <QFont>

#include "QViewer.h"
#include "QNenaSocket.h"

#ifndef INT64_C
  #define INT64_C(c) (c ## LL)
  #define UINT64_C(c) (c ## ULL)
#endif

extern "C" {
  #include <avcodec.h>
  #include <avformat.h>
  #include <swscale.h>
}

class QViewer;

class QMainView : public QWidget
{
  Q_OBJECT

  public:
    QMainView(QWidget* parent = 0);
    ~QMainView();
    void closeEvent(QCloseEvent* event);

  private:
    QString nenaSocketPath;
    QString videoName;
    QString fileName;
    QString ctrlName;
    QNenaDataSocket* videoSocket;
    QNenaCtrlSocket* ctrlSocket;
    
    AVCodecContext* pCodecCtx;
    SwsContext* img_convert_ctx;
    AVFrame* pFrame;
    AVFrame* pFrameRGB;
    
    bool playing;

    void initializeLayout();
    bool setupStream(QString);
    void getConfiguration();
    
    QViewer* viewer;
    QPushButton* btnStart;
    QPushButton* btnRewind;
    QPushButton* btnPlayPause;
    QPushButton* btnForward;
    QLabel* lblState;

    QPixmap pStart, pRewind, pPlay, pPause, pForward;

  public slots:
    void readData(QByteArray);
    void clickedStart();
    void clickedRewind();
    void clickedPlayPause();
    void clickedForward();
    void exitPlayer();
};

#endif
