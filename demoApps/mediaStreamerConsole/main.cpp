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
 * @file main.cpp
 * @author Helge Backhaus
 */

#include <QApplication>
#include <QDebug>
#include <QString>
#include "QMainThread.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if(qApp->argc() == 2 && (QString(qApp->argv()[1]).toLower() == "--help" || QString(qApp->argv()[1]).toLower() == "-h")) {
        qDebug();qDebug() << "Usage:" << "mediaStreamer" << "[options]";
        qDebug();qDebug() << "Options:";
        qDebug() << "\t--help\t\tprint this help";
        qDebug() << "\t--file \"path\"\tpath to config file (\"./\" = default)";
        qDebug();
        return 0;
    }
    QMainThread mainThread;
    
    return app.exec();
}
