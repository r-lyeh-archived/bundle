/*	MCM file compressor

	Copyright (C) 2015, Google Inc.
	Authors: Mathieu Chartier

	LICENSE

    This file is part of the MCM file compressor.

    MCM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MCM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MCM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _DELTA_FILTER_HPP_
#define _DELTA_FILTER_HPP_

#include <memory>

#include "Filter.hpp"
#include "Memory.hpp"

// Encoding format:
// E8/E9 FF/00 XX XX XX
// E8/E8 B2 <char> -> E8/E9 <char>
// E8/E9 <other> -> E8/E9 <other>
class DeltaFilter : public ByteStreamFilter<16 * KB, 20 * KB> {
public:
	DeltaFilter(Stream* stream, uint32_t bytes, uint32_t offset /* multiple of bytes*/) : ByteStreamFilter(stream), prev_(0), bytes_(bytes), offset_(offset), pos_(0) {
		prev_storage_.reset(new uint32_t[offset]);
		prev_ = prev_storage_.get();
		std::fill(prev_, prev_ + offset, 0U);
	}
	virtual void forwardFilter(byte* out, size_t* out_count, byte* in, size_t* in_count) {
		process<true>(out, out_count, in, in_count);
	}
	virtual void reverseFilter(byte* out, size_t* out_count, byte* in, size_t* in_count) {
		process<false>(out, out_count, in, in_count);
	}
	static uint32_t getMaxExpansion() {
		return 1;
	}
	void dumpInfo() const {
	}
	void setSpecific(uint32_t s) {
	}
	
private:
	template <bool encode>
	void process(byte* out, size_t* out_count, byte* in, size_t* in_count) {
		const auto max_c = std::min(*out_count, *in_count);
		if (max_c < bytes_) {
			memcpy(out, in, max_c);
			*out_count = *in_count = max_c;
		} else {
			size_t i;
			for (i = 0; i <= max_c - bytes_; i += bytes_) {
				uint32_t cur = static_cast<uint32_t>(readBytes<uint32_t, false>(in + i, bytes_));
				uint32_t prev = prev_[pos_];
				if (encode) {
					prev_[pos_] = cur;
					cur -= prev;
				} else {
					cur += prev;
					prev_[pos_] = cur;
				}
				writeBytes<uint32_t, false>(out + i, bytes_, cur);
				if (++pos_ == offset_) {
					pos_ = 0;
				}
			}
			*out_count = *in_count = i;
		}
	}

	std::unique_ptr<uint32_t[]> prev_storage_;
	uint32_t* prev_;
	size_t bytes_;
	size_t offset_;
	size_t pos_;
};

template <uint32_t bytes, uint32_t offset>
class FixedDeltaFilter : public DeltaFilter {
public:
	FixedDeltaFilter(Stream* stream) : DeltaFilter(stream, bytes, offset) { }
	void setOpt(size_t) {}
};

#endif
