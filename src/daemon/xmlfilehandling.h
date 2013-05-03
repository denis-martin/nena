/** @file
 * xmlfilehandling.h
 *
 * @brief XML file handling for NENA Configuration Module
 *
 * (c) 2008-2012 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Jan 01, 2012
 *      Author: Benjamin Behringer
 */

#ifndef _XMLFILEHANDLING_H_
#define _XMLFILEHANDLING_H_

#include <exceptions.h>

#include <stdint.h>

#include <string>
#include <set>
#include <vector>

#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

/**
 * @class XMLFile
 *
 * @brief Base XML File handling for use in spec and config files
 */
class XMLFile : public boost::noncopyable
{
protected:
	class BasicXML;
	boost::scoped_ptr<BasicXML> d;

	XMLFile (BasicXML * d);

	BasicXML * d_func ();
	const BasicXML * d_func () const;

public:

	/// Config file not found
	NENA_EXCEPTION(EFile);

	/// Configuration error
	NENA_EXCEPTION(EConfig);

	/// error accessing the configuration
	NENA_EXCEPTION(EAccess);

	/// encountered unknown data
	NENA_EXCEPTION(EUnknown);

	/// types of parameters
	enum PARAMETER_TYPE { VALUE = 0, LIST, TABLE, MAX_TYPES};
	/// datatypes of parameters
	enum PARAMETER_DATATYPE { BOOL = 0, UINT32_T, FLOAT, STRING, MAX_DATATYPES };

	/**
	 * @fn inline static string convertDatatype (const PARAMETER_DATATYPE & type);
	 *
	 * @brief convert a datatype to a descriptive string
	 *
	 * @param type					the datatype
	 *
	 * @return a string describing the datatype
	 */
	inline static std::string convertDatatype (const PARAMETER_DATATYPE & type);
	inline static PARAMETER_DATATYPE convertDatatype (const std::string & type);

	/**
	 * @fn inline string convertType (const PARAMETER_TYPE & type);
	 *
	 * @brief convert a type to a descriptive string
	 *
	 * @param type					the type
	 *
	 * @return a string describing the type
	 */
	inline static std::string convertType (const PARAMETER_TYPE & type);
	inline static PARAMETER_TYPE convertType (const std::string & type);

	/**
	 * @fn XMLFile (const std::string & fn);
	 *
	 * @brief open xml file "filename"
	 *
	 * @param fn 		name of file
	 */
	XMLFile (const std::string & fn);
	virtual ~XMLFile ();

	/**
	 * @fn bool hasParameter (const std::string & componentId, const std::string & paramName, const PARAMETER_DATATYPE & datatype = STRING, const PARAMETER_TYPE & type = VALUE) const;
	 *
	 * @brief return wether or not the xml file contains a config parameter
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param datatype				datatype of the parameter
	 * @param type					type of the parameter
	 *
	 * @return true if there is a parameter for varName and componentId, false otherwise
	 */
	bool hasParameter (const std::string & componentId, const std::string & paramName, const PARAMETER_DATATYPE & datatype = STRING, const PARAMETER_TYPE & type = VALUE) const;

	/**
	 * @fn std::set<std::string> getComponentIds () const
	 *
	 * @brief return a set of all component ids this xml file has component options of
	 *
	 * @return a set of all component ids this xml file has component options of
	 */
	std::set<std::string> getComponentIds () const;

	/**
	 * @fn std::set<std::string> getComponentParameters (const std::string & componentId) const;
	 *
	 * @brief return all available parameters for the component in this xml file
	 *
	 * @param componentId			the component which parameters will be listed
	 *
	 * @return a set of parameters of the given component
	 */
	std::set<std::string> getComponentParameters (const std::string & componentId) const;

	/**
	 * @fn void getParameter (const std::string & componentId, const std::string & paramName, string & val);
	 *
	 * @brief read a parameter from the xml file and fill the reference with it
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
	 * @fn void getParameterList (const std::string & componentId, const std::string & paramName, std::vector<std::string> & vec) const;
	 *
	 * @brief return a list of parameters from this xml file
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param vec					reference to contain the list of parameters
	 */
	void getParameterList (const std::string & componentId, const std::string & paramName, std::vector<std::string> & vec) const;
	void getParameterList (const std::string & componentId, const std::string & paramName, std::vector<bool> & vec) const;
	void getParameterList (const std::string & componentId, const std::string & paramName, std::vector<uint32_t> & vec) const;
	void getParameterList (const std::string & componentId, const std::string & paramName, std::vector<float> & vec) const;

	/**
	 * @fn void getParameterTable (const std::string & componentId, const std::string & paramName, std::vector<std::vector<std::string> > & vec) const;
	 *
	 * @brief return a table of parameters from this xml file
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param table					reference to contain the table of parameters
	 */
	void getParameterTable (const std::string & componentId, const std::string & paramName, std::vector<std::vector<std::string> > & table) const;
	void getParameterTable (const std::string & componentId, const std::string & paramName, std::vector<std::vector<bool> > & table) const;
	void getParameterTable (const std::string & componentId, const std::string & paramName, std::vector<std::vector<uint32_t> > & table) const;
	void getParameterTable (const std::string & componentId, const std::string & paramName, std::vector<std::vector<float> > & table) const;

};

/**
 * @class SpecFile
 *
 * @brief Represents a spec file for one or more components
 */
class SpecFile : public XMLFile
{
protected:
	class SpecXML;

	SpecXML * d_func ();
	const SpecXML * d_func () const;
public:
	/**
	 * @fn SpecFile (const std::string & fn);
	 *
	 * @brief open xml file "filename"
	 *
	 * @param fn 		name of file
	 */
	SpecFile (const std::string & fn);

	/**
	 * @fn ~SpecFile ()
	 *
	 * @brief not save but close the spec file
	 */
	virtual ~SpecFile ();

	/**
	 * @fn bool checkParameter (const std::string & componentId, const std::string & paramName, const string & val) const;
	 *
	 * @brief check a parameter value based on the constraints of the spec file
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param val					value to check
	 *
	 * @return true if the parameter fits the specs
	 */
	bool checkParameter (const std::string & componentId, const std::string & paramName, const std::string & val) const;
	bool checkParameter (const std::string & componentId, const std::string & paramName, const bool & val) const;
	bool checkParameter (const std::string & componentId, const std::string & paramName, const uint32_t & val) const;
	bool checkParameter (const std::string & componentId, const std::string & paramName, const float & val) const;

	/**
	 * @fn bool checkParameterList (const std::string & componentId, const std::string & paramName, const std::vector<std::string> & vec) const;
	 *
	 * @brief check a parameter list based on the constraints of the spec file
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param vec					list to check
	 *
	 * @return true if the parameter fits the specs
	 */
	bool checkParameterList (const std::string & componentId, const std::string & paramName, const std::vector<std::string> & vec) const;
	bool checkParameterList (const std::string & componentId, const std::string & paramName, const std::vector<bool> & vec) const;
	bool checkParameterList (const std::string & componentId, const std::string & paramName, const std::vector<uint32_t> & vec) const;
	bool checkParameterList (const std::string & componentId, const std::string & paramName, const std::vector<float> & vec) const;

	/**
	 * @fn bool checkParameterTable (const std::string & componentId, const std::string & paramName, const std::vector<std::vector<std::string> > & table) const;
	 *
	 * @brief check a parameter table based on the constraints of the spec file
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param table					table to check
	 *
	 * @return true if the parameter fits the specs
	 */
	bool checkParameterTable (const std::string & componentId, const std::string & paramName, const std::vector<std::vector<std::string> > & table) const;
	bool checkParameterTable (const std::string & componentId, const std::string & paramName, const std::vector<std::vector<bool> > & table) const;
	bool checkParameterTable (const std::string & componentId, const std::string & paramName, const std::vector<std::vector<uint32_t> > & table) const;
	bool checkParameterTable (const std::string & componentId, const std::string & paramName, const std::vector<std::vector<float> > & table) const;

	/**
	 * fn const PARAMETER_DATATYPE getParameterDatatype (const std::string & componentId, const std::string & paramName) const
	 *
	 * @brief return the parameter datatype of a given parameter as string
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 *
	 * @return the parameter datatype of a given parameter
	 */
	const PARAMETER_DATATYPE getParameterDatatype (const std::string & componentId, const std::string & paramName) const;

	/**
	 * fn const PARAMETER_TYPE getParameterType (const std::string & componentId, const std::string & paramName) const
	 *
	 * @brief return the parameter type of a given parameter as string
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 *
	 * @return the parameter type of a given parameter
	 */
	const PARAMETER_TYPE getParameterType (const std::string & componentId, const std::string & paramName) const;
};

/**
 * @class DefaultsFile
 *
 * @brief represents a config file with default values for one or more nodes
 */
class DefaultsFile : public XMLFile
{
protected:
	class DefXML;

	DefXML * d_func ();
	const DefXML * d_func () const;
public:
	/**
	 * @fn DefaultsFile (const std::string & fn);
	 *
	 * @brief open xml file "filename"
	 *
	 * @param fn 		name of file
	 */
	DefaultsFile (const std::string & fn);

	/**
	 * @fn ~DefaultsFile ()
	 *
	 * @brief not save but close the defaults file
	 */
	virtual ~DefaultsFile ();
};

/**
 * @class ConfigFile
 *
 * @brief Represents a nena config file for one or more nodes
 */
class ConfigFile : public XMLFile
{
protected:
	class ConfigXML;

	ConfigXML * d_func ();
	const ConfigXML * d_func () const;

public:
	/**
	 * @var fSave
	 *
	 * @brief save the config file on destruction or not
	 */
	bool fSave;

	/**
	 * @fn ConfigFile (const std::string & fn, const std::string & nn);
	 *
	 * @brief open xml configuration in file "filename"
	 *
	 * @param fn 		name of file
	 * @param nn		node Name which configuration shall be used
	 */
	ConfigFile (const std::string & fn, const std::string & nn);

	/**
	 * @fn ~ConfigFile ():
	 *
	 * @brief save (if fSave == true) and close config file
	 */
	virtual ~ConfigFile ();

	/**
	 * @fn void addParameter (const string & componentId, const string & paramName, const PARAMETER_DATATYPE & datatype = STRING, const PARAMETER_TYPE & type = VALUE)
	 *
	 * @brief add a parameter to the config file, that can later be assigned and saved
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param datatype				datatype of parameter
	 * @param type 					type of parameter
	 */
	void addParameter (const std::string & componentId, const std::string & paramName, const PARAMETER_DATATYPE & datatype = STRING, const PARAMETER_TYPE & type = VALUE);

	/**
	 * @fn void delParameter (const string & componentId, const string & varName)
	 *
	 * @brief delete a parameter from the config file, deletes unnecessary trees
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 */
	void delParameter (const std::string & componentId, const std::string & paramName);

	/**
	 * @fn void delComponentConfig (const std::string & componentId)
	 *
	 * @brief delete all parameters for component componentId
	 *
	 * @param componentId			id of the calling component
	 */
	void delComponentConfig (const std::string & componentId);

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
	 * @brief return a table of parameters from this xml file
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 * @param table					reference to contain the table of parameters
	 */
	void setParameterTable (const std::string & componentId, const std::string & paramName, const std::vector<std::vector<std::string> > & table);
	void setParameterTable (const std::string & componentId, const std::string & paramName, const std::vector<std::vector<bool> > & table);
	void setParameterTable (const std::string & componentId, const std::string & paramName, const std::vector<std::vector<uint32_t> > & table);
	void setParameterTable (const std::string & componentId, const std::string & paramName, const std::vector<std::vector<float> > & table);
};

#endif
