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
 * @file QNetletItem.cpp
 * @author Helge Backhaus
 */

#include "QNetletItem.h"

QNetletItem::QNetletItem(int x, int y, int dx, int dy, char type, QColor color)
{
    setPos(x, y);

    start = QPoint(x, y);
    end = QPoint(x + dx, y + dy);

    /*timerActivate = new QTimeLine(1500);
	timerActivate->setFrameRange(0, 100);
	connect(timerActivate, SIGNAL(finished()), this, SLOT(setItemStateActivate()));
	aniActivate = new QGraphicsItemAnimation();
	aniActivate->setItem(this);
	aniActivate->setTimeLine(timerActivate);
	aniActivate->setPosAt(0.0, start);
	aniActivate->setPosAt(1.0, end);*/

	timerLoad = new QTimeLine(3000);
	timerLoad->setFrameRange(0, 100);
	connect(timerLoad, SIGNAL(finished()), this, SLOT(setItemState()));
	aniLoad = new QGraphicsItemAnimation();
	aniLoad->setItem(this);
	aniLoad->setTimeLine(timerLoad);
	aniLoad->setScaleAt(0.0, 0.0, 1.0);
	aniLoad->setScaleAt(0.5, 1.0, 1.0);
	aniLoad->setScaleAt(1.0, 1.0, 1.0);
	aniLoad->setPosAt(0.0, start);
	aniLoad->setPosAt(0.5, start);
	aniLoad->setPosAt(1.0, end);
	scale(0.0, 1.0);

    isActivated = false;
    this->type = type;

    switch(type) {
		case 0:
			netletShape << QPointF(-30, -180) << QPointF(30, -180) << QPointF(30, -15)
						<< QPointF(15, -15)   << QPointF(15, 0)    << QPointF(-15, 0)
						<< QPointF(-15, -15)  << QPointF(-30, -15) << QPointF(-30, -180);
			break;
		case 1:
			netletShape << QPointF(-30, -180) << QPointF(30, -180) << QPointF(30, 0)
						<< QPointF(15, 0)     << QPointF(15, -15)  << QPointF(-15, -15)
						<< QPointF(-15, 0)    << QPointF(-30, 0)   << QPointF(-30, -180);
			break;
		case 2:
			netletShape << QPointF(-30, -180) << QPointF(30, -180) << QPointF(30, 0)
						<< QPointF(15, 0)     << QPointF(15, -15)  << QPointF(0, -15)
						<< QPointF(0, 0)      << QPointF(-15, 0)   << QPointF(-15, -15)
						<< QPointF(-30, -15)  << QPointF(-30, -180);
			break;
		default:
			netletShape << QPointF(-30, -180) << QPointF(30, -180) << QPointF(30, -15)
						<< QPointF(15, -15)   << QPointF(15, 0)    << QPointF(-15, 0)
						<< QPointF(-15, -15)  << QPointF(-30, -15) << QPointF(-30, -180);			
			break;
    }
    setPolygon(netletShape);
    setPen(QPen(Qt::black, 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QLinearGradient linearGrad(boundingRect().topLeft(), boundingRect().bottomRight());
    linearGrad.setColorAt(0, color);
    linearGrad.setColorAt(1, color.darker(125));
    setBrush(QBrush(linearGrad));
}

QNetletItem::~QNetletItem()
{
	//delete aniActivate;
	delete aniLoad;
	//delete timerActivate;
	delete timerLoad;
}

void QNetletItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget* widget)
{
	QGraphicsPolygonItem::paint(painter, item, widget);

    if(!txtLabel.isEmpty()) {
    	painter->rotate(-90.0);
        painter->drawText(textBox, Qt::AlignHCenter | Qt::AlignVCenter, txtDraw);
        painter->resetTransform();
    }
}

void QNetletItem::setLabel(QString txtLabel)
{
    this->txtLabel = txtLabel;
    textBox.setRect(22.0, -boundingRect().width() / 2.0, boundingRect().height() - 40, boundingRect().width());
    txtDraw = txtLabel;
    txtDraw.remove(0, txtDraw.lastIndexOf("/") + 1);
    txtDraw.replace("Simple", "");
    int parsed = 0;
    int linebreak = 0;
    int length = 0;
    int insert = 0;
    QFontMetrics metric(qApp->font());

    while(parsed < txtDraw.size() && linebreak < 2) {

        if(txtDraw[parsed] >= 'A' && txtDraw[parsed] <= 'Z') {
        	insert = parsed;
        }
        length += metric.boundingRect(txtDraw[parsed]).width();
        if(length > textBox.width()) {
			length = 0;
			if(insert > 0) {
				txtDraw.insert(insert, "\n");
			} else {
				txtDraw.insert(parsed, "\n");
			}
			insert = 0;
			parsed++;
			linebreak++;
		}
        parsed++;
    }
    length = metric.boundingRect("...").width();
    while(parsed < txtDraw.size()) {
    	length += metric.boundingRect(txtDraw[parsed]).width();
    	if(length > textBox.width()) {
    		break;
    	}
    	parsed++;
    }
    if(parsed < txtDraw.size()) {
        txtDraw.chop(txtDraw.size() - parsed + 1);
        txtDraw.append("...");
    }
    update();
}

/*void QNetletItem::activate()
{
	timerActivate->setDirection(QTimeLine::Forward);
	if(timerActivate->state() == QTimeLine::NotRunning && !isActivated) {
		timerActivate->start();
	}
}

void QNetletItem::deactivate()
{
	timerActivate->setDirection(QTimeLine::Backward);
	if(timerActivate->state() == QTimeLine::NotRunning && isActivated) {
		timerActivate->start();
	}
}*/

void QNetletItem::load()
{
	setVisible(true);
	timerLoad->setDirection(QTimeLine::Forward);
	if(timerLoad->state() == QTimeLine::NotRunning && !isActivated) {
		timerLoad->start();
	}
}

void QNetletItem::unload()
{
	timerLoad->setDirection(QTimeLine::Backward);
	if(timerLoad->state() == QTimeLine::NotRunning && isActivated) {
		timerLoad->start();
	}
}

void QNetletItem::setItemState()
{
    if(timerLoad->direction() == QTimeLine::Forward) {
        isActivated = true;
    } else if(timerLoad->direction() == QTimeLine::Backward) {
        isActivated = false;
        setVisible(false);
    }
}
