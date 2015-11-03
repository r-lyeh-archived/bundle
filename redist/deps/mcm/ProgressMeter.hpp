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

#ifndef _PROGRESS_METER_HPP_
#define _PROGRESS_METER_HPP_

#include <condition_variable>
#include <iostream>
#include <thread>

#include "Stream.hpp"

class ProgressMeter {
	uint64_t count;
	clock_t start;
	bool encode;
public:
	ProgressMeter(bool encode = true)
		: count(0)
		, start(clock())
		, encode(encode) {
	}

	~ProgressMeter() {
#ifndef _WIN64
		_mm_empty();
#endif
	}

	forceinline uint64_t getCount() const {
		return count;
	}

	forceinline uint64_t addByte() {
		return ++count;
	}

	bool isEncode() const {
		return encode;
	}

	// Surprisingly expensive to call...
	void printRatio(uint64_t comp_size, const std::string& extra) {
		printRatio(comp_size, count, extra);
	}

	// Surprisingly expensive to call...
	void printRatio(uint64_t comp_size, uint64_t in_size, const std::string& extra) const {
		const auto ratio = double(comp_size) / in_size;
		auto cur_time = clock();
		auto time_delta = cur_time - start;
		if (!time_delta) {
			++time_delta;
		}
		const uint32_t rate = uint32_t(double(in_size / KB) / (double(time_delta) / double(CLOCKS_PER_SEC)));
		std::cout << in_size / KB << "KB ";
		if (comp_size > KB) {
			std::cout << (encode ? "->" : "<-") << " " << comp_size / KB << "KB ";
		} else {
			std::cout << ", ";
		}
		std::cout << rate << "KB/s";
		if (comp_size > KB) {
			std::cout << " ratio: " << std::setprecision(5) << std::fixed << ratio << extra.c_str();
		}
		std::cout << "\t\r" << std::flush;
	}

	forceinline void addBytePrint(uint64_t total, const char* extra = "") {
		if (!(addByte() & 0xFFFF)) {
			// 4 updates per second.
			// if (clock() - prev_time > 250) {
			// 	printRatio(total, extra);
			// }
		}
	}
};

// DEPRECIATED
class ProgressStream : public Stream {
	static const size_t kUpdateInterval = 512 * KB;
public:
	ProgressStream(Stream* in_stream, Stream* out_stream, bool encode = true)
		: in_stream_(in_stream), out_stream_(out_stream), meter_(encode), update_count_(0) {
	}
	virtual int get() {
		byte b;
		if (read(&b, 1) == 0) {
			return EOF;
		}
		return b;
	}
	virtual size_t read(byte* buf, size_t n) {
		size_t ret = in_stream_->read(buf, n);
		update_count_ += ret;
		if (update_count_ > kUpdateInterval) {
			update_count_ -= kUpdateInterval;
			//meter_.printRatio(out_stream_->tell(), in_stream_->tell(), "");
		}
		return ret;
	}
	virtual void put(int c) {
		byte b = c;
		write(&b, 1);
	}
	virtual void write(const byte* buf, size_t n) {
		out_stream_->write(buf, n);
		addCount(n);
	}
	void addCount(size_t delta) {
		update_count_ += delta;
		if (update_count_ > kUpdateInterval) {
			update_count_ -= kUpdateInterval;
			size_t comp_size = out_stream_->tell(), other_size = in_stream_->tell();
			if (!meter_.isEncode()) {
				std::swap(comp_size, other_size);
			}
			//meter_.printRatio(comp_size, other_size, "");
		}
	}
	virtual uint64_t tell() const {
		return meter_.isEncode() ? in_stream_->tell() : out_stream_->tell();
	}
	virtual void seek(uint64_t pos) {
		(meter_.isEncode() ? in_stream_ : out_stream_)->seek(pos);
	}

private:
	Stream* const in_stream_;
	Stream* const out_stream_;
	ProgressMeter meter_;
	uint64_t update_count_;
};

class ProgressThread {
public:
	ProgressThread(Stream* in_stream, Stream* out_stream, bool encode = true, uint64_t sub_out = 0, uintptr_t interval = 250)
		: done_(false), interval_(interval), in_stream_(in_stream), out_stream_(out_stream), sub_out_(sub_out), meter_(encode) {
		thread_ = new std::thread(Callback, this);
	}
	virtual ~ProgressThread() {
		done();
		delete thread_;
	}
	void done() {
		if (!done_) {
			{
				ScopedLock mu(mutex_);
				done_ = true;
				cond_.notify_one();
			}
			thread_->join();
		}
	}
	void print() {
		/*auto out_c = out_stream_->tell() - sub_out_;
		auto in_c = in_stream_->tell();
		if (in_c != 0 && out_c != 0) {
			meter_.printRatio(out_c, in_c, "");
		}*/
	}

private:
	bool done_;
	const uintptr_t interval_;
	Stream* const in_stream_;
	Stream* const out_stream_;
	const uint64_t sub_out_;
	ProgressMeter meter_;
	std::thread* thread_;
	std::mutex mutex_;
	std::condition_variable cond_;

	void run() {
		std::unique_lock<std::mutex> lock(mutex_);
		while (!done_) {
			cond_.wait_for(lock, std::chrono::milliseconds(interval_));
			//print();
		}
	}
	static void Callback(ProgressThread* thr) {
		thr->run();
	}
};

#endif
