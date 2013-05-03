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
 * @file definitions.h
 * @author Helge Backhaus
 */

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <QString>

// Boost appConnector message type definitions
//enum MSG_TYPE {MSG_TYPE_DATA, MSG_TYPE_ID, MSG_TYPE_TARGET};
#include "../../src/targets/boost/msg.h"

// Bits per pixel
static const int BPP = 3 * sizeof(char);

// set in config file
// default webcam resolution
static int stdResolutionX = 640;
static int stdResolutionY = 360;
static int altResolutionX = 480;
static int altResolutionY = 360;

// default FPS for webcam stream
static int camFPS = 30; // send # frames per second
static int dropFrameLimit = 5; // drop frames after # unsend frames

// connection
static QList<QString> defaultTargets;
static QString defaultIdentifier = "app://";
static QString socketPath = "../../build/targets/boost/boost_sock";

// config file
static const QString defaultFile = "mediaApp.conf";
static QString configFile = "";

#endif
