//-----------------------------------------------------------------------------
// Part of reboost (http://reboost.org).  Released under the
// BSD 2-clause license (http://www.opensource.org/licenses/bsd-license.php).
// Copyright 2012, Sebastian Mies <mies@reboost.org> --- All rights reserved.
//-----------------------------------------------------------------------------

#include "shared_buffer.hpp"
#include <iostream>

namespace reboost {

using namespace std;

void shared_buffer_t::onexit() {
	if (allocated_buffers != 0)
		cerr << "shared_buffer_t: " << allocated_buffers << " leaked buffers." << endl;
}

size_t shared_buffer_t::init() {
	atexit(&shared_buffer_t::onexit);
	return 0;
}

size_t shared_buffer_t::allocated_buffers = init();

}
