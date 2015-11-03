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

#ifndef _SLIDING_WINDOW_HPP_
#define _SLIDING_WINDOW_HPP_
#pragma once

#include <cassert>
#include "Util.hpp"

template <typename T>
class CyclicBuffer {
protected:
	size_t pos_, mask_, alloc_size_;
	T *storage_, *data_;
public:

	forceinline size_t getPos() const {return pos_;}
	forceinline size_t getMask() const {return mask_;}
	forceinline T* getData() { return data_; }

	inline size_t prev(size_t pos, size_t count) const {
		// Relies on integer underflow behavior. Works since pow 2 size.
		return (pos - count) & mask_;
	}

	inline size_t next(size_t pos, size_t count) const {
		return (pos + count) & mask_;
	}

	// Maximum size.
	forceinline size_t getSize() const {
		return mask_ + 1;
	}

	CyclicBuffer() : storage_(nullptr), mask_(0) {
	}

	virtual ~CyclicBuffer() {
		release();
	}

	virtual void restart() {
		pos_ = 0;
	}

	forceinline void push(T val) {
		data_[pos_++ & mask_] = val;
	}

	forceinline T& operator [] (size_t offset) {
		return data_[offset & mask_];
	}
	forceinline T operator [] (size_t offset) const {
		return data_[offset & mask_];
	}
	forceinline T operator () (size_t offset) const {
		return data_[offset];
	}

	virtual void release() {
		pos_ = alloc_size_ = 0;
		mask_ = static_cast<size_t>(-1);
		delete [] storage_;
		storage_ = data_ = nullptr;
	}

	void fill(T d) {
		std::fill(storage_, storage_ + getSize(), d);
	}

	// Can be used for LZ77.
	void copyStartToEndOfBuffer(size_t count) {
		size_t size = getSize();
		for (size_t i = 0 ;i < count ;++i) {
			data_[size + i] = data_[i];
		}
	}

	// Can be used for LZ77.
	void copyEndToStartOfBuffer(size_t count) {
		size_t size = getSize();
		for (size_t i = 0;i < count;++i) {
			storage_[i] = storage_[i + size];
		}
	}

	void resize(size_t new_size, size_t padding = sizeof(uint32_t)) {
		// Ensure power of 2.
		assert((new_size & (new_size - 1)) == 0);
		delete [] storage_;
		mask_ = new_size - 1;
		alloc_size_ = new_size + padding * 2;
		storage_ = new T[alloc_size_]();
		data_ = storage_ + padding;
		restart();
	}
};

template <typename T>
class CyclicDeque : public CyclicBuffer<T> {
	size_t size_;
	size_t front_pos_;
public:
	CyclicDeque() : size_(0), front_pos_(0) {
	}
	forceinline size_t capacity() const {
		return this->mask_ + 1;
	}
	void pop_front(size_t count = 1) {
		dcheck(size_ >= count);
		front_pos_ += count;
		size_ -= count;
	}
	void push_back(T c) {
		assert(size_ < capacity());
		++size_;
		this->push(c);
	}
	forceinline T front() const {
		return this->data_[front_pos_ & this->mask_];
	}
	forceinline size_t size() const {
		return size_;
	}
	forceinline T operator [] (size_t offset) const {
		return this->data_[(front_pos_ + offset) & this->mask_];
	}
	forceinline bool full() const {
		return this->size() == capacity();
	}
	forceinline bool empty() const {
		return this->size() == 0;
	}
};

#endif
