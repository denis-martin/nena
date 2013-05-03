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
 * @file QConnectorItem.cpp
 * @author Helge Backhaus
 */

#include "QConnectorItem.h"

QConnectorItem::QConnectorItem(int x, int y, char type, QColor color)
{
    setPos(x, y);
    this->type = type;

    switch(type) {
		case 0:
			netletShape << QPointF(-43, -80) << QPointF(43, -80) << QPointF(43, -25)
						<< QPointF(0, -25)    << QPointF(0, 0)     << QPointF(0, -25)
						<< QPointF(-43, -25)  << QPointF(-43, -80);
			break;
		case 1:
			netletShape << QPointF(-43, 25)  << QPointF(0, 25)  << QPointF(0, 0)
						<< QPointF(0, 25)    << QPointF(43, 25) << QPointF(43, 80)
						<< QPointF(-43, 80) << QPointF(-43, 25);
			break;
    }

    setPolygon(netletShape);
    setPen(QPen(Qt::black, 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QLinearGradient linearGrad(boundingRect().topLeft(), boundingRect().bottomRight());
    linearGrad.setColorAt(0, color);
    linearGrad.setColorAt(1, color.darker(125));
    setBrush(QBrush(linearGrad));
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

QConnectorItem::~QConnectorItem()
{
	delete aniLoad;
	delete timerLoad;
}

void QConnectorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget* widget)
{
	painter->setPen(QPen(Qt::black, 5.0, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
	switch(type) {
		case 0:
			painter->drawLine(QPointF(0, -25), QPointF(0, 0));
			break;
		case 1:
			painter->drawLine(QPointF(0, 25), QPointF(0, 0));
			break;
	}
	painter->setPen(QPen(Qt::black, 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

	QGraphicsPolygonItem::paint(painter, item, widget);

    if(!txtDraw.isEmpty()) {
        painter->drawText(textBox, Qt::AlignHCenter | Qt::AlignVCenter, txtDraw);
    }
}

void QConnectorItem::setLabel(QString txtLabel)
{
    this->txtDraw = txtLabel;
    switch(type) {
		case 0:
			textBox.setRect(-43, -80, 86, 55);
			break;
		case 1:
			textBox.setRect(-43, 25, 86, 55);
			break;
	}
    update();
}

QString QConnectorItem::getLabel()
{
	return txtDraw;
}

void QConnectorItem::load()
{
	setVisible(true);
	timerLoad->setDirection(QTimeLine::Forward);
	if(timerLoad->state() == QTimeLine::NotRunning && !loaded) {
		timerLoad->start();
	}
}

void QConnectorItem::unload()
{
	timerLoad->setDirection(QTimeLine::Backward);
	if(timerLoad->state() == QTimeLine::NotRunning && loaded) {
		timerLoad->start();
	}
}

void QConnectorItem::setUnloaded()
{
	if(timerLoad->direction() == QTimeLine::Forward) {
		loaded = true;
	}
	else if(timerLoad->direction() == QTimeLine::Backward) {
		loaded = false;
		setVisible(false);
	}
}
