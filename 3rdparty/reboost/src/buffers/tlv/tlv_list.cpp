//-----------------------------------------------------------------------------
// Part of reboost (http://reboost.org).  Released under the
// BSD 2-clause license (http://www.opensource.org/licenses/bsd-license.php).
// Copyright 2012, Sebastian Mies <mies@reboost.org> --- All rights reserved.
//-----------------------------------------------------------------------------


#include "tlv_list.hpp"

namespace reboost {

/// stream operator
std::ostream& operator<<( std::ostream& os, const tlv_list& list ) {
	os << "[";
	for (tlv_list::const_iterator i = list.begin(); i != list.end(); i++)
		os << (i!=list.begin()?",":"") << *i;
	return os << "]";
}

}




