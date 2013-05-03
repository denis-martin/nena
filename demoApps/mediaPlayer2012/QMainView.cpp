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

void QMainView::getConfiguration()
{
  QFile qfConf;

  qfConf.setFileName("mediaPlayer.conf");
  if(!qfConf.exists()) {
    qDebug() << "ERROR: mediaPlayer.conf not found!";
    qApp->exit(0);
  } 
  else if(qfConf.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Processing mediaPlayer.conf";
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
	  if(key == "video_ctrl_name") {
	    ctrlName = value;
	    qDebug() << "video_ctrl_name:" << ctrlName;
	  }
	}
      }
    }
  }
}

QMainView::QMainView(QWidget* parent) : QWidget(parent)
{
  setWindowTitle(tr("NENA Media Player"));
  
  if(qApp->argc() < 2) {
    qDebug() << "ERROR: Usage:" << QString(qApp->argv()[0]) << "[get-request]";
    qApp->exit(0);
  }
  else {
   videoName = QString(qApp->argv()[1]);
   fileName = videoName.mid(videoName.lastIndexOf('/') + 1);
   qDebug() << "Request:" << videoName;
   qDebug() << "Video:" << fileName;     
  }
  
  getConfiguration();
  
  initializeLayout();

  av_register_all();
  
  if(setupStream(QDir::cleanPath((QDir::current().absolutePath() + "/videos/" + fileName)))) {
    qDebug() << "Streaming context successfully created";
  }
  else
  {
    qDebug() << "ERROR: Could not create streaming context";
    exitPlayer();
  }
  
  tmnet::error err = tmnet::init();
  if (err != tmnet::eOk) {
    qDebug() << "ERROR: Could not initialize tmnet:" << err;
    exitPlayer();
  }

  if (!nenaSocketPath.isEmpty()) {
    tmnet::plugin::set_option("tmnet::nena", "ipcsocket", nenaSocketPath.toStdString());
  }
  
  videoSocket = new QNenaDataSocket(videoName);
  connect(videoSocket, SIGNAL(dataReady(QByteArray)), this, SLOT(readData(QByteArray)));
  
  ctrlSocket = new QNenaCtrlSocket(ctrlName);
  connect(ctrlSocket, SIGNAL(connectionClosed()), this, SLOT(exitPlayer()));
  
  videoSocket->start();
  playing = true;
}

QMainView::~QMainView()
{
  delete viewer;
  delete btnStart;
  delete btnRewind;
  delete btnPlayPause;
  delete btnForward;
  delete lblState;
  
  delete ctrlSocket;
  
  av_free(pFrame);
  av_free(pFrameRGB);
  avcodec_close(pCodecCtx);
        
  videoSocket->close();
  videoSocket->quit();  
  videoSocket->wait();
}

void QMainView::closeEvent(QCloseEvent *event) 
{ 
  event->accept(); 
  exitPlayer();
} 

void QMainView::initializeLayout()
{
  // load graphics
  pStart.load("./images/start.png");
  pRewind.load("./images/rewind.png");
  pPlay.load("./images/play.png");
  pPause.load("./images/pause.png");
  pForward.load("./images/forward.png");
  
  // setup controls
  btnStart = new QPushButton(pStart, "");
  btnStart->setIconSize(QSize(62, 62));
  btnStart->setMinimumSize(QSize(82, 82));
  connect(btnStart, SIGNAL(clicked()), this, SLOT(clickedStart()));
  btnRewind = new QPushButton(pRewind, "");
  btnRewind->setIconSize(QSize(62, 62));
  btnRewind->setMinimumSize(QSize(82, 82));
  connect(btnRewind, SIGNAL(clicked()), this, SLOT(clickedRewind()));
  btnPlayPause = new QPushButton(pPause, "");
  btnPlayPause->setIconSize(QSize(62, 62));
  btnPlayPause->setMinimumSize(QSize(82, 82));
  connect(btnPlayPause, SIGNAL(clicked()), this, SLOT(clickedPlayPause()));
  btnForward = new QPushButton(pForward, "");
  btnForward->setIconSize(QSize(62, 62));
  btnForward->setMinimumSize(QSize(82, 82));
  connect(btnForward, SIGNAL(clicked()), this, SLOT(clickedForward()));
  
  QHBoxLayout* controlsLayout = new QHBoxLayout();
  controlsLayout->addStretch(1);
  controlsLayout->addWidget(btnStart);
  controlsLayout->addWidget(btnRewind);
  controlsLayout->addWidget(btnPlayPause);
  controlsLayout->addWidget(btnForward);
  controlsLayout->addStretch(1);
  
  viewer = new QViewer();
  QFont lblFont;
  lblFont.setPointSize(14);
  lblFont.setBold(true);
  lblState = new QLabel("Playing " + videoName);
  lblState->setFont(lblFont);
  
  QHBoxLayout* viewerLayout = new QHBoxLayout();
  viewerLayout->addStretch(1);
  viewerLayout->addWidget(viewer);
  viewerLayout->addStretch(1);
  
  QVBoxLayout* mainLayout = new QVBoxLayout();
  mainLayout->addWidget(lblState);
  mainLayout->addStretch(1);
  mainLayout->addLayout(viewerLayout);
  mainLayout->addStretch(1);
  mainLayout->addLayout(controlsLayout);
  setLayout(mainLayout);
}

void QMainView::readData(QByteArray data)
{
  if(data.remove(0, 1).data()[0] == 'f') {
    avcodec_flush_buffers(pCodecCtx);
  }
  // Decode video frame
  AVPacket pkt;
  av_init_packet(&pkt);
  pkt.data = (uint8_t*)data.data();
  pkt.size = data.size();
  pkt.flags = AV_PKT_FLAG_KEY;
  int frameFinished = 0;
  avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &pkt);

  // Did we get a video frame?
  if(frameFinished != 0) {
    sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
    viewer->setResolution(QSize(pCodecCtx->width, pCodecCtx->height));
    viewer->updateView(pFrameRGB);
  }
}

void QMainView::clickedStart()
{
  ctrlSocket->write('s'); // seek start
  if(!playing) {
    playing = true;
    btnPlayPause->setIcon(pPause);
  }
}

void QMainView::clickedRewind()
{
  ctrlSocket->write('r'); // rewind
  if(!playing) {
    playing = true;
    btnPlayPause->setIcon(pPause);
  }
}

void QMainView::clickedPlayPause()
{
  playing = !playing;
  if(playing) {
    ctrlSocket->write('p'); // play
    btnPlayPause->setIcon(pPause);
  }
  else
  {
    ctrlSocket->write('w'); // wait
    btnPlayPause->setIcon(pPlay);
  }
}

void QMainView::clickedForward()
{
  ctrlSocket->write('f'); // fast forward
  if(!playing) {
    playing = true;
    btnPlayPause->setIcon(pPause);
  }
}

void QMainView::exitPlayer()
{
  qDebug() << "Goodbye";
  qApp->exit(0);
}

bool QMainView::setupStream(QString file)
{
  AVFormatContext* pFormatCtx;
  AVCodec* pCodec;
  int videoStream;
    
  // Open video file
  if(av_open_input_file(&pFormatCtx, file.toAscii(), NULL, 0, NULL) != 0) {
    qDebug() << "ERROR: av_open_input()";
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

  // Allocate video frame
  pFrame = avcodec_alloc_frame();

  // Allocate an AVFrame structure
  pFrameRGB = avcodec_alloc_frame();

  qDebug() << "Frames allocated successfully";
  
  // Determine required buffer size and allocate buffer
  int numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
  unsigned char* buffer = new unsigned char[numBytes];

  // Assign appropriate parts of buffer to image planes in pFrameRGB
  avpicture_fill((AVPicture*)pFrameRGB, (uint8_t*)buffer, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

  // Convert the image into YUV format that SDL uses
  img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
  if(img_convert_ctx == NULL) {
    qDebug() << "ERROR: Cannot initialize image conversion context";
    return false;
  }
  return true;
}