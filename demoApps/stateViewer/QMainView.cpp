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
 * @file QMainView.cpp
 * @author Helge Backhaus
 */

#include "QMainView.h"
#include "QNenaConnector.h"

#include <QScriptValue>
#include <QScriptEngine>

#include <assert.h>

using namespace std;

QMainView::QMainView(QWidget* parent) : QWidget(parent)
{
    incomingBuffer = NULL;
    incomingSize = 0;
    readSize = 0;
    nenaConnector = NULL;

    // process options
    for(int i = 0; i < qApp->argc(); ++i) {
        if(QString(qApp->argv()[i]).toLower() == "--file" || QString(qApp->argv()[i]).toLower() == "-f") {
            if(qApp->argc() > i + 1) {
                configFile = QString(qApp->argv()[i + 1]).remove("\"");
                configFile = QDir::cleanPath((QDir::current().absolutePath() + "/" + configFile));
            }
        }
    }

    getConfiguration();
    
	nenaConnector = new QNenaConnector(nenaSocketPath);
	connect(nenaConnector, SIGNAL(dataReady(QString)), this, SLOT(processData(QString)));

    initializeLayout();
	initScene();

	setWindowTitle(tr("NENA Status Viewer"));

	appIcons["unknown"] = new QImage("unknown.png");
	appIcons["app://python3"] = new QImage("web.png"); // web-api
	appIcons["app://mediaPlayer"] = new QImage("video.png");

    stateTimer = new QTimer(this);
    stateTimer->setInterval(1000);
    connect(stateTimer, SIGNAL(timeout()), nenaConnector, SLOT(refreshAppStatus()));

	nenaConnector->start();
	stateTimer->start();
}

void QMainView::getConfiguration()
{  
    QFile qfConf;
    bool opened = false;
    
    if(!configFile.isEmpty()) {
        qfConf.setFileName(configFile);
        if(!qfConf.exists()) {
            qDebug() << "WARNING:" << configFile << "not found!";
        } else if(qfConf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Processing" << configFile;
            opened = true;
        }
    }
    
    if(!opened) {
        qfConf.setFileName(defaultFile);
        if(!qfConf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "WARNING:" << defaultFile << "not found!";
            return;
        } else {
            qDebug() << "Processing" << defaultFile;
        }
    }
    
    QTextStream streamIn(&qfConf);
    while (!streamIn.atEnd()) {
        QString line = streamIn.readLine().simplified();
        if(!line.startsWith("#") && !line.isEmpty()) {
            QStringList lineList = line.split("=");
            if(lineList.size() == 2) {
                QString key = lineList[0].simplified();
                QString value = lineList[1].simplified();

                if(key == "nenasocket") {
                    nenaSocketPath = value.toStdString();
                    qDebug() << "nenasocket:" << nenaSocketPath.c_str();
                }
            }
        }
    }
}

void QMainView::initializeLayout()
{
    // prepare rendering area
    scene.setItemIndexMethod(QGraphicsScene::NoIndex);
    scene.setBackgroundBrush(QBrush(QColor(128, 128, 128)));
    view.setScene(&scene);
    view.setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    view.setMinimumSize(810, 555);
    view.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    updateButton = new QPushButton(tr("Update"));
    connect(updateButton, SIGNAL(clicked()), nenaConnector, SLOT(refreshNenaStatus()));
    
//    restartButton = new QPushButton(tr("Restart NENA daemon"));
//    connect(restartButton, SIGNAL(clicked()), this, SLOT(restartNodeArch()));
    
    // setup window layout
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    controlsLayout->addStretch(1);
    controlsLayout->addWidget(updateButton);
//    controlsLayout->addWidget(restartButton);
    
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addLayout(controlsLayout);
    mainLayout->addWidget(&view);
    setLayout(mainLayout);
}

void QMainView::initScene()
{
    scene.items().clear();
    scene.setSceneRect(0, 0, 800, 545);

    QBoxItem* Background = new QBoxItem(10, 90, 780, 405, QColor(180, 225, 180));
    Background->setZValue(1.0);
    scene.addItem(Background);

    //QConnectorItem* AppConnect = new QConnectorItem(150, 90, 0, QColor(255, 255 , 155));
    //AppConnect->setLabel("Application");
    //AppConnect->setZValue(3.0);
	//scene.addItem(AppConnect);

    /*QConnectorItem* NetAdapt = new QConnectorItem(150, 455, 1, QColor(255, 255 , 155));
    NetAdapt->setLabel("Network\nAdaptor");
    NetAdapt->setZValue(3.0);
	scene.addItem(NetAdapt);*/

    QBoxItem* Selection = new QBoxItem(30, 110, 740, 55, QColor(175, 210, 210), true);
    Selection->setLabel("Netlet Selection");
    Selection->setZValue(2.0);
    scene.addItem(Selection);
/*
    QBoxItem* Mapper = new QBoxItem(340, 90, 85, 55, QColor(175, 210, 210), true);
    Mapper->setLabel("Name\nAddress\nMapper");
    Mapper->setZValue(2.0);
    scene.addItem(Mapper);

    QBoxItem* Decision = new QBoxItem(multiplexerWidth + 40, 90, 85, 55, QColor(175, 210, 210), true);
    Decision->setLabel("Decision\nEngine");
    Decision->setZValue(2.0);
    scene.addItem(Decision);

    QBoxItem* Management = new QBoxItem(multiplexerWidth + 40, 155, 85, 300, QColor(175, 210, 210), true);
    Management->setLabel("Management", true);
    Management->setZValue(2.0);
    scene.addItem(Management);
*/
    QBoxItem* Access = new QBoxItem(30, 410, 740, 45, QColor(175, 210, 210), true);
    Access->setLabel("Network Access Manager");
    Access->setZValue(2.0);
    scene.addItem(Access);
}

QMainView::~QMainView()
{
    if (stateTimer)
    	stateTimer->stop();
    delete stateTimer;
}

void QMainView::processData(QString jsonString)
{
	qDebug() << "Got JSON: " << jsonString;

	QScriptValue scv;
	QScriptEngine engine;
	scv = engine.evaluate("(" + jsonString + ")");

	if (scv.property("architectures").isArray()) {
		QStringList items;
		qScriptValueToSequence(scv.property("architectures"), items);
		if (items.contains("architecture://video"))
			items.push_front("architecture://video");
		if (items.contains("architecture://cdn"))
			items.push_front("architecture://cdn");
		if (items.contains("architecture://web"))
			items.push_front("architecture://web");
		items.removeDuplicates();
		qDebug("adding %d multiplexers", items.size());

		foreach (QString str, items) {
			QString multiName(str);
			qDebug("  %s", multiName.toStdString().c_str());

			if(!multiplexerList.contains(multiName)) {
				QColor color;

				multiplexerList.insert(multiName, new TMultiplexer);
				multiplexerList[multiName]->count = multiplexerList.size() - 1;
				switch (multiplexerList[multiName]->count) {
					case 0: {
						color = QColor(221, 235,  255);
						//color = QColor(255, 216,  33);
					}break;
					case 1: {
						color = QColor(255, 255,  183);
						//color = QColor(255, 255, 195);
					}break;
					case 2: {
						color = QColor(255, 217,  179);
						//color = QColor(255,  89,  33);
					}break;
					default: {
						color = QColor(255, 216,  33);
					}break;
				}
				multiplexerList[multiName]->multiplexer = new QMultiplexerItem(30 + MULTIPLEXER_WIDTH * multiplexerList[multiName]->count,
																			   345, multiplexerList[multiName]->count, color);
				multiplexerList[multiName]->multiplexer->setLabel(multiName);
				multiplexerList[multiName]->multiplexer->setSockets(MULTIPLEXER_SOCKETS);
				multiplexerList[multiName]->multiplexer->setZValue(2.0);
				scene.addItem(multiplexerList[multiName]->multiplexer);
				multiplexerList[multiName]->multiplexer->load();
			}
			else {
				multiplexerList[multiName]->multiplexer->load();
			}

		}

	}

	if (scv.property("netlets").isArray()) {
		QScriptValue netletsArray = scv.property("netlets");

		quint32 i = 0;
		QScriptValue netletRecord = netletsArray.property(i);
		while (netletRecord.property("name").isString()) {
			QString netletName = netletRecord.property("name").toString();
			QString archName = netletRecord.property("arch").toString();
//			qDebug() << netletName << archName;

			if(!multiplexerList[archName]->netletList.contains(netletName)) {
				QColor color;
				switch (multiplexerList[archName]->count) {
					case 0: {
						color = QColor(221, 235,  255);
						//color = QColor(255, 216,  33);
					}break;
					case 1: {
						color = QColor(255, 255,  183);
						//color = QColor(255, 255, 195);
					}break;
					case 2: {
						color = QColor(255, 217,  179);
						//color = QColor(255,  89,  33);
					}break;
					default: {
						color = QColor(255, 216,  33);
					}break;
				}
				multiplexerList[archName]->netletList.insert(netletName,
						new QNetletItem(150 + 75 * (multiplexerList[archName]->netletList.size() - 1) + MULTIPLEXER_WIDTH * multiplexerList[archName]->count,
								335 + 40, 0, 0 /* 40 */, multiplexerList[archName]->count, color));
				multiplexerList[archName]->netletList[netletName]->setLabel(netletName);
				multiplexerList[archName]->netletList[netletName]->setZValue(5);
				scene.addItem(multiplexerList[archName]->netletList[netletName]);
				multiplexerList[archName]->netletList[netletName]->load();
			}
			else {
				multiplexerList[archName]->netletList[netletName]->load();
			}

			i++;
			netletRecord = netletsArray.property(i);
		}

	}

	if (scv.property("netAdapts").isArray()) {
		QScriptValue naArray = scv.property("netAdapts");

		quint32 i = 0;
		QScriptValue naRecord = naArray.property(i);
		while (naRecord.property("name").isString()) {
			QString naName = naRecord.property("name").toString();
			QString archName = naRecord.property("arch").toString();
//			qDebug() << naName << archName;

			QString label = naName;
			label.replace("netadapt://", "");

			bool found = false;
			for (int j = 0; j < multiplexerList[archName]->netAdaptList.count(); j++) {
				if (multiplexerList[archName]->netAdaptList.at(j)->getLabel() == label) {
					multiplexerList[archName]->netAdaptList.at(j)->load();
					found = true;
					break;
				}
			}

			if (!found) {
				QColor color;
				switch (multiplexerList[archName]->count) {
					case 0: {
						color = QColor(221, 235,  255);
						//color = QColor(255, 216,  33);
					}break;
					case 1: {
						color = QColor(255, 255,  183);
						//color = QColor(255, 255, 195);
					}break;
					case 2: {
						color = QColor(255, 217,  179);
						//color = QColor(255,  89,  33);
					}break;
					default: {
						color = QColor(255, 216,  33);
					}break;
				}

				QConnectorItem* NetAdapt = new QConnectorItem(83 + 96 * (multiplexerList[archName]->netAdaptList.count()) + MULTIPLEXER_WIDTH * multiplexerList[archName]->count, 455, 1, color);
				multiplexerList[archName]->netAdaptList.append(NetAdapt);
				QString label = naName;
				label.replace("netadapt://", "");
				NetAdapt->setLabel(label);
				NetAdapt->setZValue(3.0);
				scene.addItem(NetAdapt);
				NetAdapt->load();
			}

			i++;
			naRecord = naArray.property(i);
		}

	}

	if (scv.property("appFlowStates").isArray()) {
		QScriptValue fsArray = scv.property("appFlowStates");

		map<int, QArrowItem*>::iterator ait;
		for (ait = arrows.begin(); ait != arrows.end(); ait++) {
			ait->second->old = true;
//			scene.removeItem(ait->second);
//			delete ait->second;
		}

		quint32 i = 0;
		QScriptValue fsRecord = fsArray.property(i);
		while (fsRecord.property("name").isString()) {
			QString appName = fsRecord.property("name").toString();
			QString netletName = fsRecord.property("netlet").toString();
			QString archName = fsRecord.property("arch").toString();
			qint32 id = fsRecord.property("id").toInt32();
			qint32 method = fsRecord.property("method").toInt32();
			qint32 tx_floating = fsRecord.property("tx_floating").toInt32();
			qint32 rx_floating = fsRecord.property("rx_floating").toInt32();
			qsreal lossRatio = fsRecord.property("stat_remote_lossRatio").toNumber();
			qsreal curLossRatio = fsRecord.property("stat_remote_curLossRatio").toNumber();
			qDebug("%d: %s, %s, method %d (tx %d, rx %d, lossRatio %f, curLossRatio %f)", id, appName.toUtf8().data(),
					netletName.toUtf8().data(), method, tx_floating, rx_floating, lossRatio, curLossRatio);

			ait = arrows.find(id);
			if (ait != arrows.end()) {
				ait->second->old = false;

			} else {
				// search netlet to get coordinates
				if (multiplexerList.contains(archName) && multiplexerList[archName]->netletList.contains(netletName)) {
					QNetletItem* netlet = multiplexerList[archName]->netletList.value(netletName);
					qreal x = netlet->x();
					qreal y = netlet->y();

					QArrowItem* ai = new QArrowItem(x, y-175, 0, 0, method, QColor(0, 200, 0));
					ai->id = id;
	//				ai->setLabel(appName);
					ai->setZValue(5.0);
					scene.addItem(ai);
					arrows[id] = ai;
					ai->load();

					QImage* img = NULL;
					map<QString, QImage*>::iterator aiit;
					aiit = appIcons.find(appName);
					if (aiit != appIcons.end()) {
						img = aiit->second;

					} else {
						img = appIcons["unknown"];

					}
					QGraphicsPixmapItem* icon = new QGraphicsPixmapItem(QPixmap::fromImage(*img));
					activeAppIcons[id] = icon;
					icon->setZValue(4.0);
					icon->setPos(x - img->width()/2, 0);
					scene.addItem(icon);

				} else {
					qDebug() << "Cannot find architecture/netlet";

				}

			}

			i++;
			fsRecord = fsArray.property(i);
		}

		for (ait = arrows.begin(); ait != arrows.end(); ait++) {
			if (ait->second->old) {
				scene.removeItem(ait->second);
				map<int, QGraphicsPixmapItem*>::iterator aaiit;
				aaiit = activeAppIcons.find(ait->second->id);
				if (aaiit != activeAppIcons.end()) {
					scene.removeItem(aaiit->second);
					delete aaiit->second;
					activeAppIcons.erase(aaiit);
				}
				delete ait->second;
				arrows.erase(ait);

			}
		}

	}

//	uint32_t type = NA_RESET;
//	switch (type) {
//		case NA_RESET: {
//			QMap<QString, TMultiplexer*>::iterator itMulti;
//			QMap<QString, QNetletItem*>::iterator itNetlet;
//
//			for(itMulti = multiplexerList.begin(); itMulti != multiplexerList.end(); ++itMulti) {
//				itMulti.value()->multiplexer->unload();
//				for(itNetlet = itMulti.value()->netletList.begin();
//					itNetlet != itMulti.value()->netletList.end(); ++itNetlet) {
//					itNetlet.value()->unload();
//				}
//				for(int i = 0; i < itMulti.value()->netAdaptList.count(); ++i) {
//					itMulti.value()->netAdaptList.at(i)->unload();
//				}
//			}
//		}break;
//	}
}

void QMainView::refreshNenaStatus()
{
    qDebug() << "NYI: QMainView::refreshNenaStatus()";
}

void QMainView::restartNodeArch()
{
//	QProcess* restartNena = new QProcess(this);
//	restartNena->start("./restart.sh");
//
//	qDebug() << "Restarting Nodearch";
	qDebug() << "NYI: QMainView::restartNodeArch()";
}
