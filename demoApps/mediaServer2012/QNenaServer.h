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
 * @file QNenaServer.h
 * @author Helge Backhaus
 */

#ifndef QNENASERVER_H_
#define QNENASERVER_H_

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QString>
#include <QTimer>
#include <QScriptEngine>

#include "tmnet/net.h"
#include "tmnet/net_plugin.h"

enum SocketState {WAITING, OPENED, CLOSING, CLOSED};

class QNenaDataServer : public QThread
{
  Q_OBJECT

  private:
    SocketState state;
    QTimer* bindTimer;
    QString name;
    tmnet::pnhandle bindHandle;
    tmnet::pnhandle handle;
    
    void run();

  public:
    QNenaDataServer(const QString);
    ~QNenaDataServer();
    
    SocketState getState();
    
  public slots:
    void write(QByteArray);
    void close();
    
  private slots:
    void bind();
    
  signals:
    void newConnection(QString);
};

class QNenaCtrlServer : public QThread
{
  Q_OBJECT
  
  private:
    SocketState state;
    QTimer* bindTimer;
    QTimer* pollTimer;
    QString name;
    tmnet::pnhandle bindHandle;
    tmnet::pnhandle handle;
    
    void run();
    
  public slots:
    void close();
    
  private slots:
    void read();
    void bind();

  public:
    QNenaCtrlServer(const QString);
    virtual ~QNenaCtrlServer();

    SocketState getState();
    
  signals:
    void connectionClosed();
    void dataReady(char);
};

#endif
