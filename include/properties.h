/** @file
 * reqprop.h
 *
 *  Created on: Dec 23, 2009
 *      Author: denis
 */

#ifndef REQPROP_H_
#define REQPROP_H_

#include "morphableValue.h"

class CUtilityFunction
{
public:
	virtual double calculate(double x) = 0;
};

class CXPhaseFunction : public CUtilityFunction
{
public:
	class Coordinate
	{
	private:
		double x;
		double y;

	public:
		Coordinate(double x = 0.0, double y = 0.0)
			: x(x), y(y)
		{};

		virtual ~Coordinate() {};
	};

protected:
	Coordinate left;
	Coordinate right;

public:
	CXPhaseFunction(double x0, double y0, double x1, double y1)
		: left(x0, y0), right(x1, y1)
	{};

	virtual ~CXPhaseFunction() {};

	virtual double calculate(double x)
	{
		return 0.0;
	}
};

/**
 * @brief	A property
 */
class CProperty
{
public:
	/**
	 * @brief	Exception throw if properties with different IDs are compared.
	 */
	class EPropertyMismatch : public std::exception {};

protected:
	std::string id;
	std::string name;
	std::string description;
	CProperty* parentProperty;
	std::list<CProperty*> childrenProperties;
	CMorphableValue* value;

public:
	CProperty()
		: parentProperty(NULL), value(NULL)
	{};

	CProperty(std::string id, CMorphableValue* value, CProperty* parentProperty = NULL)
		: id(id), parentProperty(parentProperty), value(value)
	{
		if (parentProperty != NULL)
			parentProperty->addChildProperty(this);
	};

	CProperty(std::string id, std::string name, CMorphableValue* value, CProperty* parentProperty = NULL)
		: id(id), name(name), parentProperty(parentProperty), value(value)
	{
		if (parentProperty != NULL)
			parentProperty->addChildProperty(this);
	};

	virtual ~CProperty()
	{
		parentProperty = NULL;

		std::list<CProperty*>::iterator cit;
		for (cit = childrenProperties.begin(); cit != childrenProperties.end(); cit++)
		{
			delete *cit;
		}
		childrenProperties.clear();

		if (value != NULL)
			delete value;
	};

	/**
	 * @brief	Return ID of this property
	 */
    const std::string& getId() const
    {
        return id;
    }

    /**
     * @brief	Set ID of this property
     */
    void setId(std::string id)
    {
        this->id = id;
    }

	/**
	 * @brief	Return name of this property
	 */
    const std::string& getName() const
    {
        return name;
    }

    /**
     * @brief	Set name of this property
     */
    void setName(std::string name)
    {
        this->name = name;
    }

	/**
	 * @brief	Return informal description of property
	 */
    const std::string& getDescription() const
    {
        return description;
    }

    /**
     * @brief	Set informal description of property
     */
    void setDescription(std::string description)
    {
        this->description = description;
    }

    /**
     * @brief	Return parent property
     */
    CProperty* getParentProperty() const
    {
        return parentProperty;
    }

    /**
     * @brief	Set parent property
     */
    void setParentProperty(CProperty* parentProperty)
    {
        this->parentProperty = parentProperty;
    }

    /**
     * @brief	Return children properties
     */
    const std::list<CProperty*>& getChildrenProperties() const
    {
        return childrenProperties;
    }

    /**
     * @brief	Add a child property
     */
    void addChildProperty(CProperty* child)
    {
    	assert(child != NULL);
        this->childrenProperties.push_back(child);
    }

    /**
     * @brief	Return value of property
     */
    CMorphableValue *getValue() const
    {
        return value;
    }

    /**
     * @brief	Set value of property
     */
    void setValue(CMorphableValue *value)
    {
        this->value = value;
    }

    /**
     * @brief	Return a human-readable string of the property
     *
     * @param includeSubProperties	True if all sub properties should be
     * 								included
     * @param indention				String to prepend to every line produced;
     * 								will be duplicated for each hierarchy level
     */
    virtual std::string toStr(bool includeSubProperties = false, std::string indention = std::string()) const
    {
    	std::string s = indention + "(" + id + ", " + name;
    	if (value != NULL)
    		s += ", " + value->toStr();
    	s += ")";

    	if (includeSubProperties) {
    		std::list<CProperty*>::const_iterator it;
    		for (it = childrenProperties.begin(); it != childrenProperties.end(); it++)
    			s += "\n" + (*it)->toStr(true, indention + indention);
    	}

    	return s;
    }

};

/**
 * @brief	Property with an associated weight
 */
class CWeightedProperty : public CProperty
{
protected:
	double weight;

public:
	CWeightedProperty()
		: CProperty(), weight(0.0)
	{};

	CWeightedProperty(std::string id, CMorphableValue* value, double weight, CProperty* parentProperty = NULL)
		: CProperty(id, value, parentProperty), weight(weight)
	{};

	CWeightedProperty(std::string id, std::string name, CMorphableValue* value, double weight, CProperty* parentProperty = NULL)
		: CProperty(id, name, value, parentProperty), weight(weight)
	{};

	virtual ~CWeightedProperty()
	{};

	/**
	 * @brief	Return weight of the property
	 */
	const double& getWeight() const
	{
		return weight;
	}

	/**
	 * @brief	Set weight of the property
	 */
	void setWeight(double weight)
	{
		this->weight = weight;
	}

	/**
	 * @brief	Return a human-readable string of the property
	 *
	 * @param includeSubProperties	True if all sub properties should be
	 * 								included
	 * @param indention				String to prepend to every line produced;
	 * 								will be duplicated for each hierarchy level
	 */
	virtual std::string toStr(bool includeSubProperties = false, std::string indention = std::string()) const
	{
		std::string s = indention + "(" + id + ", " + name + ", ";
		if (value != NULL)
			s += value->toStr();
		else
			s += "N/A";
		s += ", " + (FMT("%1%") % weight).str() + ")";

		if (includeSubProperties) {
			std::list<CProperty*>::const_iterator it;
			for (it = childrenProperties.begin(); it != childrenProperties.end(); it++)
				s += "\n" + (*it)->toStr(true, indention + indention);
		}

		return s;
	}
};

#endif /* REQPROP_H_ */
