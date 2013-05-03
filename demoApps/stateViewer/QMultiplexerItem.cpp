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
 * @file QMultiplexerItem.cpp
 * @author Helge Backhaus
 */

#include "QMultiplexerItem.h"

QMultiplexerItem::QMultiplexerItem(int x, int y, char type, QColor color)
{
    setPos(x, y);
    this->type = type;
    sockets = 1;
    this->color = color;
    createShape();
    timerLoad = new QTimeLine(1500);
	timerLoad->setFrameRange(0, 100);
	connect(timerLoad, SIGNAL(finished()), this, SLOT(setUnloaded()));
	aniLoad = new QGraphicsItemAnimation();
	aniLoad->setItem(this);
	aniLoad->setTimeLine(timerLoad);
	aniLoad->setScaleAt(0.0, 1.0, 0.0);
	aniLoad->setScaleAt(1.0, 1.0, 1.0);
	scale(1.0, 0.0);
	loaded = false;
}

QMultiplexerItem::~QMultiplexerItem()
{
	delete aniLoad;
	delete timerLoad;
}

void QMultiplexerItem::setSockets(int sockets)
{
	if(sockets > 0) {
		this->sockets = sockets;
		createShape();
	}
}

void QMultiplexerItem::createShape()
{
	multiplexerShape.clear();

	for(int i = 0; i < sockets; ++i) {
		switch(type) {
			case 0:
				multiplexerShape << QPointF(75 * i, 0) << QPointF(15 + 75 * i, 0) << QPointF(15 + 75 * i, 15)
								 << QPointF(30 + 75 * i, 15) << QPointF(30 + 75 * i, 30) << QPointF(60 + 75 * i, 30)
								 << QPointF(60 + 75 * i, 15) << QPointF(75 + 75 * i, 15) << QPointF(75 + 75 * i, 0);
				break;
			case 1:
				multiplexerShape << QPointF(75 * i, 0) << QPointF(15 + 75 * i, 0) << QPointF(15 + 75 * i, 30)
								 << QPointF(30 + 75 * i, 30) << QPointF(30 + 75 * i, 15) << QPointF(60 + 75 * i, 15)
								 << QPointF(60 + 75 * i, 30) << QPointF(75 + 75 * i, 30) << QPointF(75 + 75 * i, 0);
				break;
			case 2:
				multiplexerShape << QPointF(75 * i, 0) << QPointF(15 + 75 * i, 0) << QPointF(15 + 75 * i, 15)
								 << QPointF(30 + 75 * i, 15) << QPointF(30 + 75 * i, 30) << QPointF(45 + 75 * i, 30)
								 << QPointF(45 + 75 * i, 15) << QPointF(60 + 75 * i, 15) << QPointF(60 + 75 * i, 30)
								 << QPointF(75 + 75 * i, 30) << QPointF(75 + 75 * i, 0);
				break;
			default:
				multiplexerShape << QPointF(75 * i, 0) << QPointF(15 + 75 * i, 0) << QPointF(15 + 75 * i, 15)
								 << QPointF(30 + 75 * i, 15) << QPointF(30 + 75 * i, 30) << QPointF(60 + 75 * i, 30)
								 << QPointF(60 + 75 * i, 15) << QPointF(75 + 75 * i, 15) << QPointF(75 + 75 * i, 0);
				break;
		}
	}
	multiplexerShape << QPointF(15 + 75 * sockets, 0) << QPointF(15 + 75 * sockets, 55) << QPointF(0, 55) << QPointF(0, 0);
	setPolygon(multiplexerShape);
	textBox.setRect(0, 30, boundingRect().width(), boundingRect().height() - 30);

	setPen(QPen(Qt::black, 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	QLinearGradient linearGrad(boundingRect().topLeft(), boundingRect().bottomRight());
	linearGrad.setColorAt(0, color);
	linearGrad.setColorAt(1, color.darker(125));
	setBrush(QBrush(linearGrad));
	update();
}

void QMultiplexerItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget* widget)
{
	QGraphicsPolygonItem::paint(painter, item, widget);
    if(!txtDraw.isEmpty()) {
        painter->drawText(textBox, Qt::AlignHCenter | Qt::AlignVCenter, txtDraw);
    }
}

void QMultiplexerItem::setLabel(QString txtLabel)
{
    txtDraw = txtLabel;
    update();
}

void QMultiplexerItem::load()
{
	setVisible(true);
	timerLoad->setDirection(QTimeLine::Forward);
	if(timerLoad->state() == QTimeLine::NotRunning && !loaded) {
		timerLoad->start();
	}
}

void QMultiplexerItem::unload()
{
	timerLoad->setDirection(QTimeLine::Backward);
	if(timerLoad->state() == QTimeLine::NotRunning && loaded) {
		timerLoad->start();
	}
}

void QMultiplexerItem::setUnloaded()
{
	if(timerLoad->direction() == QTimeLine::Forward) {
		loaded = true;
	}
	else if(timerLoad->direction() == QTimeLine::Backward) {
		loaded = false;
		setVisible(false);
	}
}
