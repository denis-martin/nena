//-----------------------------------------------------------------------------
// Part of reboost (http://reboost.org).  Released under the
// BSD 2-clause license (http://www.opensource.org/licenses/bsd-license.php).
// Copyright 2012, Sebastian Mies <mies@reboost.org> --- All rights reserved.
//-----------------------------------------------------------------------------

#include "tlv.hpp"
#include "tlv_list.hpp"
#include "internal/miniz.h"

namespace reboost {

using namespace std;

// nil tlv
const tlv_t tlv_t::nil;
const tlv_t tlv_t::truetlv(tlv_t::TRUE,0,"");
const tlv_t tlv_t::falsetlv(tlv_t::FALSE,0,"");


// decompress tlv using miniz
void tlv_t::uncompress() {
	if(type_ != COMPRESSED) return;
	shared_buffer_t c(size());
	size_t sz;
	int res = mz_uncompress((unsigned char *)c.mutable_data(), &sz, data(), size());
	if (res==0) {
		c.resize(sz);
		*this = unpack(c);
	}
}

// compress tlv using miniz
void tlv_t::compress() {
	shared_buffer_t p = pack();
	shared_buffer_t c(p.size()*2);
	size_t sz;
	int res = mz_compress((unsigned char *)c.mutable_data(), &sz, p.data(), p.size());
	if (res==0 && sz <= p.size()) {
		type_ = COMPRESSED;
		c.resize(sz);
		super::operator=(c);
	}
}

ostream& operator<<(ostream& os, const tlv_t& tlv) {
	switch (tlv.type()) {
	case tlv_t::TEXT: {
		os << "\"";
		for (size_t i=0; i<tlv.size(); i++) os << (const char)tlv[i];
		return os << "\"";
	}
	case tlv_t::SINT:
	case tlv_t::UINT:
	case tlv_t::VINTPOS: {
		uintmax_t value = 0;
		for (size_t i=0; i<tlv.size(); i++) value |= tlv[i]<<(i*8);
		if (tlv.type()==tlv_t::SINT && tlv[tlv.size()-1] >= 0x80) {
			for (size_t i=tlv.size(); i<sizeof(uintmax_t); i++) value |= 0xFF << (i*8);
			intmax_t neg = value;
			return os << neg;
		} else
			return os << value;
	}
	case tlv_t::VINTNEG: {
		uintmax_t value = 0;
		for (size_t i=0; i<tlv.size(); i++) value |= tlv[i]<<(i*8);
		return os << "-" << value;
	}
	case tlv_t::FLOAT: {
		if (tlv.size() == sizeof(float)) {
			float f; memcpy(&f, tlv.data(), sizeof(float));
			return os << f;
		} else
		if (tlv.size() == sizeof(double)) {
			double f; memcpy(&f, tlv.data(), sizeof(double));
			return os << f;
		} else
			return os << "<FLOAT UNKNOWN>";
	}
	case tlv_t::ARRAY:
	case tlv_t::LIST: {
		tlv_list list = tlv_list::unpacki(tlv.type(), tlv);
		return os << list;
	}

	case tlv_t::COMPRESSED: {
		return os << "(type=COMPRESSED size="<< tlv.size() <<")";
	}

	default: {
		return os << "(type="<< tlv.type()
				<< ":" << static_cast<const buffer_t>(tlv) << ")";
	}
	}
}


}




