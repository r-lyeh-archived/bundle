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

#ifndef _FILTER_HPP_
#define _FILTER_HPP_

#include <memory>

#include "Stream.hpp"
#include "Util.hpp"

/*
Filter usage:

// in_stream -> filter -> out_stream
Filter f(in_stream);
compress(out_stream, &f);

// in_stream -> filter -> out_stream
Filter f(out_stream);
compres(&f, in_stream);

*/

class Filter : public Stream {
public:
	virtual void flush() {
	}
};

// Byte filter is complicated since filters are not necessarily a 1:1 mapping.
template<uint32_t kInBufferSize = 16 * KB, uint32_t kOutBufferSize = 16 * KB>
class ByteStreamFilter : public Filter {
public:
	void flush() {
		while (in_buffer_.pos() != 0) {
			refillWriteAndProcess();
		}
	}
	explicit ByteStreamFilter(Stream* stream) : stream_(stream), count_(0) {
	}
	virtual int get() {
		if (UNLIKELY(out_buffer_.remain() == 0)) { 
			if (refillReadAndProcess() == 0) {
				return EOF;
			}
		}
		return out_buffer_.get();
	}
	virtual size_t read(byte* buf, size_t n) {
		const byte* start_ptr = buf;
		while (n != 0) {
			size_t remain = out_buffer_.remain();
			if (remain == 0) {
				if ((remain = refillReadAndProcess()) == 0) {
					break;
				}
			}
			const size_t read_count = std::min(remain, n);
			out_buffer_.read(buf, read_count);
			buf += read_count;
			n -= read_count;
		}
		return buf - start_ptr;
	}
	virtual void put(int c) {
		if (in_buffer_.remain() == 0) {
			if (refillWriteAndProcess() == 0) {
				check(false);
			}
		}
		in_buffer_.put(c);
	}
	virtual void write(const byte* buf, size_t n) {
		while (n != 0) {
			size_t remain = in_buffer_.remain();
			if (remain == 0) {
				remain = refillWriteAndProcess();
				check(remain != 0);
			}
			const size_t len = std::min(n, remain);
			in_buffer_.write(buf, len);
			buf += len;
			n -= len;
		}
	}
	virtual void forwardFilter(uint8_t* out, size_t* out_count, uint8_t* in, size_t* in_count) = 0;
	virtual void reverseFilter(uint8_t* out, size_t* out_count, uint8_t* in, size_t* in_count) = 0;
	uint64_t tell() const {
		return count_;
	}

private:
	size_t refillWriteAndProcess() {
		size_t in_pos = 0;
		size_t out_limit = out_buffer_.capacity();
		size_t in_limit = in_buffer_.pos() - in_pos;
		reverseFilter(&out_buffer_[0], &out_limit, &in_buffer_[in_pos], &in_limit);
		in_pos += in_limit;
		stream_->write(&out_buffer_[0], out_limit);
		in_buffer_.erase(in_pos);
		in_buffer_.addSize(in_buffer_.reamainCapacity());
		return in_buffer_.remain();
	}
	size_t refillReadAndProcess() {
		refillRead();  // Try to refill as much of the inbuffer as possible.
		out_buffer_.erase(out_buffer_.pos());  // Erase the characters we already read from the out buffer.
		size_t out_limit = out_buffer_.reamainCapacity();
		size_t in_limit = in_buffer_.pos();
		forwardFilter(out_buffer_.end(), &out_limit, in_buffer_.begin(), &in_limit);
		out_buffer_.addSize(out_limit);  // Add the characters we processed to out.
		count_ += out_limit;
		in_buffer_.erase(in_limit);  // Erase the caracters we processed in in.
		return out_buffer_.size();
	}
	void refillRead() {
		// Read from input until buffer is full.
		size_t count = stream_->read(in_buffer_.end(), in_buffer_.reamainCapacity());
		in_buffer_.addSize(count);
		in_buffer_.addPos(count);
	}

	// In buffer, contains either transformed or untransformed.
	StaticBuffer<uint8_t, kInBufferSize> in_buffer_;
	// Out buffer (passed through filter or reverse filter).
	StaticBuffer<uint8_t, kInBufferSize> out_buffer_;

protected:
	// Proxy stream.
	Stream* const stream_;
	uint64_t count_;
};

class IdentityFilter : public ByteStreamFilter<4 * KB, 4 * KB> {
public:
	IdentityFilter(Stream* stream) : ByteStreamFilter(stream) { }
	virtual void forwardFilter(uint8_t* out, size_t* out_count, uint8_t* in, size_t* in_count) {
		const auto max_c = std::min(*out_count, *in_count);
		std::copy(in, in + max_c, out);
		*out_count = *in_count = max_c;
	}
	virtual void reverseFilter(uint8_t* out, size_t* out_count, uint8_t* in, size_t* in_count) {
		const auto max_c = std::min(*out_count, *in_count);
		std::copy(in, in + max_c, out);
		*out_count = *in_count = max_c;
	}
	void dumpInfo() {
	}
	void setOpt(uint32_t) { }
	static size_t getMaxExpansion() {
		return 1;
	}
};

template <uint32_t kBlockSize = 0x10000>
class ByteBufferFilter : public Filter {
public:
	ByteBufferFilter(Stream* stream) : stream_(stream), block_pos_(0), block_size_(0) {
		block_.reset(new uint8_t[kBlockSize]);
		block_data_ = block_.get();
	}
	void flush() {
		flushWrite();
	}
	virtual int get() {
		if (UNLIKELY(block_pos_ >= block_size_)) {
			if (refillRead() == 0) {
				return EOF;
			}
		}
		return block_data_[block_pos_++];
	}
    virtual size_t read(byte* buf, size_t n) {
		byte* ptr = buf;
		while (n != 0) {
			size_t remain = block_size_ - block_pos_;
			if (remain == 0) {
				remain = refillRead();
				if (remain == 0) {
					break;
				}
			}
			const size_t count = std::min(n, remain);
			std::copy(block_data_ + block_pos_, block_data_ + block_pos_ + count, ptr);
			n -= count;
			block_pos_ += count;
			ptr += count;
		}
		return ptr - buf;
	}
	virtual void put(int c) {
		if (UNLIKELY(block_pos_ >= block_size_)) {
			flushWrite();
		}
		block_data_[block_pos_++] = c;
	}
	virtual void write(const byte* buf, size_t n) {
		while (n != 0) {
			size_t remain = block_size_ - block_pos_;
			if (remain == 0) {
				remain = flushWrite();
				dcheck(remain != 0);
			}
			const size_t count = std::min(n, remain);
			std::copy(buf, buf + count, block_data_ + block_pos_);
			block_pos_ += count;
			buf += count;
			n -= count;
		}
	}
	virtual void forwardFilter(byte* ptr, size_t size) = 0;
	virtual void reverseFilter(byte* ptr, size_t size) = 0;

private:
	size_t refillRead() {
		check(block_pos_ == block_size_);
		block_pos_ = 0;
		block_size_ = stream_->read(block_.get(), kBlockSize);
		forwardFilter(block_data_, block_size_);
		return block_size_;
	}
	size_t flushWrite() {
		block_size_ = kBlockSize;
		reverseFilter(block_data_, block_pos_);
		stream_->write(block_data_, block_pos_);
		block_pos_ = 0;
		return block_size_;
	}

	Stream* const stream_;
	std::unique_ptr<uint8_t[]> block_;
	uint8_t* block_data_;
	size_t block_size_;
	size_t block_pos_;
};


class IdentityBlockFilter : public ByteBufferFilter<0x10000> {
public:
	IdentityBlockFilter(Stream* stream) : ByteBufferFilter(stream) {
	}
	virtual void forwardFilter(byte* ptr, size_t count) {
	}
	virtual void reverseFilter(byte* ptr, size_t count) {
	}
	static uint32_t getMaxExpansion() {
		return 1;
	}
	void dumpInfo() const {
	}
	void setOpt(uint32_t s) {
	}
};

template <typename Compressor, typename Filter>
class FilterCompressor : public Compressor {
public:
	template <typename TOut, typename TIn>
	bool DeCompress(TOut& sout, TIn& sin) {
		auto fout = Filter::Make(sout);
		return Compressor::DeCompress(fout, sin);
	}

	template <typename TOut, typename TIn>
	uint64_t Compress(TOut& sout, TIn& sin) {
		auto fin = Filter::Make(sin);
		return Compressor::Compress(sout, fin);
	}
};

#endif
