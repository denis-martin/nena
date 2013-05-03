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
 * @file QBoxItem.cpp
 * @author Helge Backhaus
 */

#include "QBoxItem.h"

QBoxItem::QBoxItem(qreal x, qreal y, qreal width, qreal height, QColor color, bool gradient)
{
    setRect(0, 0, width, height);
    setPos(x, y);
    setPen(QPen(Qt::black, 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if(gradient) {
        QLinearGradient linearGrad(QPointF(0.0, 0.0), QPointF(width, height));
        linearGrad.setColorAt(0, color);
        linearGrad.setColorAt(1, color.darker(125));
        setBrush(QBrush(linearGrad));
    } else {
        setBrush(QBrush(color));
    }
    rotated = false;
}

void QBoxItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget* widget)
{
    QGraphicsRectItem::paint(painter, item, widget);
    if(!txtDraw.isEmpty()) {
        if(rotated) {
            painter->rotate(-90.0);
        }
        painter->drawText(textBox, Qt::AlignHCenter | Qt::AlignVCenter, txtDraw);
        painter->resetTransform();
    }
}

void QBoxItem::setLabel(QString txtLabel, bool rotated)
{
    this->txtDraw = txtLabel;
    this->rotated = rotated;
    if(rotated) {
        textBox.setRect(-boundingRect().height(), 0, boundingRect().height(), boundingRect().width());
    } else {
        textBox = boundingRect();
    }
    update();
}
