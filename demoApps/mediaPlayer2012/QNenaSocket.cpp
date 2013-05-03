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
 * @file QNenaSocket.cpp
 * @author Helge Backhaus
 */

#include "QNenaSocket.h"

/***************
 * DATA SOCKET *
 ***************/

QNenaDataSocket::QNenaDataSocket(const QString name) {
  this->name = name;
  handle = NULL;
  moveToThread(this);
}

QNenaDataSocket::~QNenaDataSocket()
{
  close();
  terminate();
}

void QNenaDataSocket::run()
{
  setTerminationEnabled(true);
  
  pollTimer = new QTimer(this);
  connect(pollTimer, SIGNAL(timeout()), this, SLOT(read()));
  
  tmnet::error err;
  err = tmnet::get(handle, name.toStdString(), "{\"content-type\": \"application/octet-stream\"}");
  if (err != tmnet::eOk) {
    qDebug() << "ERROR: tmnet::get(data):" << err;
    return;
  }
  pollTimer->start(0);
  exec();
}

void QNenaDataSocket::read()
{
  tmnet::error err;
  size_t size;
  
  uint bytesToRead;
  uint bytesRead;
  
  size = sizeof(bytesToRead);
  err = tmnet::read(handle, (char*)&bytesToRead, size);
  if (err != tmnet::eOk) {
    qDebug() << "ERROR: tmnet::read(data.size):" << err;
    close();
    return;
  }
  
  if(bytesToRead > 0) {
    char readBuffer[bytesToRead];
    bytesRead = 0;    
    
    while(bytesToRead > 0) {
      size = bytesToRead;
      err = tmnet::read(handle, readBuffer + bytesRead, size);
      if (err != tmnet::eOk) {
	qDebug() << "ERROR: tmnet::read(data.bytes):" << err;
	close();
	return;
      }
      bytesToRead -= size;
      bytesRead += size;
    }
    
    QByteArray data((char*)&readBuffer, bytesRead);
    
    emit dataReady(data);
  }
}

void QNenaDataSocket::close()
{
  pollTimer->stop();
  delete pollTimer;
  pollTimer = NULL;
  if(handle) {
    tmnet::error err;
    err = tmnet::close(handle);
    if (err != tmnet::eOk) {
      qDebug() << "ERROR: tmnet::close(data):" << err;
    }
    handle = NULL;
    qDebug() << "Closed data stream";
  }
}

/******************
 * CONTROL SOCKET *
 ******************/

QNenaCtrlSocket::QNenaCtrlSocket(const QString name) {
  handle = NULL;
  tmnet::error err;
  err = tmnet::connect(handle, name.toStdString(), "{\"content-type\": \"text/cmd\"}");
  if (err != tmnet::eOk) {
    qDebug() << "ERROR: tmnet::connect(ctrl):" << err;
  }
}

QNenaCtrlSocket::~QNenaCtrlSocket()
{
  close();
}

void QNenaCtrlSocket::write(char command)
{
  if(handle) {
    tmnet::error err;
    err = tmnet::write(handle, &command, sizeof(command));
    if (err != tmnet::eOk) {
      qDebug() << "ERROR: tmnet::write(ctrl):" << err;
      close();
    }
  }
}

void QNenaCtrlSocket::close()
{
  if(handle) {
    tmnet::error err;
    err = tmnet::close(handle);
    if (err != tmnet::eOk) {
      qDebug() << "ERROR: tmnet::close(ctrl):" << err;
    }
    handle = NULL;
    emit connectionClosed();
  }
}
