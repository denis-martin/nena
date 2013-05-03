/** @file
 * debug.h
 *
 * @brief Some general functions for debugging.
 *
 * Please use only the following macros:
 *
 * DBG_INFO("Message"); // to print an info message <br>
 * DBG_DEBUG("Message"); // to print a debug message <br>
 * DBG_WARNING("Message"); // to print a warning message <br>
 * DBG_ERROR("Message"); // to print an error message <br>
 * DBG_FAIL("Message"); // to print an error message and stop the execution immediately <br>
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 9, 2008
 *      Author: denis
 *	Modified on: Sep 28, 2009
 *		Author: Benjamin Behringer
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <string>
#include <iostream>
#include <fstream>
#include <exception>
#include <ostream>

#include <stdlib.h>

#include <boost/format.hpp>

#define FMT boost::format

//#define DBG_PRINT Debug::print
#define DBG_INFO(msg...) globalChannel->info(boost::format("%1%") % (msg))
#define DBG_DEBUG(msg...) globalChannel->debug(boost::format("%1%") % (msg))
#define DBG_WARNING(msg...) globalChannel->warning(boost::format("%1%") % (msg))
#define DBG_ERROR(msg...) globalChannel->error(boost::format("%1%:%2%: %3%") % __FILE__ % __LINE__ % (msg))
#define DBG_FAIL(msg...) globalChannel->fail(boost::format("%1%:%2%: %3%") % __FILE__ % __LINE__ % (msg))

/**
 * @brief Debug interface class.
 *
 * Will be used by nodearch, system wrapper has to deliver an implementation
 */

class IDebugChannel
{
	public:
	enum VerbosityLevel { QUIET = 0,			///< Do not output to the console at all
						  FAIL,		         	///< Event that causes application to end
						  ERROR,                ///< Critical error, but application can commence
						  WARNING,              ///< Minor error, can work around
						  DEBUG,                ///< technical info, app state etc...
						  INFO };               ///< else

	IDebugChannel (VerbosityLevel vl): vlevel(vl) {}
	virtual ~IDebugChannel () {}



	virtual void fail(boost::format fmt) = 0;
	virtual void error(boost::format fmt) = 0;
	virtual void warning(boost::format fmt) = 0;
	virtual void debug(boost::format fmt) = 0;
	virtual void info(boost::format fmt) = 0;

	virtual void setVerbosity (VerbosityLevel l) { vlevel = l; }
	virtual VerbosityLevel getVerbosity () const { return vlevel; }

	protected:
	/// indicates which messages shall be displayed
	VerbosityLevel vlevel;

};


// Pointer to Architecture specific channel, filled by system Wrapper
extern IDebugChannel * globalChannel;

#endif /* DEBUG_H_ */

