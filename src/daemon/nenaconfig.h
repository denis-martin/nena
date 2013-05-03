/** @file
 * nenaconfig.h
 *
 * @brief NENA Configuration Module
 *
 * (c) 2008-2012 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Jan 01, 2012
 *      Author: Benjamin Behringer
 */

#ifndef _NENACONFIG_H_
#define _NENACONFIG_H_

#include "xmlfilehandling.h"

#include <exceptions.h>

#include <stdint.h>

#include <string>
#include <map>
#include <set>

// v3 was introduced in 1.44, thus we need at least boost v1.44
#define BOOST_FILESYSTEM_VERSION 3

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

/**
 * @class NenaConfig
 *
 * @brief The NenaConfig configuration frontend
 */
class NenaConfig : public boost::noncopyable
{
private:
	class Impl;
	boost::scoped_ptr<Impl> d;

	Impl * d_func ();
	const Impl * d_func() const;

public:
	/// Configuration error
	NENA_EXCEPTION(EConfig);

	/// error accessing the configuration
	NENA_EXCEPTION(EAccess);

	/// encountered unknown data
	NENA_EXCEPTION(EUnknown);

	/// describe how not spec'ed parameters are treated
	enum SPEC_DEPENDENCY {
		STRICT, 					///< don't allow parameters that do not appear in a spec file
		PERMISSIVE 					///< allow all parameters
	};

	/**
	 * @fn NenaConfig (std::string nodename, std::string configFileName);
	 *
	 * @param nodename				name of this nena node
	 * @param configFileName		filename of the config file to load
	 * @param defaultsFileName		filename of config file with default values
	 * @param configId				id of the config module to be used in the configuration file
	 */
	NenaConfig (std::string nodename, std::string configFileName = "nenaconf.xml", std::string defaultsFileName = "nenaconf.xml", std::string configId = "internalservice://nena/config");
	~NenaConfig ();

	/**
	 * @fn void getParameter (const std::string & componentId, const std::string & paramName, string & val);
	 *
	 * @brief read a parameter from the config file and fill the reference with it
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param val					reference to contain the value of the parameter
	 */
	void getParameter (const std::string & componentId, const std::string & paramName, std::string & val) const;
	void getParameter (const std::string & componentId, const std::string & paramName, bool & val) const;
	void getParameter (const std::string & componentId, const std::string & paramName, uint32_t & val) const;
	void getParameter (const std::string & componentId, const std::string & paramName, float & val) const;

	/**
	 * @fn void getParameterList (const std::string & componentId, const std::string & paramName, std::vector<std::string> & list) const;
	 *
	 * @brief return a list of parameters
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param val					reference to contain the list of parameters
	 */
	void getParameterList (const std::string & componentId, const std::string & paramName, std::vector<std::string> & list) const;
	void getParameterList (const std::string & componentId, const std::string & paramName, std::vector<bool> & list) const;
	void getParameterList (const std::string & componentId, const std::string & paramName, std::vector<uint32_t> & list) const;
	void getParameterList (const std::string & componentId, const std::string & paramName, std::vector<float> & list) const;

	/**
	 * @fn void getParameterTable (const std::string & componentId, const std::string & paramName, std::vector<std::vector<std::string> > & vec) const;
	 *
	 * @brief return a table of parameters
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param table					reference to contain the table of parameters
	 */
	void getParameterTable (const std::string & componentId, const std::string & paramName, std::vector<std::vector<std::string> > & table) const;
	void getParameterTable (const std::string & componentId, const std::string & paramName, std::vector<std::vector<bool> > & table) const;
	void getParameterTable (const std::string & componentId, const std::string & paramName, std::vector<std::vector<uint32_t> > & table) const;
	void getParameterTable (const std::string & componentId, const std::string & paramName, std::vector<std::vector<float> > & table) const;

	/**
	 * @fn setParameter (const std::string & componentId, const std::string & paramName, const std::string & val);
	 *
	 * @brief set a parameter in the configuration file to 'val'
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param val					value to set the parameter to
	 */
	void setParameter (const std::string & componentId, const std::string & paramName, const std::string & val);
	void setParameter (const std::string & componentId, const std::string & paramName, const bool & val);
	void setParameter (const std::string & componentId, const std::string & paramName, const uint32_t & val);
	void setParameter (const std::string & componentId, const std::string & paramName, const float & val);

	/**
	 * @fn setParameterList (const std::string & componentId, const std::string & paramName, const std::vector<std::string> & vec);
	 *
	 * @brief set a parameter list in the configuration file to 'vec'
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param vec					vector to set the parameter to
	 */
	void setParameterList (const std::string & componentId, const std::string & paramName, const std::vector<std::string> & vec);
	void setParameterList (const std::string & componentId, const std::string & paramName, const std::vector<bool> & vec);
	void setParameterList (const std::string & componentId, const std::string & paramName, const std::vector<uint32_t> & vec);
	void setParameterList (const std::string & componentId, const std::string & paramName, const std::vector<float> & vec);

	/**
	 * @fn void setParameterTable (const std::string & componentId, const std::string & paramName, const std::vector<std::vector<std::string> > & vec);
	 *
	 * @brief set a table of parameters
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param table					reference to contain the table of parameters
	 */
	void setParameterTable (const std::string & componentId, const std::string & paramName, const std::vector<std::vector<std::string> > & table);
	void setParameterTable (const std::string & componentId, const std::string & paramName, const std::vector<std::vector<bool> > & table);
	void setParameterTable (const std::string & componentId, const std::string & paramName, const std::vector<std::vector<uint32_t> > & table);
	void setParameterTable (const std::string & componentId, const std::string & paramName, const std::vector<std::vector<float> > & table);

	/**
	 * @fn bool hasParameter (const std::string & componentId, const std::string & paramName, const XMLFile::PARAMETER_DATATYPE & datatype = XMLFile::STRING, const XMLFile::PARAMETER_TYPE & type = XMLFile::VALUE);
	 *
	 * @brief return wether or not the is a config parameter with paramName for the component componentId
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param datatype				datatype of the parameter
	 * @param type					type of the parameter
	 *
	 * @return true if there is a variable for paramName and componentId, false otherwise
	 */
	bool hasParameter (const std::string & componentId, const std::string & paramName,
					   const XMLFile::PARAMETER_DATATYPE & datatype = XMLFile::STRING, const XMLFile::PARAMETER_TYPE & type = XMLFile::VALUE);

	/**
	 * @fn std::set<std::string> getComponentIds () const
	 *
	 * @brief return a set of all component ids available
	 *
	 * @return a set of all component ids this xml file has component options of
	 */
	std::set<std::string> getComponentIds () const;

	/**
	 * @fn std::set<std::string> getComponentParameters (const std::string & componentId) const;
	 *
	 * @brief return all available parameters for the component
	 *
	 * @param componentId			the component which parameters will be listed
	 *
	 * @return a set of parameters of the given component
	 */
	std::set<std::string> getComponentParameters (const std::string & componentId) const;

	/**
	 * @fn void resetToDefault (const std::string & componentId, const std::string & paramName)
	 *
	 * @brief reset a variable to its default value, or delete it if none exists
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the variable
	 */
	void resetToDefault (const std::string & componentId, const std::string & paramName);

	/**
	 * @fn void resetToDefault (const std::string & componentId)
	 *
	 * @brief reset all variables of a component to their default value, or delete them if none existed
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the variable
	 */
	void resetToDefault (const std::string & componentId);

	/**
	 * @fn void setSpecDependency (const SPEC_DEPENDENCY & dep);
	 *
	 * @brief set checking based on spec files
	 *
	 * @param dep					level of checking
	 */
	void setSpecDependency (const SPEC_DEPENDENCY & dep);

	/**
	 * @fn SPEC_DEPENDENCY getSpecDependency () const;
	 *
	 * @brief see how dependant the config is on spec files
	 *
	 * @return the dependency level
	 */
	SPEC_DEPENDENCY getSpecDependency () const;
};

#endif
