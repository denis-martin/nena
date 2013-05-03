/** @file
 * exceptions.h
 *
 * @brief nena specific exceptions
 *
 * (c) 2008-2010 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 29, 2010
 *      Author: Benjamin Behringer
 */

#ifndef _EXCEPTIONS_H_
#define _EXCEPTIONS_H_

#include <exception>
#include <string>

#define NENA_EXCEPTION(classname)	class classname : public std::exception \
									{ \
										protected: \
											std::string msg; \
										public: \
											classname() throw () : msg(std::string()) {}; \
											classname(const std::string& msg) throw () : msg(msg){}; \
											virtual ~classname() throw () {}; \
											virtual const char* what() const throw () { return msg.c_str(); }; \
									}

#endif
