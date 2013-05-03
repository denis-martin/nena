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

#ifndef INT64_C
  #define INT64_C(c) (c ## LL)
  #define UINT64_C(c) (c ## ULL)
#endif

extern "C" {
  #include <avcodec.h>
  #include <avformat.h>
  #include <swscale.h>
}

class QViewer : public QLabel
{
  Q_OBJECT

  public:
    QViewer();
    ~QViewer();
    void updateView(AVFrame*);
    void setResolution(QSize);

  private:
    QImage qDisplayImage;
    QSize resolution;
};

#endif
