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
 * @file QWebCam.h
 * @author Helge Backhaus
 */

/*
 * Based on QWebCam.Widget by
 * Stephane List, Copyright (C) 2008, 2009
 * slist@lilotux.net
 */

#ifndef QWEBCAM_H
#define QWEBCAM_H

#include <QObject>
#include <QImage>
#include <QPixmap>
#include <QLabel>
#include <QTimer>
#include <QDebug>
#include <QThread>

#include <cv.h>
#include <highgui.h>

#include "definitions.h"

class QCapture : public QThread
{
    Q_OBJECT

    public:
        void run();
        void setCvCapture(CvCapture* camCapture);

    public slots:
        void grabImage();

    signals:
        void frameChanged(uchar* bits);

    private:
        void Ipl2QImage();

        uchar* qCamImageBits;
        int frameCount;
        QTimer grabTimer;
        CvCapture* camCapture;
        IplImage* camImage;
};

class QWebCam : public QLabel
{
    Q_OBJECT

    public:
        QWebCam(int captureDevice = -1);
        ~QWebCam();
        uchar* getData();
        bool getReady();
        QSize getResolution();
        void setResolution(QSize resolution);

    public slots:
        void updateFrame(uchar* bits);

    private:
        QCapture* captureLoop;
        QSize resolution;
        QSize nativeResolution;
        CvCapture* camCapture;
        QImage qCamImage;
        bool ready;
};

#endif
