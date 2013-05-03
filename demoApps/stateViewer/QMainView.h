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
 * @file QMainView.h
 * @author Helge Backhaus
 */

#ifndef QMAINVIEW_H
#define QMAINVIEW_H

#include <QtGui>
#include <QObject>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QTimer>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QVector>
#include <QTextStream>
#include <QProcess>

#include <sys/types.h>
#include <stdint.h>
#include <netinet/in.h>

#include "definitions.h"
#include "QNetletItem.h"
#include "QBoxItem.h"
#include "QMultiplexerItem.h"
#include "QConnectorItem.h"
#include "QArrowItem.h"

class QNenaConnector;

class QMainView : public QWidget
{
    Q_OBJECT

    public:
    	QNenaConnector* nenaConnector;

        QMainView(QWidget* parent = 0);
        ~QMainView();

    private:
        void getConfiguration();
        void initializeLayout();
        void initScene();

        std::string nenaSocketPath;

        QGraphicsView view;
        QGraphicsScene scene;
        QPushButton* updateButton;
        QPushButton* restartButton;

        char* incomingBuffer;
        uint32_t incomingSize;
        uint32_t readSize;

	    class TMultiplexer
    	{
	    	public:
        		int count;
            	QMultiplexerItem* multiplexer;
            	QMap<QString, QNetletItem*> netletList; 
            	QList<QConnectorItem*> netAdaptList;
    	}; 

	    std::map<QString, QImage*> appIcons;

	    QMap<QString, TMultiplexer*> multiplexerList;
	    std::map<int, QArrowItem*> arrows;
	    std::map<int, QGraphicsPixmapItem*> activeAppIcons;

        QTimer* stateTimer;

    private slots:
        void processData(QString jsonString);
        void refreshNenaStatus();
        void restartNodeArch();
//        void pingNodearch();
};

#endif
