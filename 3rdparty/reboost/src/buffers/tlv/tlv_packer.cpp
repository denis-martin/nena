//-----------------------------------------------------------------------------
// Part of reboost (http://reboost.org).  Released under the
// BSD 2-clause license (http://www.opensource.org/licenses/bsd-license.php).
// Copyright 2012, Sebastian Mies <mies@reboost.org> --- All rights reserved.
//-----------------------------------------------------------------------------

#include "tlv_packer.hpp"

#include <sstream>

using namespace std;

namespace reboost {

string tlv_packer::to_string(const uint8_t* tl) {
	ostringstream os;
	os << "size=" << field_size(tl) << " ";
	os << "type=" << type(tl) << " " << "length=" << length(tl);
	return os.str();
}

bool test_packed_tl(size_t _type, size_t _length) {
	uint8_t tl[10];
	tlv_packer::set(tl, _type, _length);
	cout << "Testing - " << tlv_packer::to_string(tl) << ": ";
	bool suc = (tlv_packer::type(tl) == _type &&
				tlv_packer::length(tl) == _length);
	if (suc) cout << "Ok."; else cout << "Error.";
	cout << endl;
	return suc;
}

bool tlv_packer::test() {
	bool failed = false;
	failed |= !test_packed_tl(0, 0);
	failed |= !test_packed_tl(31, 1);
	failed |= !test_packed_tl(31, 2);
	failed |= !test_packed_tl(8, 4);
	failed |= !test_packed_tl(31, 4);
	failed |= !test_packed_tl(31, 8);
	failed |= !test_packed_tl(32, 32);
	failed |= !test_packed_tl(31, 255);
	failed |= !test_packed_tl(32, 255);
	failed |= !test_packed_tl(32, 256);
	failed |= !test_packed_tl(64, 32);
	failed |= !test_packed_tl(63, 255);
	failed |= !test_packed_tl(64, 255);
	failed |= !test_packed_tl(64, 256);

	failed |= !test_packed_tl(0xFFFFFFFF, 0xFFFFFFFF);
	failed |= !test_packed_tl(0x00FFFFFF, 0xFFFFFFFF);
	failed |= !test_packed_tl(0x0000FFFF, 0xFFFFFFFF);
	failed |= !test_packed_tl(0x000000FF, 0xFFFFFFFF);
	failed |= !test_packed_tl(0x00000000, 0xFFFFFFFF);

	failed |= !test_packed_tl(0xFFFFFFFF, 0xFFFFFFFF);
	failed |= !test_packed_tl(0xFFFFFFFF, 0x00FFFFFF);
	failed |= !test_packed_tl(0xFFFFFFFF, 0x0000FFFF);
	failed |= !test_packed_tl(0xFFFFFFFF, 0x000000FF);
	failed |= !test_packed_tl(0xFFFFFFFF, 0x00000000);

	cout << "Tests have " << (failed ? "FAILED." : "SUCCEEDED.") << endl;
	return failed;
}

}
