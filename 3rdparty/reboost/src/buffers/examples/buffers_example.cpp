//-----------------------------------------------------------------------------
// Part of reboost (http://reboost.org).  Released under the
// BSD 2-clause license (http://www.opensource.org/licenses/bsd-license.php).
// Copyright 2012, Sebastian Mies <mies@reboost.org> --- All rights reserved.
//-----------------------------------------------------------------------------

#include <iostream>
#include <ctype.h>

using namespace std;

#include "../buffers.hpp"


#include "../tlv/tlv.hpp"
#include "../tlv/tlv_list.hpp"
#include "../tlv/tlv_array.hpp"

using namespace reboost;

tlv_list test_concat() {
	return tlv_t("Test")+tlv_t(1.25f)+tlv_t("Sebastian Mies");
}



int main() {
	message_t m = (shared_buffer_t("Hello") + "World");
	cout << m << endl;
	m.push_back("Hello again!");
	message_t cm = m;
	cout << m << endl;
	tlv_t t("Hallo");
	cout << t << endl;
	shared_buffer_t packed = t.pack();
	cout << packed << endl;
	cout << tlv_t::unpack(packed) << endl;

	tlv_list list;
	list.push_back( tlv_t::uint(26) );
	list.push_back( tlv_t::uint(8) );
	list.push_back( tlv_t::uint(76) );
	list.push_back( "Sebastian Mies" );
	list.push_back( "Gebhardstraße 25" );
	list.push_back( "76137 Karlsruhe" );
	list.push_back( "76137 Karlsruhe kajdsflkjaslkfjlk adskjfasldf  adskfjf" );
	list.push_back( 1.25f );
	cout << list << endl;
	shared_buffer_t buf = list.pack();
	cout << buf << endl;
	tlv_list list2 = tlv_list::unpack(buf);
	cout << list2 << endl;
	tlv_t tlvx = tlv_t::unpack(buf);
	cout << tlvx << endl;
	tlvx.compress();
	cout << tlvx << endl;
	tlvx.decompress();
	cout << tlvx << endl;

	tlv_array arr(tlv_t::UINT, 4, 4);
	arr[0]=26;
	arr[1]=8;
	arr[2]=1976;
	arr.resize(3);
	tlv_array arr2 = arr.dup();
	arr2[0]=10;

	cout << arr << endl;
	arr.push_back( tlv_t::uint(1980,4) );
	cout << arr << endl;
	shared_buffer_t abuf = arr.pack();
	cout << abuf << endl;
	cout << tlv_array::unpack(abuf) << endl;

	message_t msg;
	tlv_list lst2;
	lst2 = test_concat();
	msg << lst2;
	cout << msg << endl;
	tlv_t tlv1;
	msg >> tlv1;
	cout << tlv1 << endl;

	return 0;
}
