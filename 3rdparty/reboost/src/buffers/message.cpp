//-----------------------------------------------------------------------------
// Part of reboost (http://reboost.org).  Released under the
// BSD 2-clause license (http://www.opensource.org/licenses/bsd-license.php).
// Copyright 2012, Sebastian Mies <mies@reboost.org> --- All rights reserved.
//-----------------------------------------------------------------------------

#include "message.hpp"
#include<iostream>

namespace reboost {

struct to_stream {
	std::ostream& os;
	int i;
	inline void operator()(buffer_t buf) {
		if (i!=0) os <<",";
		os << buf;
		i++;
	}
};

std::ostream& operator<<(std::ostream& os, const message_t m) {
	struct to_stream ts = { os, 0 };
	os << "message({size=" << m.size() << ",buffers=" << (int) m.length()
			<< ",hash=" << m.hash() << "},";
	m.foreach(ts);
	os << ")";
	return os;
}

}
