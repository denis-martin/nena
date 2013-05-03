/** @file
 * managementDefs.h
 *
 * @brief Some general definitions for management.
 *
 * (c) 2008-2010 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Jul 01, 2010
 *      Author: faller
 */

#ifndef MANAGEMENT_DEFS_H_
#define MANAGEMENT_DEFS_H_

#include <map>
#include <list>
#include <string>
#include <utility>

/**
 * @brief All known tuning types
 */
typedef enum _MANAGEMENT_TUNING_TYPE
{
	MANAGEMENT_TUNING_TYPE_InvalidFirst,

	MANAGEMENT_TUNING_TYPE_ErrorCorrection,
	MANAGEMENT_TUNING_TYPE_Bandwidth,

	MANAGEMENT_TUNING_TYPE_InvalidLast
} MANAGEMENT_TUNING_TYPE;

/**
 * @brief A list of supported tuning types.
 */
typedef std::list<MANAGEMENT_TUNING_TYPE> TUNING_TYPE_LIST;
typedef TUNING_TYPE_LIST::iterator TUNING_TYPE_LIST_ITERATOR;

/**
 * @brief A map of supported tuning options by one Netlet for a specific tuning type, including the amount of supported tuning stages for that type.
 */
typedef std::map<std::string, int> EXECUTION_OPTIONS;
typedef std::pair<std::string, int> EXECUTION_PAIR;
typedef EXECUTION_OPTIONS::iterator EXECUTION_OPTIONS_ITERATOR;


#endif /* MANAGEMENT_DEFS_H_ */

