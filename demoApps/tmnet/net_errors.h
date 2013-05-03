/*
 * net_errors.h
 *
 *  Created on: 26 Jan 2012
 *      Author: denis
 */

#ifndef TMNET_ERRORS_H_
#define TMNET_ERRORS_H_

#define TMNET_OK				0x000
#define TMNET_INVALID_PARAMETER	0x001
#define TMNET_UNSUPPORTED		0x100

// plugin related
#define TMNET_PLUGIN_FAILED					0x200
#define TMNET_SCHEME_ALREADY_REGISTERED		0x201
#define TMNET_SYSTEM_ERROR					0x202

// flow related
#define TMNET_ENDOFSTREAM		0x300
#define TMNET_NETWORKERROR		0x301


#endif /* TMNET_ERRORS_H_ */
