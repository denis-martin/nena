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

#define NA_RESET            1
#define NA_CONNECT          2
#define NA_MULTIADDED       3
#define NA_ADAPTADDED       4
#define NA_NETLETADDED      5

#define MULTIPLEXER_SOCKETS	3
#define MULTIPLEXER_WIDTH   (79 * MULTIPLEXER_SOCKETS - 1)

// config file
static const QString defaultFile = "stateViewer.conf";
static QString configFile = "";

#endif
