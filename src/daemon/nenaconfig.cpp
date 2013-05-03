/** @file
 * nenaconfig.cpp
 *
 * @brief NENA Configuration Module implementation
 *
 * (c) 2008-2012 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Jan 01, 2012
 *      Author: Benjamin Behringer
 */

#include "nenaconfig.h"
#include "xmlfilehandling.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include "pugixml.h"

using namespace pugi;
using namespace std;
using namespace boost::filesystem;
using boost::shared_ptr;
using boost::format;
using boost::lexical_cast;
using boost::shared_lock;
using boost::shared_mutex;
using boost::unique_lock;
using boost::upgrade_lock;
using boost::upgrade_to_unique_lock;

class NenaConfig::Impl
{
private:
	NenaConfig * q;
public:
	/**
	 * @var ConfigFile configFile
	 *
	 * @brief config file for this node
	 */
	ConfigFile configFile;

	/**
	 * @var defaultsFile
	 *
	 * @brief contains default values
	 */
	DefaultsFile defaultsFile;

	/// temporary, to be removed/renamed
	const string configId;

	/// how not spec'ed parameters are treated
	NenaConfig::SPEC_DEPENDENCY specDependency;

	/// wa little bit of transaction security for every access to the config file
	mutable shared_mutex transactionMutex;

	/**
	 * @fn inline string reducePath (string componentId) const;
	 *
	 * @brief strip last part of componentId
	 *
	 * @return reduced id
	 */
	inline string reducePath (string componentId) const;

	/**
	 * @var std::map<string, boost::shared_ptr<SpecFile> > specMap
	 *
	 * @brief map with all components and the spec files that has their default values
	 */
	map<string, shared_ptr<SpecFile> > specMap;

	/**
	 * @fn void loadSpecs (boost::filesystem::path specDirPath)
	 *
	 * @brief search all spec files for default values of components and fill the specMap
	 */
	void loadSpecs (boost::filesystem::path specDirPath);

	/**
	 * @fn bool internalParameter (string componentId, string paramName) const;
	 *
	 * @brief check if the parameter is NenaConfig config internal
	 *
	 * @param componentId			id of the calling component
	 * @param paramName				name of the parameter
	 *
	 * @return true if the parameter is internal, false otherwise
	 */
	bool internalParameter (string componentId, string paramName) const;

	Impl (NenaConfig * _q, string nodename, string configFileName, string defaultsFileName, string configId);
	virtual ~Impl () {}

	template<typename T>
	void getParameter (const string & componentId, const string & paramName, T & val, const XMLFile::PARAMETER_DATATYPE & datatype) const
	{
		shared_lock<shared_mutex> readLock(transactionMutex);
		map<string, shared_ptr<SpecFile> >::const_iterator it = specMap.find(componentId);
		if (it == specMap.end ()) it = specMap.find(reducePath(componentId));

		if (configFile.hasParameter(componentId, paramName, datatype))
		{
			/// only check if strict dependencies are enabled, and the parameters are not internal
			if (specDependency == STRICT && !internalParameter(componentId, paramName))
			{
				if (it == specMap.end())
					throw EUnknown((format("NenaConfig::getParameter: No parameter '%1%' for component '%2%' in spec found.") % paramName % componentId).str());
				if (!it->second->checkParameter(componentId, paramName, val))
					throw EConfig((format("NenaConfig::getParameter: Variable '%1%' for component '%2%' does not fit the spec.") % paramName % componentId).str());
			}

			configFile.getParameter (componentId, paramName, val);
		}
		else if (defaultsFile.hasParameter(componentId, paramName, datatype))
		{
			/// only check if strict dependencies are enabled, and the parameters are not internal
			if (specDependency == STRICT && !internalParameter(componentId, paramName))
			{
				if (it == specMap.end())
					throw EUnknown((format("NenaConfig::getParameter: No parameter '%1%' for component '%2%' in spec found.") % paramName % componentId).str());
				if (!it->second->checkParameter(componentId, paramName, val))
					throw EConfig((format("NenaConfig::getParameter: Variable '%1%' for component '%2%' does not fit the spec.") % paramName % componentId).str());
			}

			defaultsFile.getParameter (componentId, paramName, val);
		}
		else if (it != specMap.end())
			it->second->getParameter(componentId, paramName, val);
		else
			throw EUnknown((format("NenaConfig::getParameter: No parameter '%1%' for component '%2%' found.") % paramName % componentId).str());

		return;
	}

	template<typename T>
	void getParameterList (const string & componentId, const string & paramName, vector<T> & vec, const XMLFile::PARAMETER_DATATYPE & datatype) const
	{
		shared_lock<shared_mutex> readLock(transactionMutex);
		map<string, shared_ptr<SpecFile> >::const_iterator it = specMap.find(componentId);
		if (it == specMap.end ()) it = specMap.find(reducePath(componentId));

		if (configFile.hasParameter(componentId, paramName, datatype, XMLFile::LIST))
		{
			/// only check if strict dependencies are enabled, and the parameters are not internal
			if (specDependency == STRICT && !internalParameter(componentId, paramName))
			{
				if (it == specMap.end())
					throw EUnknown((format("NenaConfig::getParameterList: No list '%1%' for component '%2%' in spec found.") % paramName % componentId).str());
				if (!it->second->checkParameterList(componentId, paramName, vec))
					throw EConfig((format("NenaConfig::getParameterList: Variable '%1%' for component '%2%' does not fit the spec.") % paramName % componentId).str());
			}

			configFile.getParameterList (componentId, paramName, vec);
		}
		else if (defaultsFile.hasParameter(componentId, paramName, datatype, XMLFile::LIST))
		{
			/// only check if strict dependencies are enabled, and the parameters are not internal
			if (specDependency == STRICT && !internalParameter(componentId, paramName))
			{
				if (it == specMap.end())
					throw EUnknown((format("NenaConfig::getParameterList: No list '%1%' for component '%2%' in spec found.") % paramName % componentId).str());
				if (!it->second->checkParameterList(componentId, paramName, vec))
					throw EConfig((format("NenaConfig::getParameterList: Variable '%1%' for component '%2%' does not fit the spec.") % paramName % componentId).str());
			}

			defaultsFile.getParameterList (componentId, paramName, vec);
		}
		else if (it != specMap.end())
			it->second->getParameterList(componentId, paramName, vec);
		else
			throw EUnknown((format("NenaConfig::getParameterList: No parameter '%1%' for component '%2%' found.") % paramName % componentId).str());

		return;
	}

	template<typename T>
	void getParameterTable (const string & componentId, const string & paramName, vector<vector<T> > & table, const XMLFile::PARAMETER_DATATYPE & datatype) const
	{
		shared_lock<shared_mutex> readLock(transactionMutex);
		map<string, shared_ptr<SpecFile> >::const_iterator it = specMap.find(componentId);
		if (it == specMap.end ()) it = specMap.find(reducePath(componentId));

		if (configFile.hasParameter(componentId, paramName, datatype, XMLFile::TABLE))
		{
			/// only check if strict dependencies are enabled, and the parameters are not internal
			if (specDependency == STRICT && !internalParameter(componentId, paramName))
			{
				if (it == specMap.end())
					throw EUnknown((format("NenaConfig::getParameterTable: No table '%1%' for component '%2%' in spec found.") % paramName % componentId).str());
				if (!it->second->checkParameterTable(componentId, paramName, table))
					throw EConfig((format("NenaConfig::getParameterTable: Variable '%1%' for component '%2%' does not fit the spec.") % paramName % componentId).str());
			}

			configFile.getParameterTable (componentId, paramName, table);
		}
		else if (defaultsFile.hasParameter(componentId, paramName, datatype, XMLFile::TABLE))
		{
			/// only check if strict dependencies are enabled, and the parameters are not internal
			if (specDependency == STRICT && !internalParameter(componentId, paramName))
			{
				if (it == specMap.end())
					throw EUnknown((format("NenaConfig::getParameterTable: No table '%1%' for component '%2%' in spec found.") % paramName % componentId).str());
				if (!it->second->checkParameterTable(componentId, paramName, table))
					throw EConfig((format("NenaConfig::getParameterTable: Variable '%1%' for component '%2%' does not fit the spec.") % paramName % componentId).str());
			}

			defaultsFile.getParameterTable (componentId, paramName, table);
		}
		else if (it != specMap.end())
			it->second->getParameterTable (componentId, paramName, table);
		else
			throw EUnknown((format("NenaConfig::getParameterTable: No parameter '%1%' for component '%2%' found.") % paramName % componentId).str());

		return;
	}

	template<typename T>
	void setParameter (const string & componentId, const string & paramName, const T & val, const XMLFile::PARAMETER_DATATYPE & datatype)
	{
		upgrade_lock<shared_mutex> readBeforeWriteLock(transactionMutex);
		/// only check if strict dependencies are enabled, and the parameters are not internal
		if (specDependency == STRICT && !internalParameter(componentId, paramName))
		{
			map<string, shared_ptr<SpecFile> >::const_iterator it = specMap.find(componentId);

			if (it == specMap.end())
				throw EAccess ((format("NenaConfig::setParameter: Component '%1%' unknown.") % componentId).str());
			if (!it->second->hasParameter(componentId, paramName, datatype))
				throw EAccess ((format("NenaConfig::setParameter: Parameter '%1%' of component '%2%' unknown.") % paramName % componentId).str());

			if (!it->second->checkParameter(componentId, paramName, val))
				throw EAccess ((format("NenaConfig::setParameter: Value '%1%' for parameter '%2%' of component '%3%' not permitted.") % val % paramName % componentId).str());
		}

		if (!configFile.hasParameter(componentId, paramName, datatype))
		{
			upgrade_to_unique_lock<shared_mutex> writeAfterReadLock(readBeforeWriteLock);
			configFile.addParameter(componentId, paramName, datatype, XMLFile::VALUE);
		}

		upgrade_to_unique_lock<shared_mutex> writeAfterReadLock(readBeforeWriteLock);
		configFile.setParameter(componentId, paramName, val);

		return;
	}

	template<typename T>
	void setParameterList (const string & componentId, const string & paramName, const vector<T> & vec, const XMLFile::PARAMETER_DATATYPE & datatype)
	{
		upgrade_lock<shared_mutex> readBeforeWriteLock(transactionMutex);
		/// only check if strict dependencies are enabled, and the parameters are not internal
		if (specDependency == STRICT && !internalParameter(componentId, paramName))
		{
			map<string, shared_ptr<SpecFile> >::const_iterator it = specMap.find(componentId);

			if (it == specMap.end())
				throw EAccess ((format("NenaConfig::setParameterList: Component '%1%' unknown.") % componentId).str());
			if (!it->second->hasParameter(componentId, paramName, datatype, XMLFile::LIST))
				throw EAccess ((format("NenaConfig::setParameterList: Parameter '%1%' of component '%2%' unknown.") % paramName % componentId).str());

			if (!it->second->checkParameterList(componentId, paramName, vec))
				throw EAccess ((format("NenaConfig::setParameterList: Value for parameter list '%1%' of component '%2%' not permitted.") % paramName % componentId).str());
		}

		if (!configFile.hasParameter(componentId, paramName, datatype, XMLFile::LIST))
		{
			upgrade_to_unique_lock<shared_mutex> writeAfterReadLock(readBeforeWriteLock);
			configFile.addParameter(componentId, paramName, datatype, XMLFile::LIST);
		}

		upgrade_to_unique_lock<shared_mutex> writeAfterReadLock(readBeforeWriteLock);
		configFile.setParameterList(componentId, paramName, vec);

		return;
	}

	template<typename T>
	void setParameterTable (const string & componentId, const string & paramName, const vector<vector<T> > & table, const XMLFile::PARAMETER_DATATYPE & datatype)
	{
		upgrade_lock<shared_mutex> readBeforeWriteLock(transactionMutex);
		/// only check if strict dependencies are enabled, and the parameters are not internal
		if (specDependency == STRICT && !internalParameter(componentId, paramName))
		{
			map<string, shared_ptr<SpecFile> >::const_iterator it = specMap.find(componentId);

			if (it == specMap.end())
				throw EAccess ((format("NenaConfig::setParameterTable: Component '%1%' unknown.") % componentId).str());
			if (!it->second->hasParameter(componentId, paramName, datatype, XMLFile::TABLE))
				throw EAccess ((format("NenaConfig::setParameterTable: Parameter '%1%' of component '%2%' unknown.") % paramName % componentId).str());

			if (!it->second->checkParameterTable(componentId, paramName, table))
				throw EAccess ((format("NenaConfig::setParameterTable: Value for parameter table '%1%' of component '%2%' not permitted.") % paramName % componentId).str());
		}

		if (!configFile.hasParameter(componentId, paramName, datatype, XMLFile::TABLE))
		{
			upgrade_to_unique_lock<shared_mutex> writeAfterReadLock(readBeforeWriteLock);
			configFile.addParameter(componentId, paramName, datatype, XMLFile::TABLE);
		}

		upgrade_to_unique_lock<shared_mutex> writeAfterReadLock(readBeforeWriteLock);
		configFile.setParameterTable(componentId, paramName, table);

		return;
	}
};

NenaConfig::Impl::Impl (NenaConfig * _q, string nodename, string configFileName, string defaultsFileName, string configId):
		q (_q),
		configFile (configFileName, nodename),
		defaultsFile(defaultsFileName),
		configId(configId),
		specDependency(NenaConfig::STRICT)
{}

string NenaConfig::Impl::reducePath (string componentId) const
{
	size_t n = componentId.rfind('/');

	if (n != string::npos)
		return componentId.substr(0, n+1);
	else
		return "";
}

void NenaConfig::Impl::loadSpecs (path specDirPath)
{
	directory_iterator end;

	if (!exists (specDirPath))
	{
		throw EConfig((format("NenaConfig::loadSpecs: Path %1% does not exist.") % specDirPath.filename()).str());
		return;
	}

	for (directory_iterator it(specDirPath); it != end; it++)
	{
		if (is_directory (it->status ()))
		{
			if (it->path ().filename ().native()[0] != '.')
			{
				loadSpecs (it->path ());
			}
		}
		else
		{
			shared_ptr<SpecFile> tmp(new SpecFile(it->path().string()));
			set<string> componentSet = tmp->getComponentIds ();

			for (set<string>::const_iterator lit = componentSet.begin(); lit != componentSet.end(); lit++)
			{
				if (specMap.find(*lit) != specMap.end())
					throw EConfig((format("NenaConfig::loadSpecs: Two spec files for component %1% found.") % *lit).str());

				specMap[*lit] = tmp;
			}

		}
	}
}

bool NenaConfig::Impl::internalParameter (string componentId, string paramName) const
{
	if (componentId != configId)
		return false;

	if (paramName != "specpath" && paramName != "spec_dependency" && paramName != "save_config")
		return false;

	return true;
}

/* ************************************************************************* */

NenaConfig::NenaConfig (std::string nodename, string configFileName, string defaultsFileName, string configId):
	d(new Impl(this, nodename, configFileName, defaultsFileName, configId))
{
	/// get the spec directory, don't check with the specs because we don't know them yet
	string specDirName;
	string specdep = "strict";
	if (hasParameter(d_func ()->configId, "specpath"))
		getParameter (d_func ()->configId, "specpath", specDirName);

	if (!specDirName.empty())
	{
		path specDirPath (specDirName);
		d_func ()->loadSpecs (specDirPath);
	}

	/// Beware: no check is performed whether a second nena instance might do the same to the file
	if (hasParameter(d_func ()->configId, "save_config", XMLFile::BOOL))
		getParameter(d_func ()->configId, "save_config", d_func()->configFile.fSave);

	if (hasParameter(d_func ()->configId, "spec_dependency"))
		getParameter (d_func ()->configId, "spec_dependency", specdep);

	if (specdep == "permissive")
		d_func ()->specDependency = PERMISSIVE;
	else if (specdep != "strict")
		throw EConfig((format("NenaConfig::NenaConfig: Spec file dependency type '%1%' unknown.") % specdep).str());
}

NenaConfig::~NenaConfig ()
{}

NenaConfig::Impl * NenaConfig::d_func ()
{
	return d.get ();
}
const NenaConfig::Impl * NenaConfig::d_func() const
{
	return d.get();
}

void NenaConfig::getParameter (const string & componentId, const string & paramName, string & val) const
{
	d_func ()->getParameter(componentId, paramName, val, XMLFile::STRING);
	return;
}

void NenaConfig::getParameter (const string & componentId, const string & paramName, bool & val) const
{
	d_func ()->getParameter(componentId, paramName, val, XMLFile::BOOL);
	return;
}

void NenaConfig::getParameter (const string & componentId, const string & paramName, uint32_t & val) const
{
	d_func ()->getParameter(componentId, paramName, val, XMLFile::UINT32_T);
	return;
}

void NenaConfig::getParameter (const string & componentId, const string & paramName, float & val) const
{
	d_func ()->getParameter(componentId, paramName, val, XMLFile::FLOAT);
	return;
}

void NenaConfig::getParameterList (const string & componentId, const string & paramName, vector<string> & list) const
{
	d_func ()->getParameterList(componentId, paramName, list, XMLFile::STRING);
	return;
}

void NenaConfig::getParameterList (const string & componentId, const string & paramName, vector<bool> & list) const
{
	d_func ()->getParameterList(componentId, paramName, list, XMLFile::BOOL);
	return;
}

void NenaConfig::getParameterList (const string & componentId, const string & paramName, vector<uint32_t> & list) const
{
	d_func ()->getParameterList(componentId, paramName, list, XMLFile::UINT32_T);
	return;
}

void NenaConfig::getParameterList (const string & componentId, const string & paramName, vector<float> & list) const
{
	d_func ()->getParameterList(componentId, paramName, list, XMLFile::FLOAT);
	return;
}

void NenaConfig::getParameterTable (const string & componentId, const string & paramName, vector<vector<string> > & table) const
{
	d_func ()->getParameterTable (componentId, paramName, table, XMLFile::STRING);
	return;
}

void NenaConfig::getParameterTable (const string & componentId, const string & paramName, vector<vector<bool> > & table) const
{
	d_func ()->getParameterTable (componentId, paramName, table, XMLFile::BOOL);
	return;
}

void NenaConfig::getParameterTable (const string & componentId, const string & paramName, vector<vector<uint32_t> > & table) const
{
	d_func ()->getParameterTable (componentId, paramName, table, XMLFile::UINT32_T);
	return;
}

void NenaConfig::getParameterTable (const string & componentId, const string & paramName, vector<vector<float> > & table) const
{
	d_func ()->getParameterTable (componentId, paramName, table, XMLFile::FLOAT);
	return;
}

void NenaConfig::setParameter (const string & componentId, const string & paramName, const string & val)
{
	d_func ()->setParameter(componentId, paramName, val, XMLFile::STRING);
	return;
}

void NenaConfig::setParameter (const string & componentId, const string & paramName, const bool & val)
{
	d_func ()->setParameter(componentId, paramName, val, XMLFile::BOOL);
	return;
}

void NenaConfig::setParameter (const string & componentId, const string & paramName, const uint32_t & val)
{
	d_func ()->setParameter(componentId, paramName, val, XMLFile::UINT32_T);
	return;
}

void NenaConfig::setParameter (const string & componentId, const string & paramName, const float & val)
{
	d_func ()->setParameter(componentId, paramName, val, XMLFile::FLOAT);
	return;
}

void NenaConfig::setParameterList (const string & componentId, const string & paramName, const vector<string> & vec)
{
	d_func ()->setParameterList(componentId, paramName, vec, XMLFile::STRING);
	return;
}

void NenaConfig::setParameterList (const string & componentId, const string & paramName, const vector<bool> & vec)
{
	d_func ()->setParameterList(componentId, paramName, vec, XMLFile::BOOL);
	return;
}

void NenaConfig::setParameterList (const string & componentId, const string & paramName, const vector<uint32_t> & vec)
{
	d_func ()->setParameterList(componentId, paramName, vec, XMLFile::UINT32_T);
	return;
}

void NenaConfig::setParameterList (const string & componentId, const string & paramName, const vector<float> & vec)
{
	d_func ()->setParameterList(componentId, paramName, vec, XMLFile::FLOAT);
	return;
}

void NenaConfig::setParameterTable (const string & componentId, const string & paramName, const vector<vector<string> > & table)
{
	d_func ()->setParameterTable(componentId, paramName, table, XMLFile::STRING);
	return;
}

void NenaConfig::setParameterTable (const string & componentId, const string & paramName, const vector<vector<bool> > & table)
{
	d_func ()->setParameterTable(componentId, paramName, table, XMLFile::BOOL);
	return;
}

void NenaConfig::setParameterTable (const string & componentId, const string & paramName, const vector<vector<uint32_t> > & table)
{
	d_func ()->setParameterTable(componentId, paramName, table, XMLFile::UINT32_T);
	return;
}

void NenaConfig::setParameterTable (const string & componentId, const string & paramName, const vector<vector<float> > & table)
{
	d_func ()->setParameterTable(componentId, paramName, table, XMLFile::FLOAT);
	return;
}

bool NenaConfig::hasParameter (const string & componentId, const string & paramName, const XMLFile::PARAMETER_DATATYPE & datatype, const XMLFile::PARAMETER_TYPE & type)
{
	shared_lock<shared_mutex> readLock(d_func()->transactionMutex);

	if (d_func()->configFile.hasParameter(componentId, paramName, datatype, type))
		return true;

	if (d_func()->defaultsFile.hasParameter(componentId, paramName, datatype, type))
		return true;

	map<string, shared_ptr<SpecFile> >::const_iterator it = d_func()->specMap.find(componentId);

	if (it != d_func()->specMap.end() && it->second->hasParameter(componentId, paramName, datatype, type))
		return true;

	return false;
}

set<string> NenaConfig::getComponentIds () const
{
	shared_lock<shared_mutex> readLock(d_func()->transactionMutex);
	set<string> idSet;

	for (map<string, shared_ptr<SpecFile> >::const_iterator it = d_func()->specMap.begin(); it != d_func()->specMap.end(); it++)
	{
		set<string> tmp = it->second->getComponentIds();
		idSet.insert(tmp.begin (), tmp.end ());
	}

	if (d_func()->specDependency == PERMISSIVE)
	{
		set<string> tmp = d_func()->configFile.getComponentIds();
		idSet.insert(tmp.begin (), tmp.end ());
	}

	return idSet;
}

set<string> NenaConfig::getComponentParameters (const string & componentId) const
{
	shared_lock<shared_mutex> readLock(d_func()->transactionMutex);
	set<string> parameterSet;

	for (map<string, shared_ptr<SpecFile> >::const_iterator it = d_func()->specMap.begin(); it != d_func()->specMap.end(); it++)
	{
		set<string> tmp = it->second->getComponentParameters(componentId);
		parameterSet.insert(tmp.begin (), tmp.end ());
	}

	if (d_func()->specDependency == PERMISSIVE)
	{
		set<string> tmp = d_func()->configFile.getComponentIds();
		parameterSet.insert(tmp.begin (), tmp.end ());
	}

	return parameterSet;
}

void NenaConfig::resetToDefault (const string & componentId, const string & paramName)
{
	unique_lock<shared_mutex> readBeforeWriteLock(d_func ()->transactionMutex);
	d_func ()->configFile.delParameter(componentId, paramName);
}

void NenaConfig::resetToDefault (const string & componentId)
{
	unique_lock<shared_mutex> writeLock(d_func ()->transactionMutex);
	d_func ()->configFile.delComponentConfig(componentId);
}

void NenaConfig::setSpecDependency (const SPEC_DEPENDENCY & dep)
{
	unique_lock<shared_mutex> writeLock(d_func ()->transactionMutex);
	switch (dep)
	{
	case STRICT:
		if (d_func ()->specDependency != STRICT)
		{
			d_func ()->configFile.setParameter(d_func()->configId, "spec_dependency", string("strict"));
			d_func ()->specDependency = STRICT;
		}
		break;
	case PERMISSIVE:
		if (d_func ()->specDependency != PERMISSIVE)
		{
			d_func ()->configFile.setParameter(d_func()->configId, "spec_dependency", string("permissive"));
			d_func ()->specDependency = PERMISSIVE;
		}
		break;
	default:
		throw EAccess ((format("NenaConfig::setSpecDependency: Unknown option.")).str());
	}
}

NenaConfig::SPEC_DEPENDENCY NenaConfig::getSpecDependency () const
{
	shared_lock<shared_mutex> readLock(d_func()->transactionMutex);
	return d_func ()->specDependency;
}
