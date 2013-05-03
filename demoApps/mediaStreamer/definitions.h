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
enum MSG_TYPE {MSG_TYPE_DATA, MSG_TYPE_ID, MSG_TYPE_TARGET};

// Bits per pixel
static const int BPP = 3 * sizeof(char);

// set in config file
static int dropFrameLimit = 5; // drop frames after # unsend frames
static int FPS = 24;

// media file
static QString aviPath = "video/none.fail";

// connection
static QString defaultTarget = "node://";
static QString defaultIdentifier = "app://";
static QString socketPath = "../../build/targets/boost/boost_sock";

// config file
static const QString defaultFile = "mediaStreamer.conf";
static QString configFile = "";

#endif
