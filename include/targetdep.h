/*
 * targetdep.h
 *
 * Target specific dependencies/definitions
 *
 *  Created on: Jul 29, 2010
 *      Author: denis
 */

#ifndef TARGETDEP_H_
#define TARGETDEP_H_

#ifdef __ARM_EABI__

// missing in sys/types.h
typedef unsigned short		ushort;

// missing in netinet/in.h
#define INET_ADDRSTRLEN		(16)

#endif // __ARM_EABI__

#endif /* TARGETDEP_H_ */
