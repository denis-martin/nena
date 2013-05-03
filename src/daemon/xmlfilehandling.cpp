/** @file
 * xmlfilehandling.cpp
 *
 * @brief XML file handling for NENA Configuration Module implementation
 *
 * (c) 2008-2012 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Jan 01, 2012
 *      Author: Benjamin Behringer
 */

#include "xmlfilehandling.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <pugixml.h>

using namespace pugi;
using namespace std;
using boost::format;
using boost::lexical_cast;

string DATATYPE_STRINGS[] = { "bool", "uint32_t", "float", "string" };
string TYPE_STRINGS[] 	  = { "value", "list", "table" };

template<typename T, size_t N>
inline
size_t array_size (const T (&lhs)[N])
{
	return N;
}

class XMLFile::BasicXML
{
public:
	/// back link
	XMLFile * q;

	/// filename of the xml file used
	string filename;

	/**
	 * @var pugi::xml_document configFile;
	 *
	 * @brief the xml file root pointer
	 */
	xml_document xmlFile;

	/**
	 * @var pugi::xml_node componentsRoot;
	 *
	 * @brief pointer to the root of the xml subtree that contains the various components sections
	 */
	xml_node componentsRoot;

	explicit BasicXML(const string & fn);
	virtual ~BasicXML () {}

	XMLFile * q_func ();
	const XMLFile * q_func () const;

	/**
	 * @fn string parseComponentScheme (const string & componentId) const;
	 *
	 * @brief extract component scheme from componentId
	 *
	 * @param componentId		id of component
	 * @return					component scheme
	 */
	inline string parseComponentScheme (const string & componentId) const;

	/**
	 * @fn inline string reducePath (string componentId) const;
	 *
	 * @brief strip last part of componentId
	 *
	 * @return reduced id
	 */
	inline string reducePath (string componentId) const;

	/**
	 * @fn pugi::xml_node locateParamNode (const std::string & componentId, const std::string & paramName) const;
	 *
	 * @brief find a general or specific parameter xml node in the config file
	 */
	virtual xml_node locateParamNode (const string & componentId, const string & paramName) const;

	/**
	 * @fn virtual xml_node componentSearch (const xml_node components, const string & componentId) const;
	 *
	 * @brief localte the component node based on componentId
	 *
	 * @return the matchingcomponent node
	 */
	virtual xml_node componentSearch (const xml_node components, const string & componentId) const;

	template<typename T>
	void getParameter (const string & componentId, const string & paramName, T & val, const PARAMETER_DATATYPE & datatype = STRING) const
	{
		if (!q_func()->hasParameter (componentId, paramName, datatype))
			throw EUnknown ((format("XMLFile::BasicXML::getParameter: Value parameter '%1%' of '%2%' with datatype '%3%' unknown.")
							% paramName % componentId % q_func()->convertDatatype(datatype) ).str());

		xml_node param = locateParamNode (componentId, paramName);
		string tmp = param.child("value").child_value();
		val = lexical_cast<T> (tmp);
		return;
	}

	template<typename T>
	void getParameterList (const string & componentId, const string & paramName, vector<T> & vec, const PARAMETER_DATATYPE & datatype = STRING) const
	{
		if (!q_func()->hasParameter (componentId, paramName, datatype, LIST))
			throw EUnknown ((format("XMLFile::BasicXML::getParameterList: List parameter '%1%' of '%2%' with datatype '%3%' unknown.")
							% paramName % componentId % q_func()->convertDatatype(datatype) ).str());

		xml_node param = locateParamNode (componentId, paramName);
		vec.clear();

		xml_node it = param.child("value").child("li");

		while (it) {
			vec.push_back(lexical_cast<T> (it.child_value()));
			it = it.next_sibling("li");
		}
	}

	template<typename T>
	void getParameterTable (const string & componentId, const string & paramName, vector<vector<T> > & table, const PARAMETER_DATATYPE & datatype = STRING) const
	{
		if (!q_func()->hasParameter (componentId, paramName, datatype, TABLE))
			throw EUnknown ((format("XMLFile::BasicXML::getParameterTable: Table parameter '%1%' of '%2%' with datatype '%3%' unknown.")
							% paramName % componentId % q_func()->convertDatatype(datatype) ).str());

		xml_node param = locateParamNode (componentId, paramName);
		table.clear();

		bool firstLine = true;
		size_t width = 0;

		xml_node h = param.child("value").child("tr");
		while (h) {
			table.push_back(vector<T>());

			size_t i = 0;
			xml_node w = h.child("td");
			while (w) {
				table.back().push_back(lexical_cast<T> (w.child_value()));
				w = w.next_sibling("td");
				i++;
				if (firstLine) width++;
			}

			h = h.next_sibling("tr");
			if (firstLine) {
				firstLine = false;
			} else {
				if (i != width)
					throw EConfig ((format("XMLFile::BasicXML::getParameterTable: Column size (%1%) differs from first line (%2%) for parameter '%2%'.") % i % width % paramName).str());
			}
		}
	}
};

XMLFile::BasicXML::BasicXML(const string & fn):
		q(NULL),
		filename(fn)
{}

XMLFile * XMLFile::BasicXML::q_func ()
{
	return q;
}
const XMLFile * XMLFile::BasicXML::q_func () const
{
	return q;
}

string XMLFile::BasicXML::parseComponentScheme (const string & componentId) const
{
	size_t n = componentId.find("://");

	if (n != string::npos)
		return componentId.substr(0, n);
	else
		return "";

}

string XMLFile::BasicXML::reducePath (string componentId) const
{
	size_t n = componentId.rfind('/');

	if (n != string::npos)
		return componentId.substr(0, n+1);
	else
		return "";
}

xml_node XMLFile::BasicXML::locateParamNode (const string & componentId, const string & paramName) const
{
	/// first, get component class by prefix
	string componentScheme = parseComponentScheme (componentId);

	/// check if component scheme exists
	if (componentScheme.empty())
		throw EConfig ((format("XMLFile: Component scheme parse error for component: %1%") % componentId ).str());

	/// locate the class subtree
	xml_node components = componentsRoot.find_child_by_attribute("components", "type", componentScheme.c_str());

	if (!components)
		throw EConfig ((format("XMLFile: Unable to find class %1%") % componentScheme).str());

	/// then find xml parent for this netlet
	xml_node component = componentSearch (components, componentId);

	/// we could search for the variable directly, but like this we can do more error checking
	if (!component)
		throw EConfig ((format("XMLFile: Unable to find %1% configuration for %1% %2%") % componentScheme % componentId ).str());

	xml_node parameter = component.find_child_by_attribute("parameter", "name", paramName.c_str());

	/// if we find the variable, return its xml_node
	if (!parameter)
		throw EConfig ((format("XMLFile: Unable to find parameter '%1%' for %2% '%3%'") % paramName % componentScheme % componentId ).str());

	/// return the var node
	return parameter;
}

xml_node XMLFile::BasicXML::componentSearch (const xml_node components, const string & componentId) const
{
	xml_node component = components.find_child_by_attribute("component", "id", reducePath(componentId).c_str());

	/// if not found, search more specific
	if (!component)
		component = components.find_child_by_attribute("component", "id", componentId.c_str());

	return component;
}

/* ************************************************************************* */

class SpecFile::SpecXML : public XMLFile::BasicXML
{
	public:
	explicit SpecXML (string fn);
	virtual ~SpecXML () {}

	SpecFile * q_func ();
	const SpecFile * q_func () const;
};

SpecFile::SpecXML::SpecXML (string fn):
		BasicXML(fn)
{}

SpecFile * SpecFile::SpecXML::q_func ()
{
	return dynamic_cast<SpecFile *> (BasicXML::q_func ());
}
const SpecFile * SpecFile::SpecXML::q_func () const
{
	return dynamic_cast<const SpecFile *> (BasicXML::q_func ());
}

/* ************************************************************************* */

class DefaultsFile::DefXML : public XMLFile::BasicXML
{
	public:
	explicit DefXML (string fn);
	virtual ~DefXML () {}

	DefaultsFile * q_func ();
	const DefaultsFile * q_func () const;
};

DefaultsFile::DefXML::DefXML (string fn):
		BasicXML(fn)
{}

DefaultsFile * DefaultsFile::DefXML::q_func ()
{
	return dynamic_cast<DefaultsFile *> (BasicXML::q_func ());
}
const DefaultsFile * DefaultsFile::DefXML::q_func () const
{
	return dynamic_cast<const DefaultsFile *> (BasicXML::q_func ());
}

/* ************************************************************************* */

class ConfigFile::ConfigXML : public XMLFile::BasicXML
{
public:
	/**
	 * @var string nodeName
	 *
	 * @brief name of the node which configuration shall be represented by this class
	 */
	string nodeName;

	explicit ConfigXML (string fn, string nn);
	virtual ~ConfigXML();

	ConfigFile * q_func ();
	const ConfigFile * q_func () const;

	template<typename T>
	void setParameter (const string & componentId, const string & paramName, const T & val, const PARAMETER_DATATYPE & datatype = STRING)
	{
		if (!q_func()->hasParameter (componentId, paramName, datatype))
			throw EUnknown ((format("ConfigFile::ConfigXML::setParameter: %1% parameter '%2' of '%3%' with datatype '%4%' unknown.")
							% paramName % componentId % q_func()->convertDatatype(datatype) ).str());

		xml_node param = locateParamNode (componentId, paramName);
		param.child("value").first_child().set_value(lexical_cast<string>(val).c_str());
		return;
	}

	template<typename T>
	void setParameterList (const string & componentId, const string & paramName, const vector<T> & vec, const PARAMETER_DATATYPE & datatype = STRING)
	{
		if (!q_func()->hasParameter (componentId, paramName, datatype, LIST))
			throw EUnknown ((format("ConfigFile::ConfigXML::setParameterList: List parameter '%1%' of '%2%' with datatype '%3%' unknown.")
							% paramName % componentId % q_func()->convertDatatype(datatype) ).str());

		xml_node param = locateParamNode (componentId, paramName);

		param.remove_child("value");

		xml_node value = param.append_child("value");
		value.append_attribute("datatype").set_value(convertDatatype(datatype).c_str());
		value.append_attribute("type").set_value(convertType(LIST).c_str());
		value.append_attribute("length").set_value(lexical_cast<string> (vec.size()).c_str());

		for (typename vector<T>::const_iterator it = vec.begin(); it != vec.end (); it++)
			if (!value.append_child("li").append_child(node_pcdata).set_value(lexical_cast<string> (*it).c_str()))
				throw EConfig ((format("ConfigFile::ConfigXML::setParameterList: Adding list entry to parameter '%1%' failed.") % paramName).str());
	}

	template<typename T>
	void setParameterTable (const string & componentId, const string & paramName, const vector<vector<T> > & table, const PARAMETER_DATATYPE & datatype = STRING)
	{
		if (!q_func()->hasParameter (componentId, paramName, datatype, TABLE))
			throw EUnknown ((format("ConfigFile::ConfigXML::setParameterTable: Table parameter '%1%' of '%2%' with datatype '%3%' unknown.")
							% paramName % componentId % q_func()->convertDatatype(datatype) ).str());

		xml_node param = locateParamNode (componentId, paramName);

		param.remove_child("value");

		size_t width = table[0].size ();

		for (typename vector<vector<T> >::const_iterator it = table.begin (); it != table.end (); it++)
			if (it->size () != width)
				throw EAccess ((format("ConfigFile::ConfigXML::setParameterTable: diffenrent row sizes in table for parameter %1%.") % paramName).str());

		xml_node value = param.append_child("value");
		value.append_attribute("datatype").set_value(convertDatatype(datatype).c_str());
		value.append_attribute("type").set_value(convertType(TABLE).c_str());
		value.append_attribute("height").set_value(lexical_cast<string> (table.size()).c_str());
		value.append_attribute("width").set_value(lexical_cast<string> (width).c_str());

		for (typename vector<vector<T> >::const_iterator hit = table.begin (); hit != table.end (); hit++)
		{
			xml_node tr = value.append_child("tr");

			for (typename vector<T>::const_iterator wit = hit->begin (); wit != hit->end (); wit++)
			{
				if (!tr.append_child ("td").append_child(node_pcdata).set_value(lexical_cast<string> (*wit).c_str()))
					throw EConfig ((format("ConfigFile::ConfigXML::setParameterTable: Adding table entry to parameter '%1%' failed.") % paramName).str());
			}
		}
	}

	/**
	 * @fn virtual xml_node componentSearch (const xml_node components, const string & componentId) const;
	 *
	 * @brief localte the component node based on componentId
	 *
	 * @return the matchingcomponent node
	 */
	virtual xml_node componentSearch (const xml_node components, const string & componentId) const;
};

ConfigFile::ConfigXML::ConfigXML (string fn, string nn):
		BasicXML(fn),
		nodeName(nn)
{}

ConfigFile::ConfigXML::~ConfigXML ()
{}

ConfigFile * ConfigFile::ConfigXML::q_func ()
{
	return dynamic_cast<ConfigFile *> (BasicXML::q_func());
}

const ConfigFile * ConfigFile::ConfigXML::q_func () const
{
	return dynamic_cast<const ConfigFile *> (BasicXML::q_func());
}

xml_node ConfigFile::ConfigXML::componentSearch (const xml_node components, const string & componentId) const
{
	return components.find_child_by_attribute("component", "id", componentId.c_str());
}

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

XMLFile::XMLFile (const string & fn):
		d(new BasicXML(fn))
{
	d_func ()->q = this;

	xml_parse_result result;

	/// load the xml config file
	if (!(result = d_func ()->xmlFile.load_file(d_func ()->filename.c_str())))
		throw EFile (result.description());
}

XMLFile::XMLFile (BasicXML * d):
		d(d)
{
	d_func ()->q = this;

	xml_parse_result result;

	/// load the xml config file
	if (!(result = d_func ()->xmlFile.load_file(d_func ()->filename.c_str())))
		throw EFile (result.description());
}


XMLFile::~XMLFile ()
{}

XMLFile::BasicXML * XMLFile::d_func ()
{
	return d.get();
}
const XMLFile::BasicXML * XMLFile::d_func () const
{
	return d.get();
}

inline string XMLFile::convertDatatype (const PARAMETER_DATATYPE & type)
{
	if (type >= array_size (DATATYPE_STRINGS))
		throw EUnknown ((format("XMLFile::BasicXML::convertDatatype: Datatype with id '%1%' unknown") % type ).str());

	return DATATYPE_STRINGS[type];
}

inline XMLFile::PARAMETER_DATATYPE XMLFile::convertDatatype (const string & type)
{
	size_t i = 0;

	for (; i < MAX_DATATYPES && type != DATATYPE_STRINGS[i]; i++);

	if (i == MAX_DATATYPES)
		throw EUnknown ((format("XMLFile::BasicXML::convertDatatype: Datatype '%1%' unknown") % type ).str());

	return static_cast<PARAMETER_DATATYPE> (i);
}

inline string XMLFile::convertType (const PARAMETER_TYPE & type)
{
	if (type >= array_size (TYPE_STRINGS))
		throw EUnknown ((format("XMLFile::BasicXML::convertType: Type with id '%1%' unknown") % type ).str());

	return TYPE_STRINGS[type];
}

inline XMLFile::PARAMETER_TYPE XMLFile::convertType (const string & type)
{
	size_t i = 0;

	for (; i < MAX_TYPES && type != TYPE_STRINGS[i]; i++);

	if (i == MAX_TYPES)
		throw EUnknown ((format("XMLFile::BasicXML::convertType: Type '%1%' unknown") % type ).str());

	return static_cast<PARAMETER_TYPE> (i);
}

bool XMLFile::hasParameter (const string & componentId, const string & paramName, const PARAMETER_DATATYPE & datatype, const PARAMETER_TYPE & type) const
{
	/// first, get component class by prefix
	string componentScheme = d_func()->parseComponentScheme (componentId);

	/// check if component class exists
	if (componentScheme.empty())
		throw EConfig ((format("XMLFile::BasicXML::hasParameter: Component class parse error for component: %1%") % componentId ).str());

	/// locate the parameter node, if it exists
	xml_node components = d_func()->componentsRoot.find_child_by_attribute("components", "type", componentScheme.c_str());
	xml_node parameter = d_func()->componentSearch(components, componentId).find_child_by_attribute("parameter", "name", paramName.c_str());

	if (!parameter)
		return false;

	string datatypeStr = parameter.child("value").attribute("datatype").value();

	if (convertDatatype (datatypeStr) != datatype)
		return false;

	string typeStr = parameter.child("value").attribute("type").value();

	if (typeStr.empty ())
	{
		if (type != VALUE)
			return false;
	}
	else if (convertType(typeStr) != type)
		return false;

	return true;
}

set<string> XMLFile::getComponentIds () const
{
	set<string> idSet;

	for (xml_node_iterator cit = d_func()->componentsRoot.begin(); cit != d_func()->componentsRoot.end (); cit++)
	{
		for (xml_node_iterator it = cit->begin(); it != cit->end (); it++)
		{
			xml_attribute id;
			if (!( id = it->attribute("id")))
				throw EConfig ((format("XMLFile::getComponentIds: Found component without id.")).str());

			idSet.insert(id.value());
		}
	}

	return idSet;
}

set<string> XMLFile::getComponentParameters (const string & componentId) const
{
	set<string> parameterSet;

	/// first, get component class by prefix
	string componentScheme = d_func()->parseComponentScheme (componentId);

	xml_node componentRoot = d_func()->componentsRoot.find_child_by_attribute("components", "type", componentScheme.c_str()).find_child_by_attribute("component", "id", componentId.c_str());

	for (xml_node_iterator cit = componentRoot.begin(); cit != componentRoot.end (); cit++)
	{
		xml_attribute name;
		if (!( name = cit->attribute("name")))
			throw EConfig ((format("XMLFile::getComponentParameters: Found component without id.")).str());

		parameterSet.insert(name.value());
	}

	return parameterSet;
}

void XMLFile::getParameter (const string & componentId, const string & paramName, string & val) const
{
	d_func ()->getParameter(componentId, paramName, val, STRING);
	return;
}

void XMLFile::getParameter (const string & componentId, const string & paramName, bool & val) const
{
	d_func ()->getParameter(componentId, paramName, val, BOOL);
	return;
}

void XMLFile::getParameter (const string & componentId, const string & paramName, uint32_t & val) const
{
	d_func ()->getParameter(componentId, paramName, val, UINT32_T);
	return;
}

void XMLFile::getParameter (const string & componentId, const string & paramName, float & val) const
{
	d_func ()->getParameter(componentId, paramName, val, FLOAT);
	return;
}

void XMLFile::getParameterList (const string & componentId, const string & paramName, vector<string> & list) const
{
	d_func ()->getParameterList(componentId, paramName, list, STRING);
}

void XMLFile::getParameterList (const string & componentId, const string & paramName, vector<bool> & list) const
{
	d_func ()->getParameterList(componentId, paramName, list, BOOL);
}

void XMLFile::getParameterList (const string & componentId, const string & paramName, vector<uint32_t> & list) const
{
	d_func ()->getParameterList(componentId, paramName, list, UINT32_T);
}

void XMLFile::getParameterList (const string & componentId, const string & paramName, vector<float> & list) const
{
	d_func ()->getParameterList(componentId, paramName, list, FLOAT);
}

void XMLFile::getParameterTable (const string & componentId, const string & paramName, vector<vector<string> > & table) const
{
	d_func ()->getParameterTable (componentId, paramName, table, STRING);
	return;
}

void XMLFile::getParameterTable (const string & componentId, const string & paramName, vector<vector<bool> > & table) const
{
	d_func ()->getParameterTable (componentId, paramName, table, BOOL);
	return;
}

void XMLFile::getParameterTable (const string & componentId, const string & paramName, vector<vector<uint32_t> > & table) const
{
	d_func ()->getParameterTable (componentId, paramName, table, UINT32_T);
	return;
}

void XMLFile::getParameterTable (const string & componentId, const string & paramName, vector<vector<float> > & table) const
{
	d_func ()->getParameterTable (componentId, paramName, table, FLOAT);
	return;
}

/* ************************************************************************* */

SpecFile::SpecFile (const string & fn):
		XMLFile (new SpecXML(fn))
{
	d_func ()->componentsRoot = d_func ()->xmlFile.child("spec");

	if (!d_func ()->componentsRoot)
		throw EConfig ("Unable to locate components in spec file.");
}

SpecFile::~SpecFile ()
{}

SpecFile::SpecXML * SpecFile::d_func ()
{
	return dynamic_cast<SpecXML *> (XMLFile::d_func());
}
const SpecFile::SpecXML * SpecFile::d_func () const
{
	return dynamic_cast<const SpecXML *> (XMLFile::d_func());
}

const XMLFile::PARAMETER_DATATYPE SpecFile::getParameterDatatype (const string & componentId, const string & paramName) const
{
	xml_node param = d_func ()->locateParamNode(componentId, paramName);

	if (!param)
		throw EUnknown ((format("SpecFile::getParameterDatatype: No parameter '%1%' found for component %2%") % paramName % componentId).str());

	xml_node value = param.child("value");

	if (!value)
		throw EConfig ((format("SpecFile::getParameterDatatype: No value found for parameter '%1%' of component %2%") % paramName % componentId).str());

	xml_attribute type = value.attribute("datatype");

	if (!type)
		throw EConfig ((format("SpecFile::getParameterDatatype: No type of value found for parameter '%1%' of component %2%") % paramName % componentId).str());

	const string ret = type.value();

	return convertDatatype (ret);
}

const XMLFile::PARAMETER_TYPE SpecFile::getParameterType (const string & componentId, const string & paramName) const
{
	xml_node param = d_func ()->locateParamNode(componentId, paramName);

	if (!param)
		throw EUnknown ((format("SpecFile::getParameterType: No parameter '%1%' found for component %2%") % paramName % componentId).str());

	xml_node value = param.child("value");

	if (!value)
		throw EConfig ((format("SpecFile::getParameterType: No value found for parameter '%1%' of component %2%") % paramName % componentId).str());

	xml_attribute type = value.attribute("type");

	if (!type)
		throw EConfig ((format("SpecFile::getParameterType: No type of value found for parameter '%1%' of component %2%") % paramName % componentId).str());

	const string ret = type.value();

	return convertType (ret);
}

bool SpecFile::checkParameter (const string & componentId, const string & paramName, const string & val) const
{
	if (!hasParameter (componentId, paramName, STRING))
		return false;
	return true;
}

bool SpecFile::checkParameter (const string & componentId, const string & paramName, const bool & val) const
{
	if (!hasParameter (componentId, paramName, BOOL))
		return false;
	return true;
}

bool SpecFile::checkParameter (const string & componentId, const string & paramName, const uint32_t & val) const
{
	if (!hasParameter (componentId, paramName, UINT32_T))
		return false;
	return true;
}

bool SpecFile::checkParameter (const string & componentId, const string & paramName, const float & val) const
{
	if (!hasParameter (componentId, paramName, FLOAT))
		return false;
	return true;
}

bool SpecFile::checkParameterList (const string & componentId, const string & paramName, const vector<string> & val) const
{
	if (!hasParameter (componentId, paramName, STRING, LIST))
		return false;
	return true;
}

bool SpecFile::checkParameterList (const string & componentId, const string & paramName, const vector<bool> & val) const
{
	if (!hasParameter (componentId, paramName, BOOL, LIST))
		return false;
	return true;
}

bool SpecFile::checkParameterList (const string & componentId, const string & paramName, const vector<uint32_t> & val) const
{
	if (!hasParameter (componentId, paramName, UINT32_T, LIST))
		return false;
	return true;
}

bool SpecFile::checkParameterList (const string & componentId, const string & paramName, const vector<float> & val) const
{
	if (!hasParameter (componentId, paramName, FLOAT, LIST))
		return false;
	return true;
}

bool SpecFile::checkParameterTable (const string & componentId, const string & paramName, const vector<vector<string> > & val) const
{
	if (!hasParameter (componentId, paramName, STRING, TABLE))
		return false;
	return true;
}

bool SpecFile::checkParameterTable (const string & componentId, const string & paramName, const vector<vector<bool> > & val) const
{
	if (!hasParameter (componentId, paramName, BOOL, TABLE))
		return false;
	return true;
}

bool SpecFile::checkParameterTable (const string & componentId, const string & paramName, const vector<vector<uint32_t> > & val) const
{
	if (!hasParameter (componentId, paramName, UINT32_T, TABLE))
		return false;
	return true;
}

bool SpecFile::checkParameterTable (const string & componentId, const string & paramName, const vector<vector<float> > & val) const
{
	if (!hasParameter (componentId, paramName, FLOAT, TABLE))
		return false;
	return true;
}

/* ************************************************************************* */

DefaultsFile::DefaultsFile (const string & fn):
		XMLFile (new DefXML(fn))
{
	d_func ()->componentsRoot = d_func ()->xmlFile.child("config").child("defaults");

	if (!d_func ()->componentsRoot)
		throw EConfig ("Unable to locate components in defaults file.");
}

DefaultsFile::~DefaultsFile ()
{}

DefaultsFile::DefXML * DefaultsFile::d_func ()
{
	return dynamic_cast<DefXML *> (XMLFile::d_func());
}
const DefaultsFile::DefXML * DefaultsFile::d_func () const
{
	return dynamic_cast<const DefXML *> (XMLFile::d_func());
}

/* ************************************************************************* */

ConfigFile::ConfigFile (const string & fn, const string & nn):
		XMLFile(new ConfigXML(fn, nn)),
		fSave(false)
{
	d_func ()->componentsRoot = d_func ()->xmlFile.child("config").child("nodes").find_child_by_attribute("node", "name", nn.c_str());

	if (!d_func ()->componentsRoot)
		throw EConfig ((format("ConfigFile: Unable to locate node %1% in config file.") % d_func ()->nodeName ).str());
}


ConfigFile::~ConfigFile ()
{
	if (fSave)
	{
		/// config files will be saved at the end of lifetime.
		d_func ()->xmlFile.save_file(d_func ()->filename.c_str());
	}
}

ConfigFile::ConfigXML * ConfigFile::d_func ()
{
	return dynamic_cast<ConfigXML *> (XMLFile::d_func());
}

const ConfigFile::ConfigXML * ConfigFile::d_func () const
{
	return dynamic_cast<const ConfigXML *> (XMLFile::d_func());
}

void ConfigFile::addParameter (const string & componentId, const string & paramName, const PARAMETER_DATATYPE & datatype, const PARAMETER_TYPE & type)
{
	/// first, get component class by prefix
	string componentScheme = d_func()->parseComponentScheme (componentId);

	/// check if component class exists
	if (componentScheme.empty())
		throw EConfig ((format("ConfigFile::addConfigVariable: Component class parse error for component: %1%")
						% componentId ).str());

	xml_node components, component, parameter;

	/// go through every subtree and check for the nodes
	if (!(components = d_func()->componentsRoot.find_child_by_attribute("components", "type", componentScheme.c_str())))
	{
		components = d_func()->componentsRoot.append_child("components");
		components.append_attribute("type") = componentScheme.c_str();
	}
	if (!(component = components.find_child_by_attribute("component", "id", componentId.c_str())))
	{
		component = components.append_child("component");
		component.append_attribute("id") = componentId.c_str();
	}
	if (!(parameter = component.find_child_by_attribute("parameter", "name", paramName.c_str())))
	{
		parameter = component.append_child("parameter");
		parameter.append_attribute("name") = paramName.c_str();
		parameter.append_child("value").append_child(node_pcdata);
		parameter.child("value").append_attribute("datatype").set_value(convertDatatype(datatype).c_str());
		parameter.child("value").append_attribute("type").set_value(convertType(type).c_str());
	}
	else
		throw EConfig ((format("ConfigFile::addConfigVariable: Variable %1% for %2% '%3%' already exists.")
						% paramName % componentScheme % componentId ).str());

	return;
}

void ConfigFile::delParameter (const string & componentId, const string & paramName)
{
	/// first, get component class by prefix
	string componentScheme = d_func()->parseComponentScheme (componentId);

	/// check if component class exists
	if (componentScheme.empty())
		return;

	xml_node components = d_func()->componentsRoot.find_child_by_attribute("components", "type", componentScheme.c_str());
	xml_node component = components.find_child_by_attribute("component", "id", componentId.c_str());
	xml_node parameter = component.find_child_by_attribute("parameter", "name", paramName.c_str());

	/// first, check if the variable exists
	if (parameter) {
		/// then remove it from the tree
		if (!component.remove_child(parameter))
			throw EConfig ((format("ConfigFile::delConfigVariable: Variable %1% for %2% '%3%' not deleted.")
							% paramName % componentScheme % componentId ).str());
	} else {
		return;

	}

	/// check if the node is empty, do this via "first_child", "empty" does not work
	if (!component.first_child())
		if (!components.remove_child(component))
			throw EConfig ((format("ConfigFile::delConfigVariable: Component %1% empty but could not be deleted.")
							% componentId ).str());

	/// and so on
	if (!components.first_child())
		if (!d_func ()->componentsRoot.remove_child(components))
			throw EConfig ((format("ConfigFile::delConfigVariable: Component class %1% empty, but could not be deleted.")
							% componentScheme).str());

	/// but do not delete the node tree

	return;
}

void ConfigFile::delComponentConfig (const string & componentId)
{
	/// first, get component class by prefix
	string componentScheme = d_func()->parseComponentScheme (componentId);

	/// check if component class exists
	if (componentScheme.empty())
		throw EConfig ((format("ConfigFile::delComponentConfig: Component class parse error for component: %1%")
						% componentId ).str());

	xml_node components = d_func()->componentsRoot.find_child_by_attribute("components", "type", componentScheme.c_str());
	xml_node component = components.find_child_by_attribute("component", "id", componentId.c_str());

	if (component)
		components.remove_child(component);

	if (!components.first_child())
		if (!d_func ()->componentsRoot.remove_child(components))
			throw EConfig ((format("ConfigFile::delConfigVariable: Component class %1% empty, but could not be deleted.")
							% componentScheme).str());
}

void ConfigFile::setParameter (const string & componentId, const string & paramName, const string & val)
{
	d_func ()->setParameter(componentId, paramName, val, STRING);
	return;
}

void ConfigFile::setParameter (const string & componentId, const string & paramName, const bool & val)
{
	d_func ()->setParameter(componentId, paramName, val, BOOL);
	return;
}

void ConfigFile::setParameter (const string & componentId, const string & paramName, const uint32_t & val)
{
	d_func ()->setParameter(componentId, paramName, val, UINT32_T);
	return;
}

void ConfigFile::setParameter (const string & componentId, const string & paramName, const float & val)
{
	d_func ()->setParameter(componentId, paramName, val, FLOAT);
	return;
}

void ConfigFile::setParameterList (const string & componentId, const string & paramName, const vector<string> & vec)
{
	d_func ()->setParameterList(componentId, paramName, vec, STRING);
	return;
}

void ConfigFile::setParameterList (const string & componentId, const string & paramName, const vector<bool> & vec)
{
	d_func ()->setParameterList(componentId, paramName, vec, BOOL);
	return;
}

void ConfigFile::setParameterList (const string & componentId, const string & paramName, const vector<uint32_t> & vec)
{
	d_func ()->setParameterList(componentId, paramName, vec, UINT32_T);
	return;
}

void ConfigFile::setParameterList (const string & componentId, const string & paramName, const vector<float> & vec)
{
	d_func ()->setParameterList(componentId, paramName, vec, FLOAT);
	return;
}

void ConfigFile::setParameterTable (const string & componentId, const string & paramName, const vector<vector<string> > & table)
{
	d_func ()->setParameterTable (componentId, paramName, table, STRING);
	return;
}

void ConfigFile::setParameterTable (const string & componentId, const string & paramName, const vector<vector<bool> > & table)
{
	d_func ()->setParameterTable (componentId, paramName, table, BOOL);
	return;
}

void ConfigFile::setParameterTable (const string & componentId, const string & paramName, const vector<vector<uint32_t> > & table)
{
	d_func ()->setParameterTable (componentId, paramName, table, UINT32_T);
	return;
}

void ConfigFile::setParameterTable (const string & componentId, const string & paramName, const vector<vector<float> > & table)
{
	d_func ()->setParameterTable (componentId, paramName, table, FLOAT);
	return;
}
