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
 * @file QViewer.h
 * @author Helge Backhaus
 */

#ifndef QVIEWER_H
#define QVIEWER_H

#include <QObject>
#include <QImage>
#include <QPixmap>
#include <QLabel>
#include <QColor>
#include <QDebug>

#include "definitions.h"

class QViewer : public QLabel
{
    Q_OBJECT

    public:
        QViewer();
        ~QViewer();
        void updateView(char* data);
        void setResolution(QSize resolution);

    private:
        QImage qDisplayImage;
        QSize resolution;
};

#endif
