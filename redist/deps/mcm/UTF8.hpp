/*	MCM file compressor

	Copyright (C) 2013, Google Inc.
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

#ifndef _UTF_8_HPP_
#define _UTF_8_HPP_

#include "Util.hpp"

template <bool error_check = false>
class UTF8Decoder {
	// Accumulator word.
	uint32_t extra;
	uint32_t acc;
	bool error;
public:
	UTF8Decoder() {
		init();
		error = false;
	}

	void init() {
		extra = 0;
		acc = 0;
		error = false;
	}

	forceinline bool done() const {
		return !extra;
	}

	forceinline uint32_t getAcc() const {
		return acc;
	}

	forceinline bool err() const {
		return error;
	}

	forceinline void clear_err() const {
		error = false;
	}

	forceinline void update(uint32_t c) {// 5 extra bits
		if (extra) {
			// Add another char to the UTF char.
			acc = (acc << 6) | (c & 0x3F);
			--extra;
			if (error_check && (c >> 6) != 2) {
				error = true;
			}
		} else if (!(c & 0x80)) {
			acc = c & 0x7F;
		} else if ((c & (0xFF << 1)) == ((0xFE << 1) & 0xFF)) {
			acc = c & 0x1; // 1
			extra = 5;
		} else if ((c & (0xFF << 2)) == ((0xFE << 2) & 0xFF)) {
			acc = c & 0x3; // 2
			extra = 4;
		} else if ((c & (0xFF << 3)) == ((0xFE << 3) & 0xFF)) {
			acc = c & 0x7; // 3
			extra = 3;
		} else if ((c & (0xFF << 4)) == ((0xFE << 4) & 0xFF)) {
			acc = c & 0xF; // 4
			extra = 2;
		} else if ((c & (0xFF << 5)) == ((0xFE << 5) & 0xFF)) {
			acc = c & 0x1F; // 5
			extra = 1;
		} else {
			acc = c;
			if (error_check) {
				error = true;
			}
		}
	}
};

#endif
