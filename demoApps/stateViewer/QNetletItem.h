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
 * @file QNetletItem.h
 * @author Helge Backhaus
 */

#ifndef QNETLETITEM_H
#define QNETLETITEM_H

#include <QApplication>
#include <QObject>
#include <QDebug>
#include <QGraphicsPolygonItem>
#include <QPainter>
#include <QTimeLine>
#include <QGraphicsItemAnimation>
#include <QFontMetrics>

#include "definitions.h"

class QNetletItem : public QObject, public QGraphicsPolygonItem
{
    Q_OBJECT
    
    public:
        QNetletItem(int x, int y, int dx, int dy, char type, QColor color);
        ~QNetletItem();
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget* widget);
        void setTarget(int x, int y);
        void setLabel(QString txtLabel);
        //void activate();
        //void deactivate();
        void load();
        void unload();

    private:
        QRectF textBox;
        QPointF start, end;
        //QTimeLine* timerActivate;
        QTimeLine* timerLoad;
        //QGraphicsItemAnimation* aniActivate;
        QGraphicsItemAnimation* aniLoad;
        QString txtLabel;
        QString txtDraw;
        QPolygonF netletShape;
        bool isActivated;
        char type;

    private slots:
        void setItemState();
        //void setUnloaded();
};

#endif
