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
 * @file QNenaServer.cpp
 * @author Helge Backhaus
 */

#include "QNenaServer.h"

/***************
 * DATA SERVER *
 ***************/

QNenaDataServer::QNenaDataServer(const QString name) {
  this->name = name;
  bindHandle = NULL;
  handle = NULL;
  state = CLOSED;
  moveToThread(this);
}

QNenaDataServer::~QNenaDataServer() 
{
  bindTimer->stop();
  delete bindTimer;
}

void QNenaDataServer::run()
{
  bindTimer = new QTimer(this);
  bindTimer->setSingleShot(true);
  connect(bindTimer, SIGNAL(timeout()), this, SLOT(bind()));
  
  tmnet::error err;
  err = tmnet::bind(bindHandle, name.toStdString());
  if (err != tmnet::eOk) {
    qDebug() << "ERROR: tmnet::bind(data):" << err;
    return;
  }
  qDebug() << "Binding dataSocket to" << name;
  bindTimer->start();
  exec();
}

void QNenaDataServer::bind()
{
  tmnet::error err;
  if(state == CLOSED) {
    qDebug() << "Waiting on dataSocket";
    state = WAITING;
    err = tmnet::wait(bindHandle);
    if (err != tmnet::eOk) {
      qDebug() << "ERROR: tmnet::wait(data):" << err;
      return;
    }
    err = tmnet::accept(bindHandle, handle);
    if (err != tmnet::eOk) {
      qDebug() << "ERROR: tmnet::accept(data):" << err;
      handle = NULL;
      state = CLOSED;
      return;
    }
    qDebug() << "Accepted on dataSocket";
    state = OPENED;
    std::string value;
    err = tmnet::get_property(handle, "requirements", value);
    if (err != tmnet::eOk) {
      qDebug() << "ERROR: tmnet::get_property(data):" << err;
      close();
      return;
    }
    
    QScriptEngine scriptEngine;
    QScriptValue scriptVal = scriptEngine.evaluate("(" + QString::fromStdString(value) + ")");
    if(scriptVal.property("uri").isString()) {
      QString uri = scriptVal.property("uri").toString();
      QString fileName = uri.mid(uri.lastIndexOf('/') + 1);
      qDebug() << "Request:" << uri;
      qDebug() << "Video:" << fileName;    
      emit newConnection(fileName);
    }
    else {
      qDebug() << "ERROR: Could not parse requirements";
      close();
    }
  }
}

void QNenaDataServer::write(QByteArray block)
{
  tmnet::error err;
  if(state == OPENED) {
    uint bytesToWrite = (uint)block.size();
    err = tmnet::write(handle, (char*)&bytesToWrite, sizeof(bytesToWrite));
    if (err != tmnet::eOk) {
      qDebug() << "ERROR: tmnet::write(data.size):" << err;
      close();
      return;
    }
    err = tmnet::write(handle, block.data(), block.size());
    if (err != tmnet::eOk) {
      qDebug() << "ERROR: tmnet::write(data.bytes):" << err;
      close();
      return;
    }
  }
}

void QNenaDataServer::close()
{
  if(state == OPENED) {
    qDebug() << "Closing dataSocket...";
    state = CLOSING;
    tmnet::error err;
    err = tmnet::close(handle);
    if (err != tmnet::eOk) {
      qDebug() << "ERROR: tmnet::close(data):" << err;
    }
    handle = NULL;
    state = CLOSED;
    bindTimer->start();
    qDebug() << "Data closed";
  }
}

SocketState QNenaDataServer::getState()
{
  return state;
}

/******************
 * CONTROL SERVER *
 ******************/

QNenaCtrlServer::QNenaCtrlServer(const QString name) {
  this->name = name;
  bindHandle = NULL;
  handle = NULL;
  state = CLOSED;
  moveToThread(this);
}

QNenaCtrlServer::~QNenaCtrlServer()
{
  bindTimer->stop();
  delete bindTimer;
  pollTimer->stop();
  delete pollTimer;
}

void QNenaCtrlServer::run()
{
  bindTimer = new QTimer(this);
  bindTimer->setSingleShot(true);
  connect(bindTimer, SIGNAL(timeout()), this, SLOT(bind()));
  
  pollTimer = new QTimer(this);
  connect(pollTimer, SIGNAL(timeout()), this, SLOT(read()));
  
  tmnet::error err;
  err = tmnet::bind(bindHandle, name.toStdString());
  if (err != tmnet::eOk) {
    qDebug() << "ERROR: tmnet::bind(ctrl):" << err;
    return;
  }
  qDebug() << "Binding ctrlSocket to" << name;
  bindTimer->start();
  exec();
}

void QNenaCtrlServer::bind()
{
  tmnet::error err;
  if(state == CLOSED) {
    qDebug() << "Waiting on ctrlSocket";
    state = WAITING;
    err = tmnet::wait(bindHandle);
    if (err != tmnet::eOk) {
      qDebug() << "ERROR: tmnet::wait(ctrl):" << err;
      return;
    }
    err = tmnet::accept(bindHandle, handle);
    if (err != tmnet::eOk) {
      qDebug() << "ERROR: tmnet::accept(ctrl):" << err;
      handle = NULL;
      state = CLOSED;
      return;
    }
    qDebug() << "Accepted on ctrlSocket";
    state = OPENED;
    pollTimer->start(0);
  }
}

void QNenaCtrlServer::read()
{
  tmnet::error err;
  if(state == OPENED) {
    char command;
    size_t size;
    
    size = sizeof(command);
    err = tmnet::read(handle, &command, size);
    if (err != tmnet::eOk) {
      qDebug() << "ERROR: tmnet::read(ctrl):" << err;
      close();
      return;
    }
    
    if(size == sizeof(command)) {
      emit dataReady(command);
    }
  }
}

void QNenaCtrlServer::close()
{
  if(state == OPENED) {
    qDebug() << "Closing ctrlSocket";
    state = CLOSING;
    pollTimer->stop();
    tmnet::error err;
    err = tmnet::close(handle);
    if (err != tmnet::eOk) {
      qDebug() << "ERROR: tmnet::close(ctrl):" << err;
    }
    qDebug() << "Ctrl closed";
    handle = NULL;
    bindTimer->start();
    emit connectionClosed();
    state = CLOSED;
  }
}

SocketState QNenaCtrlServer::getState()
{
  return state;
}
