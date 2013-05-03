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
 * @file QWebCam.cpp
 * @author Helge Backhaus
 */

/*
 * Based on QWebCam.Widget by
 * Stephane List, Copyright (C) 2008, 2009
 * slist@lilotux.net
 */

#include "QWebCam.h"

void QCapture::run()
{
    frameCount = 0;
    qCamImageBits = NULL;

    grabTimer.setParent(this);
    connect(&grabTimer, SIGNAL(timeout()), this, SLOT(grabImage()));
    grabTimer.start(1000 / camFPS);

    exec();
    qDebug() << "Captured frames:" << frameCount;
}

void QCapture::grabImage()
{
    if(camCapture) {
        cvGrabFrame(camCapture);
        camImage = cvRetrieveFrame(camCapture);
        if(camImage) {
            ++frameCount;
            Ipl2QImage();
        }
    }
    emit frameChanged(qCamImageBits);
}

void QCapture::setCvCapture(CvCapture* camCapture)
{
    this->camCapture = camCapture;
}

void QCapture::Ipl2QImage()
{
    if(camImage && camImage->width > 0) {
        int x, y;
        uchar* data = (uchar*)camImage->imageData;

        if(!qCamImageBits) {
            qCamImageBits = new uchar[camImage->width * camImage->height * BPP];
        }
        uchar* dest = qCamImageBits;

        for(y = 0; y < camImage->height; ++y, data += camImage->widthStep) {
            for(x = 0; x < camImage->width; ++x, dest += BPP) {
                dest[0] = data[x * camImage->nChannels + 2];
                dest[1] = data[x * camImage->nChannels + 1];
                dest[2] = data[x * camImage->nChannels];
            }
        }
    }
}

QWebCam::QWebCam(int captureDevice)
{
    ready = true;
    captureLoop = NULL;
    camCapture = NULL;
    camCapture = cvCreateCameraCapture(captureDevice);
    if (!camCapture) {
        qDebug() << "Webcam not found!";
        ready = false;
        return;
    }

    cvGrabFrame(camCapture);
    IplImage* camImage = cvRetrieveFrame(camCapture);
    qDebug() << "Image size:" << camImage->width << "x" << camImage->height;
    nativeResolution = QSize(camImage->width, camImage->height);
    resolution = QSize(stdResolutionX, stdResolutionY);
    qCamImage = QImage(resolution.width(), resolution.height(), QImage::Format_RGB888);
    qCamImage.fill(qRgb(100, 150, 235));
    setPixmap(QPixmap::fromImage(qCamImage, Qt::AutoColor));

    captureLoop = new QCapture();
    captureLoop->setCvCapture(camCapture);
    connect(captureLoop, SIGNAL(frameChanged(uchar*)), this, SLOT(updateFrame(uchar*)));
    captureLoop->start();
}

QWebCam::~QWebCam()
{
    if(captureLoop) {
        captureLoop->quit();
        captureLoop->wait();
    }
    if(camCapture) {
        cvReleaseCapture(&camCapture);
    }
}

void QWebCam::updateFrame(uchar* bits)
{
    QImage temp(nativeResolution, QImage::Format_RGB888);
    memcpy(temp.bits(), bits, nativeResolution.width() * nativeResolution.height() * BPP);

    if(nativeResolution.width() != resolution.width()) {
        temp = temp.scaledToWidth(resolution.width());
    }
    qCamImage = temp.copy(0, (temp.height() - resolution.height()) / 2, resolution.width(), resolution.height());
    setPixmap(QPixmap::fromImage(qCamImage, Qt::AutoColor));
}

uchar* QWebCam::getData()
{
    return qCamImage.bits();
}

bool QWebCam::getReady()
{
    return ready;
}

QSize QWebCam::getResolution()
{
    return resolution;
}

void QWebCam::setResolution(QSize resolution)
{
    if(resolution != this->resolution) {
        this->resolution = resolution;
        qCamImage = qCamImage.scaled(resolution);
        setPixmap(QPixmap::fromImage(qCamImage, Qt::AutoColor));
    }
}
