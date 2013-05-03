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
 * @file QConnectorItem.h
 * @author Helge Backhaus
 */

#ifndef QCONNECTORITEM_H
#define QCONNECTORITEM_H

#include <QDebug>
#include <QGraphicsPolygonItem>
#include <QPainter>
#include <QObject>
#include <QTimeLine>
#include <QGraphicsItemAnimation>

#include "definitions.h"

class QConnectorItem : public QObject, public QGraphicsPolygonItem
{
	Q_OBJECT
	
    public:
        QConnectorItem(int x, int y, char type, QColor color);
        ~QConnectorItem();
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget* widget);
        void setLabel(QString txtLabel);
        QString getLabel();
        void load();
        void unload();

    private:
        QRectF textBox;
        QString txtDraw;
        QPolygonF netletShape;
        char type;
        QTimeLine* timerLoad;
        QGraphicsItemAnimation* aniLoad;
        bool loaded;
            
    private slots:
        void setUnloaded();
};

#endif
