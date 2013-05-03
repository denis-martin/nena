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
 * @file QNenaSocket.h
 * @author Helge Backhaus
 */

#ifndef QNENASOCKET_H_
#define QNENASOCKET_H_

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QString>
#include <QTimer>

#include "tmnet/net.h"
#include "tmnet/net_plugin.h"

class QNenaDataSocket : public QThread
{
  Q_OBJECT
  
  private:
    QTimer* pollTimer;
    QString name;
    tmnet::pnhandle handle;
    
    void run();
    
  private slots:
    void read();

  public:
    QNenaDataSocket(const QString);
    ~QNenaDataSocket();
    void close();
    
  signals:
    void dataReady(QByteArray);
};

class QNenaCtrlSocket : public QObject
{
  Q_OBJECT

  private:
    tmnet::pnhandle handle;
    
  public:
    QNenaCtrlSocket(const QString);
    ~QNenaCtrlSocket();
    void write(char);
    void close();
    
  signals:
    void connectionClosed();
};

#endif
