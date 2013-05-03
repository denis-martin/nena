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
 * @file QViewer.cpp
 * @author Helge Backhaus
 */

#include "QViewer.h"

QViewer::QViewer()
{
    resolution = QSize(640, 360);
    qDisplayImage = QImage(resolution.width(), resolution.height(), QImage::Format_RGB888);
    qDisplayImage.fill(qRgb(235, 150, 100));
    setPixmap(QPixmap::fromImage(qDisplayImage, Qt::AutoColor));
}

QViewer::~QViewer() {}

void QViewer::updateView(char* data)
{
    memcpy((char*)qDisplayImage.bits(), data, resolution.width() * resolution.height() * BPP);
    setPixmap(QPixmap::fromImage(qDisplayImage.scaled(QSize(resolution.width() * 2, resolution.height() * 2)), Qt::AutoColor));
}

void QViewer::setResolution(QSize resolution)
{
    if(resolution != this->resolution) {
        this->resolution = resolution;
        qDisplayImage = QImage(resolution.width(), resolution.height(), QImage::Format_RGB888);
        setPixmap(QPixmap::fromImage(qDisplayImage.scaled(QSize(resolution.width() * 2, resolution.height() * 2)), Qt::AutoColor));
    }
}
