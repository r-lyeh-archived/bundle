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
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _MEMORY_HPP_
#define _MEMORY_HPP_

#include "Util.hpp"

class MemMap {
	void* storage;
	size_t size;
public:
	inline size_t getSize() const {
		return size;
	}

	void resize(size_t bytes);
	void release();
	void zero();

	inline const void* getData() const {
		return storage;
	}

	inline void* getData() {
		return storage;
	}

	MemMap();
	virtual ~MemMap();
};

template <typename T, bool kBigEndian>
T readBytes(byte* ptr, size_t bytes) {
	T acc = 0;
	if (!kBigEndian) {
		ptr += bytes;
	}
	for (size_t i = 0; i < bytes; ++i) {
		acc = (acc << 8) | (kBigEndian ? *ptr++ : *--ptr);
	}
	return acc;
}

template <typename T, bool kBigEndian>
void writeBytes(byte* ptr, size_t bytes, T value) {
	T acc = 0;
	if (kBigEndian) {
		ptr += bytes;
	}
	for (size_t i = 0; i < bytes; ++i) {
		*(kBigEndian ? --ptr : ptr++) = value;
		value >>= 8;
	}
}

#endif
