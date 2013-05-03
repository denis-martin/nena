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
 * @file QMainThread.cpp
 * @author Helge Backhaus
 */

#include "QMainThread.h"

void QMainThread::getConfiguration()
{
  QFile qfConf;

  qfConf.setFileName("mediaServer.conf");
  if(!qfConf.exists()) {
    qDebug() << "ERROR: mediaServer.conf not found!";
    qApp->exit(0);
  } 
  else if(qfConf.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Processing mediaServer.conf";
    QTextStream streamIn(&qfConf);
    while (!streamIn.atEnd()) {
      QString line = streamIn.readLine().simplified();
      if(!line.startsWith("#") && !line.isEmpty()) {
	QStringList lineList = line.split("=");
	if(lineList.size() == 2) {
	  QString key = lineList[0].simplified();
	  QString value = lineList[1].simplified();
	  if(key == "nenasocket") {
	    nenaSocketPath = value;
	    qDebug() << "nenasocket:" << nenaSocketPath;
	  }
	  if(key == "video_name") {
	    videoName = value;
	    qDebug() << "video_name:" << videoName;
	  }
	  if(key == "video_ctrl_name") {
	    ctrlName = value;
	    qDebug() << "video_ctrl_name:" << ctrlName;
	  }
	}
      }
    }
  }
}

QMainThread::QMainThread(QObject* parent) : QObject(parent)
{
  currentTime = 0;
  pFormatCtx = NULL;
  pCodecCtx = NULL;
  pCodec = NULL;
  videoStream = -1;
  isStreaming = false;
  
  sendTimer = new QTimer(this);
  sendTimer->setSingleShot(true);
  connect(sendTimer, SIGNAL(timeout()), this, SLOT(sendData()));
  
  getConfiguration();
  
  av_register_all();
  
  tmnet::error err = tmnet::init();
  if (err != tmnet::eOk) {
    qDebug() << "ERROR: Could not initialize tmnet:" << err;
    qApp->exit(0);
  }

  if (!nenaSocketPath.isEmpty()) {
    tmnet::plugin::set_option("tmnet::nena", "ipcsocket", nenaSocketPath.toStdString());
  }
  
  videoSocket = new QNenaDataServer(videoName);
  connect(videoSocket, SIGNAL(newConnection(QString)), this, SLOT(gotVideoRequest(QString)));
  connect(this, SIGNAL(writeData(QByteArray)), videoSocket, SLOT(write(QByteArray)));
  connect(this, SIGNAL(resetData()), videoSocket, SLOT(close()));
  
  ctrlSocket = new QNenaCtrlServer(ctrlName);
  connect(ctrlSocket, SIGNAL(connectionClosed()), this, SLOT(resetServer()));
  connect(ctrlSocket, SIGNAL(dataReady(char)), this, SLOT(gotCommand(char)));
  connect(this, SIGNAL(resetControl()), ctrlSocket, SLOT(close()));
  
  videoSocket->start();
  ctrlSocket->start();
}

QMainThread::~QMainThread()
{
  qDebug() << "Goodbye";
  
  if(sendTimer) {
    sendTimer->stop();
    delete sendTimer;
  }
  if(pCodecCtx) {
    avcodec_close(pCodecCtx);
    av_freep(pCodecCtx);
  }
  if(pFormatCtx) {
    av_close_input_file(pFormatCtx);
    av_freep(pFormatCtx);
  }
  if(pCodec) {
    av_freep(pCodec);
  }
  
  videoSocket->close();
  videoSocket->quit();  
  videoSocket->wait();

  ctrlSocket->close();
  ctrlSocket->quit();  
  ctrlSocket->wait();
}

   
void QMainThread::gotVideoRequest(QString fileName) {
  if(streamFile(QDir::cleanPath(QDir::current().absolutePath() + "/videos/" + fileName))) {
    isStreaming = true;
    sendTimer->start(updateCurrentFrame());
  }
  else
  {
    resetServer();
  }
}

void QMainThread::gotCommand(char command)
{
  switch(command)
  {
    case 's': {
      qDebug() << "Command: |<";
      seekStream(0, true);
    }break;
    case 'r': {
      qDebug() << "Command: <<";
      seekStream(-10);
    }break;
    case 'p': {
      qDebug() << "Command: >";
      sendTimer->start(updateCurrentFrame());
    }break;
    case 'w': {
      qDebug() << "Command: ||";
      sendTimer->stop();
      emit writeData(currentFrame);
    }break;
    case 'f': {
      qDebug() << "Command: >>";
      seekStream(10);
    }break;
  }
}

void QMainThread::resetServer()
{  
  qDebug() << "Server resetting";
  isStreaming = false;
  if(sendTimer) {
    sendTimer->stop();
  }
  if(videoSocket->getState() == OPENED) {
    emit resetData();
  }
  if(ctrlSocket->getState() == OPENED) {
    emit resetControl();
  }
  pCodec = NULL;
  if(pCodecCtx) {
    avcodec_close(pCodecCtx);
  }
  pCodecCtx = NULL;
  if(pFormatCtx) {
    av_close_input_file(pFormatCtx);
  }
  pFormatCtx = NULL;

  currentTime = 0;
  videoStream = -1;
  
  qDebug() << "Reset done";
}

int QMainThread::updateCurrentFrame(bool flush)
{
  int delta = 0;
  if(isStreaming) {
    bool gotFrame = false;
    while(!gotFrame) {
      int loop = 0;
      loop = av_read_frame(pFormatCtx, &packet);
      if(loop < 0) {
	av_seek_frame(pFormatCtx, videoStream, 0, AVSEEK_FLAG_BACKWARD);    
	av_read_frame(pFormatCtx, &packet);
	currentTime = 0;
      }
      // Is this a packet from the video stream?
      if(packet.stream_index == videoStream) {
	currentFrame.clear();
	if(flush) {
	  currentFrame.append('f');
	}
	else {
	  currentFrame.append('c');
	}
	currentFrame.append((char*)packet.data, packet.size);
	
	delta = (int)((packet.pts - currentTime) * av_q2d(pFormatCtx->streams[videoStream]->time_base) * 1000.0);
	if(delta < 0) {
	  delta = 0;
	}
	currentTime = packet.pts;
	gotFrame = true;
      }
      // Free the packet that was allocated by av_read_frame
      av_free_packet(&packet);
    }
  }
  return delta;
}

void QMainThread::sendData()
{
  if(videoSocket->getState() == OPENED) {
    emit writeData(currentFrame);
    sendTimer->start(updateCurrentFrame());
  }
}

void QMainThread::seekStream(int delta_s, bool absolute)
{
  sendTimer->stop();
  if(isStreaming) {
    double time_base = av_q2d(pFormatCtx->streams[videoStream]->time_base);
    int64_t pos;
    if(absolute) {
      pos = (int64_t)(delta_s / time_base);
    }
    else {
      pos = (int64_t)((currentTime * time_base + delta_s) / time_base);
    }
    if(pos < 0) {
      pos = 0;
    }
    av_seek_frame(pFormatCtx, videoStream, pos, delta_s < 0 ? AVSEEK_FLAG_BACKWARD : 0);
    currentTime = pos;
  }
  sendTimer->start(updateCurrentFrame(true));
}

bool QMainThread::streamFile(QString file)
{
  // Open video file
  if(av_open_input_file(&pFormatCtx, file.toAscii(), NULL, 0, NULL) != 0) {
    qDebug() << "ERROR: av_open_input_file()";
    return false; // Couldn't open file
  }

  // Retrieve stream information
  if(av_find_stream_info(pFormatCtx) < 0) {
    qDebug() << "ERROR: av_find_stream_info()";
    return false; // Couldn't find stream information
  }

  // Find the first video stream
  videoStream = -1;
  for(unsigned int i = 0; i < pFormatCtx->nb_streams; ++i) {
    if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStream = i;
      break;
    }
  }
  if(videoStream == -1) {
    qDebug() << "ERROR: No video stream found";
    return false; // Didn't find a video stream
  }

  // Get a pointer to the codec context for the video stream
  pCodecCtx = pFormatCtx->streams[videoStream]->codec;

  // Find the decoder for the video stream
  pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec == NULL) {
    qDebug() << "ERROR: No codec found";
    return false; // Codec not found
  }

  // Open codec
  if(avcodec_open(pCodecCtx, pCodec) < 0) {
    qDebug() << "ERROR: Could not open codec";
    return false; // Could not open codec
  }

  qDebug() << "Started streaming of" << file;
  return true;
}

