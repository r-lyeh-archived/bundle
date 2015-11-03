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

#ifndef _FILE_STREAM_HPP_
#define _FILE_STREAM_HPP_

#include <cassert>
#include <fstream>
#include <mutex>
#include <stdio.h>
#include <sstream>

#include "Compressor.hpp"
#include "Stream.hpp"

#ifndef WIN32
#define _fseeki64 fseeko
#define _ftelli64 ftello
// extern int __cdecl _fseeki64(FILE *, int64_t, int);
// extern int64_t __cdecl _ftelli64(FILE *);
#endif

class FileInfo {
public:
	FileInfo() : attributes(0) {
	}
private:
	uint32_t attributes;
	std::string name;
};

class FilePath {
public:
	FilePath(const std::string& name = "") : name(name) {
	}

	std::string getName() const {
		return name;
	}

	void setName(const std::string& new_name) {
		name = new_name;
	}

	bool isEmpty() const {
		return name.empty();
	}

private:
	std::string name;
};

inline std::vector<FileInfo> EnumerateFiles(const std::string& dir) {
	std::vector<FileInfo> ret;
	return ret;
}

class File : public Stream {
protected:
	std::mutex lock;
	uint64_t offset; // Current offset in the file.
	FILE* handle;
public:
	File()
		: handle(nullptr),
		  offset(0) {
	}

	std::mutex& getLock() {
		return lock;
	}

	// Not thread safe.
	void write(const uint8_t* bytes, size_t count) {
		size_t ret = fwrite(bytes, 1, count, handle);
		offset += ret;
		check(ret == count);
	}

	// Not thread safe.
	forceinline void put(int c) {
		++offset;
		fputc(c, handle);
	}

	int close() {
		int ret = 0;
		if (handle != nullptr) {
			ret = fclose(handle);
			handle = nullptr;
		}
		offset = 0; // Mark
		return ret;
	}

	void rewind() {
		offset = 0;
		::rewind(handle);
	}

	bool isOpen() const {
		return handle != nullptr;
	}
	
	// Return 0 if successful, errno otherwise.
	int open(const std::string& fileName, std::ios_base::open_mode mode = std::ios_base::in | std::ios_base::binary) {
		std::ostringstream oss;
		if (mode & std::ios_base::out) {
			oss << "w";
			if (mode & std::ios_base::in) {
				oss << "+";
			}
		} else if (mode & std::ios_base::in) {
			oss << "r";
		}

		if (mode & std::ios_base::binary) {
			oss << "b";
		}
		handle = fopen(fileName.c_str(), oss.str().c_str());
		if (handle != nullptr) {
			offset = 0;
			return 0;
		}
		return errno;
	}

	// Not thread safe.
	size_t read(uint8_t* buffer, size_t bytes) {
		size_t ret = fread(buffer, 1, bytes, handle);
		offset += ret;
		return ret;
	}

	// Not thread safe.
	int get() {
		++offset;
		return fgetc(handle);
	}

	uint64_t tell() const {
		return offset;
	}

	void seek(uint64_t pos) {
		auto res = seek(static_cast<uint64_t>(pos), SEEK_SET);
		dcheck(res == 0);
	}
	int seek(int64_t pos, int origin) {
		if (origin == SEEK_SET && pos == offset) {
			return 0; // No need to do anything.
		}
		int ret = _fseeki64(handle, pos, origin);
		if (ret == 0) {
			if (origin != SEEK_SET) {
				// Don't necessarily know where the end is.
				offset = _ftelli64(handle);
			} else {
				offset = pos;
				assert(_ftelli64(handle) == offset);
			}
		}
		return ret;
	}

	forceinline FILE* getHandle() {
		return handle;
	}

	// Atomic read.
	// TODO: fread already acquires a lock.
	forceinline size_t readat(uint64_t pos, uint8_t* buffer, size_t bytes) {
		std::unique_lock<std::mutex> mu(lock);
		seek(pos);
		return read(buffer, bytes);
	}

	// Atomic write.
	// TODO: fwrite already acquires a lock.
	forceinline void writeat(uint64_t pos, const uint8_t* buffer, size_t bytes) {
		std::unique_lock<std::mutex> mu(lock);
		seek(pos);
		write(buffer, bytes);
	}

	// Atomic get (slow).
	forceinline int aget(uint64_t pos) {
		std::unique_lock<std::mutex> mu(lock);
		seek(pos);
		return get();
	}

	// Atomic write (slow).
	forceinline void aput(uint64_t pos, byte c) {
		std::unique_lock<std::mutex> mu(lock);
		seek(pos);
		put(c);
	}

	forceinline uint64_t length() {
		std::unique_lock<std::mutex> mu(lock);
		seek(0, SEEK_END);
		uint64_t length = tell();
		seek(0, SEEK_SET);
		return length;
	}
};

// Used to mirror data within a file.
class FileMirror {
	bool is_dirty_;
	uint64_t offset_;
public:
	// Update the file if it is dirty.
	void update() {
		if (isDirty()) {
			write();
			setDirty(true);
		}
	}
	// Read the corresponding data from the file.
	virtual void write() {
	}
	// Read the corresponding data from the file.
	virtual void read() {
	}
	void setDirty(bool dirty) {
		is_dirty_ = dirty;
	}
	bool isDirty() const {
		return is_dirty_;
	}
	void setOffset(uint64_t offset) {
		offset_ = offset;
	}
	uint64_t getOffset() const {
		return offset_;
	}
};

template <const uint32_t size = 16 * KB>
class BufferedFileStream {
public:
	static const uint64_t mask = size - 1;
	byte buffer[size];
	uint64_t total, eof_pos;
	File* file;

private:
	void flush() {
		if ((total & mask) != 0) {
			file->write(buffer, static_cast<uint32_t>(total & mask));
		}
	}

	void flushWhole() {
		assert((total & mask) == 0);
		file->write(buffer, static_cast<uint32_t>(size));
	}

public:
	inline uint64_t getTotal() const {
		return total;
	}

	inline bool eof() const {
		return total >= eof_pos;
	}

	inline bool good() const {
		return !eof();
	}

	int open(const std::string& fileName, std::ios_base::open_mode mode = std::ios_base::in | std::ios_base::binary) {
		close();
		return file->open(fileName, mode);
	}

	inline void write(byte ch) {
		uint64_t pos = total++;
		buffer[pos & mask] = ch;
		if ((total & mask) == 0) {
			flushWhole();
		}
	}
	
	void prefetch() {
		uint64_t readCount = file->read(buffer, size);
		if (readCount != size) {
			eof_pos = total + readCount;
		}
	}

	inline int read() {
		if (!(total & mask)) {
			prefetch();
		}
		if (LIKELY(total < eof_pos)) {
			return (int)(byte)buffer[total++ & mask];
		} else {
			return EOF;
		}
	}

	// Go back to the start of the file.
	void restart() {
		total = 0;
		eof_pos = (uint32_t)-1;
		file->rewind();
	}

	void close() {
		if (file->isOpen()) {
			flush();
			file->close();
			total = 0;
		}
	}
		
	void reset() {
		file->close();
		total = 0;
		eof_pos = (uint32_t)-1;
	}

	BufferedFileStream() {
		reset();
	}

	virtual ~BufferedFileStream() {
		close();
	} 
};

// FileSegmentStream is an advanced stream which reads / writes from arbitrary sections from multiple files.
class FileSegmentStream : public Stream {
public:
	struct SegmentRange {
		// 32 bits for reducing memory usage.
		uint64_t offset_;
		uint64_t length_;
	};
	class FileSegments {
	public:
		Stream* stream_;
		uint64_t base_offset_;
		uint64_t total_size_;  // Used to optimized seek.
		std::vector<SegmentRange> ranges_;

		void calculateTotalSize() {
			total_size_ = 0;
			for (const auto& seg : ranges_) total_size_ += seg.length_;
		}

		void write(Stream* stream) {
			stream->leb128Encode(base_offset_);
			stream->leb128Encode(ranges_.size());
			check(ranges_.size());
			uint64_t prev = 0;
			for (const auto& r : ranges_) {
				stream->leb128Encode(static_cast<uint64_t>(r.length_));
			}
			for (const auto& r : ranges_) {
				uint64_t delta = r.offset_ - prev;
				stream->leb128Encode(delta);
				prev = r.offset_ + r.length_;
			}
		}

		void read(Stream* stream) {
			base_offset_ = stream->leb128Decode();
			const auto num_ranges = stream->leb128Decode();
			check(num_ranges < 10000000);
			uint64_t prev = 0;
			ranges_.resize(num_ranges);
			for (auto& r : ranges_) {
				r.length_ = stream->leb128Decode();
			}
			for (auto& r : ranges_) {
				uint64_t delta = stream->leb128Decode();
				r.offset_ = prev + delta;
				prev = r.offset_ + r.length_;
			}
		}
	};

	FileSegmentStream(std::vector<FileSegments>* segments, uint64_t count)
		: segments_(segments), count_(count) {
		seekStart();
	}
	void seekStart() {
		file_idx_ = -1;
		range_idx_ = 0;
		num_ranges_ = 0;
		cur_pos_ = 0;
		cur_end_ = 0;
	}
	virtual void put(int c) {
		const uint8_t b = c;
		write(&b, 1);
	}
	virtual void write(const uint8_t* buf, size_t n) {
		process<true>(const_cast<uint8_t*>(buf), n);
	}
	virtual int get() {
		uint8_t c;
		if (read(&c, 1) == 0) return EOF;
		return c;
	}
	virtual size_t read(uint8_t* buf, size_t n) {
		return process<false>(buf, n);
	}
	virtual uint64_t tell() const {
		return count_;
	}

private:
	std::vector<FileSegments>* const segments_;
	int file_idx_;
	Stream* cur_stream_;
	size_t range_idx_;
	size_t num_ranges_;
	uint64_t cur_pos_;
	uint64_t cur_end_;
	uint64_t count_;

	template <bool w>
	size_t process(uint8_t* buf, size_t n) {
		const uint8_t* start = buf;
		const uint8_t* limit = buf + n;
		while (buf < limit) {
			if (cur_pos_ >= cur_end_) {
				nextRange();
				if (cur_pos_ >= cur_end_) break;
			}
			const size_t max_c = std::min(
				static_cast<size_t>(limit - buf), static_cast<size_t>(cur_end_ - cur_pos_));
			size_t count;
			if (w) {
				cur_stream_->writeat(cur_pos_, buf, max_c);
				count = max_c;	
			} else {
				count = cur_stream_->readat(cur_pos_, buf, max_c);
			}
			cur_pos_ += count;
			buf += count;
		}
		count_ += buf - start;
		return buf - start;
	}
	void nextRange() {
		while (cur_pos_ >= cur_end_) {
			if (range_idx_ < num_ranges_) {
				auto* segs = &segments_->operator[](file_idx_);
				auto* range = &segs->ranges_[range_idx_];
				cur_pos_ = segs->base_offset_ + range->offset_;
				cur_end_ = cur_pos_ + range->length_;
				++range_idx_;
			} else {
				// Out of ranges in current file, go to next file.
				++file_idx_;
				if (file_idx_ >= segments_->size()) return;
				auto* segs = &segments_->operator[](file_idx_);
				num_ranges_ = segs->ranges_.size();
				cur_stream_ = segs->stream_;
				range_idx_ = 0;
			}
		}
	}
};

class FileManager {
public:
	class CachedFile {
	public:
		std::string name;
		std::ios_base::open_mode mode;
		File file;
		uint32_t count;
		
		File* getFile() {
			return &file;
		}

		const std::string& getName() const {
			return name;
		}

		CachedFile() : count(0) {
		}
	};

	// Clean up.
	CachedFile* open(const std::string& name, std::ios_base::open_mode mode = std::ios_base::binary) {
 		CachedFile* ret = nullptr;
		lock.lock();
		auto it = files.find(name);
		if (it == files.end()) {
			ret = files[name] = new CachedFile;
			ret->name = name;
			ret->file.open(name.c_str(), mode);
			ret->mode = mode;
		} else {
			// Make sure that our modes match.
			// assert(it->mode == mode);
		}
		++ret->count;
		lock.unlock();
		return ret;
	}

	void close_file(CachedFile*& file) {
		bool delete_file = false;
		lock.lock();
		if (!--file->count) {
			auto it = files.find(file->getName());
			assert(it != files.end());
			files.erase(it);
			delete_file = true;
		}
		file = nullptr;
		lock.unlock();
		if (delete_file) {
			delete file;
		}
	}

	virtual ~FileManager() {
		while (!files.empty()) {
			close_file(files.begin()->second);
		}
	}

private:
	std::map<std::string, CachedFile*> files;
	std::mutex lock;
};

forceinline WriteStream& operator << (WriteStream& stream, uint8_t c) {
	stream.put(c);
	return stream;
}

forceinline WriteStream& operator << (WriteStream& stream, int8_t c) {
	return stream << static_cast<uint8_t>(c);
}

forceinline WriteStream& operator << (WriteStream& stream, uint16_t c) {
	return stream << static_cast<uint8_t>(c >> 8) << static_cast<uint8_t>(c);
}

forceinline WriteStream& operator << (WriteStream& stream, int16_t c) {
	return stream << static_cast<uint16_t>(c);
}

inline WriteStream& operator << (WriteStream& stream, uint32_t c) {
	return stream << static_cast<uint16_t>(c >> 16) << static_cast<uint16_t>(c);
}

inline WriteStream& operator << (WriteStream& stream, int32_t c) {
	return stream << static_cast<uint32_t>(c);
}

inline WriteStream& operator << (WriteStream& stream, uint64_t c) {
	return stream << static_cast<uint16_t>(c >> 32) << static_cast<uint16_t>(c);
}

inline WriteStream& operator << (WriteStream& stream, int64_t c) {
	return stream << static_cast<uint64_t>(c);
}

inline WriteStream& operator << (WriteStream& stream, float c) {
	return stream << *reinterpret_cast<uint32_t*>(&c);
}

inline WriteStream& operator << (WriteStream& stream, double c) {
	return stream << *reinterpret_cast<uint64_t*>(&c);
}

#endif
