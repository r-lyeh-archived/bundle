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

#ifndef _DIV_TABLE_HPP_
#define _DIV_TABLE_HPP_

template <typename T, const uint32_t shift_, const uint32_t size_>
class DivTable {
	T data[size_];
public:
	DivTable() {
		
	}

	void init() {
		for (uint32_t i = 0; i < size_; ++i) {
			data[i] = int(1 << shift_) / int(i * 16 + 1);
		}
	}

	forceinline uint32_t size() const {
		return size_;
	}

	forceinline T& operator [] (T i) {
		return data[i];
	}

	forceinline T operator [] (T i) const {
		return data[i];
	}
};

#endif
