/*
 * stat.h
 *
 *  Created on: Apr 27, 2010
 *      Author: denis
 */

#ifndef STAT_H_
#define STAT_H_

#include "debug.h"

#include <vector>
#include <map>
#include <string>

/**
 * @brief Collect various statistics
 */
class Statistics
{
public:
	class Latency
	{
	public:
		std::vector<double> values;
		std::vector<double> capacities;

		Latency() {};
		virtual ~Latency() {};

		virtual void update(double val, std::size_t pktSize)
		{
			values.push_back(val);
			capacities.push_back(((double) pktSize) * val);
		}

		virtual void finalize()
		{
			assert(values.size() > 0);
			std::sort(values.begin(), values.end());
		}

		virtual double min() const
		{
			assert(values.size() > 0);
			return values[0];
		}

		virtual double max() const
		{
			assert(values.size() > 0);
			return values[values.size()-1];
		}

		virtual double median() const
		{
			assert(values.size() > 0);
			if (values.size() % 2) {
				// odd
				return values[values.size()/2];

			} else {
				// even
				return (values[values.size()/2] + values[values.size()/2 + 1]) / 2;

			}
		}

		virtual double jitter() const
		{
			return max() - min();
		}

		virtual double capacity() const
		{
			double cap = 0;
			std::vector<double>::const_iterator it;
			for (it = capacities.begin(); it != capacities.end(); it++) {
				cap += *it;
			}
			return cap / capacities.size();
		}
	};

	class Entry
	{
	public:
		Latency latency;

		Entry(Latency lat = Latency())
			: latency(lat)
		{};
	};

public:
	std::map<std::string, Entry> entries;

	Statistics() {};
	virtual ~Statistics()
	{
		print();
	};

	virtual void print()
	{
		std::map<std::string, Entry>::iterator it;
		for (it = entries.begin(); it != entries.end(); it++) {
			it->second.latency.finalize();
			DBG_INFO(FMT("[STAT] [LATENCY] [%1%] [MIN] %2%") % it->first % it->second.latency.min());
			DBG_INFO(FMT("[STAT] [LATENCY] [%1%] [MAX] %2%") % it->first % it->second.latency.max());
			DBG_INFO(FMT("[STAT] [LATENCY] [%1%] [MEDIAN] %2%") % it->first % it->second.latency.median());
			DBG_INFO(FMT("[STAT] [LATENCY] [%1%] [JITTER] %2%") % it->first % it->second.latency.jitter());
			DBG_INFO(FMT("[STAT] [LATENCY] [%1%] [CAPACITY] %2%") % it->first % it->second.latency.capacity());
		}
	}

};

#endif /* STAT_H_ */
