//-----------------------------------------------------------------------------
// Part of reboost (http://reboost.org).  Released under the
// BSD 2-clause license (http://www.opensource.org/licenses/bsd-license.php).
// Copyright 2012, Sebastian Mies <mies@reboost.org> --- All rights reserved.
//-----------------------------------------------------------------------------

#include "tlv_array.hpp"

namespace reboost {

std::ostream& operator<<( std::ostream& os, const tlv_array& arr ) {
	os << "[";
	for (size_t i=0; i<arr.size(); i++)
		os << (i!=0?",":"") << (const tlv_t)arr[i];
	return os << "]";
}

}





