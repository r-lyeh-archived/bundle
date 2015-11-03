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

#ifndef _DETECTOR_HPP_
#define _DETECTOR_HPP_

#include <fstream>
#include <deque>

#include "CyclicBuffer.hpp"
#include "Dict.hpp"
#include "Stream.hpp"
#include "UTF8.hpp"
#include "Util.hpp"

// Detects blocks and data type from input data
class Detector {
	bool is_forbidden[256]; // Chars which don't appear in text often.
	
	// MZ pattern, todo replace with better detection.
	typedef std::vector<byte> Pattern;
	Pattern exe_pattern;

	// Lookahed.
	CyclicDeque<uint8_t> buffer_;

	// Out buffer, only used to store headers (for now).
	StaticArray<uint8_t, 16 * KB> out_buffer_;
	size_t out_buffer_pos_, out_buffer_size_;

	// Read / write stream.
	Stream* stream_;

	// Opt var
	size_t opt_var_;
public:
	// Pre-detected.
	enum Profile {
		kProfileText,
		kProfileBinary,
		kProfileWave16,
		kProfileEOF,
		kProfileCount,
		// Not a real profile, tells CM to use streaming detection.
		kProfileDetect,
	};

	class DetectedBlock {
	public:
		DetectedBlock(Profile profile = kProfileBinary, uint32_t length = 0)
			: profile_(profile), length_(length) {
		}
		DetectedBlock(const DetectedBlock& other) {
			*this = other;
		}
		DetectedBlock& operator=(const DetectedBlock& other) {
			profile_ = other.profile_;
			length_ = other.length_;
			return *this;
		}

		static size_t calculateLengthBytes(size_t length) {
			if (length & 0xFF000000) return 4;
			if (length & 0xFF0000) return 3;
			if (length & 0xFF00) return 2;
			return 1;
		}
		static size_t getSizeFromHeaderByte(uint8_t b) {
			return 1 + getLengthBytes(b);
		}
		static size_t getLengthBytes(uint8_t b) {
			return (b >> kLengthBytesShift) + 1;
		}
		size_t write(uint8_t* ptr) {
			const auto* orig_ptr = ptr;
			size_t enc_len = length_ - 1;
			const auto length_bytes = calculateLengthBytes(enc_len);
			*(ptr++) = static_cast<uint8_t>(profile_) | static_cast<uint8_t>((length_bytes - 1) << kLengthBytesShift);
			for (size_t i = 0; i < length_bytes; ++i) {
				*(ptr++) = static_cast<uint8_t>(enc_len >> (i * 8));
			}
			return ptr - orig_ptr;
		}
		size_t read(const uint8_t* ptr) {
			const auto* orig_ptr = ptr;
			auto c = *(ptr++);
			profile_ = static_cast<Profile>(c & kDataProfileMask);
			auto length_bytes = getLengthBytes(c);
			length_ = 0;
			for (size_t i = 0; i < length_bytes; ++i) {
				length_ |= static_cast<uint32_t>(*(ptr++)) << (i * 8);
			}
			++length_;
			return ptr - orig_ptr;
		}
		Profile profile() const {
			return profile_;
		}
		uint64_t length() const {
			return length_;
		}
		void setLength(uint64_t length) {
			length_ = length;
		}
		void extend(uint64_t len) {
			length_ += len;
		}
		// Remove one character from length.
		void pop(uint64_t count = 1) {
			assert(length_ >= count);
			length_ -= count;
		}

	private:
		static const size_t kLengthBytesShift = 6;
		static const size_t kDataProfileMask = (1u << kLengthBytesShift) - 1;
		Profile profile_;
		uint64_t length_;
	};

	static std::string profileToString(Profile profile) {
		switch (profile) {
		case kProfileBinary: return "binary";
		case kProfileText: return "text";
		case kProfileWave16: return "wav16";
		}
		return "unknown";
	}

	// std::vector<DetectedBlock> detected_blocks_;
	DetectedBlock current_block_;

	// Detected but not already read.
	DetectedBlock detected_block_;

	// Saved detected blocks.
	std::deque<DetectedBlock> saved_blocks_;

	// Statistics
	uint64_t num_blocks_[kProfileCount];
	uint64_t num_bytes_[kProfileCount];
	uint64_t overhead_bytes_;
	uint64_t small_len_;

	// Last things.
	uint32_t last_word_;
public:

	Detector(Stream* stream) : stream_(stream), opt_var_(0), last_word_(0) {
	}

	void setOptVar(size_t var) {
		opt_var_ = var;
	}

	void init() {
		overhead_bytes_ = 0;
		small_len_ = 0;
		for (size_t i = 0; i < kProfileCount; ++i) {
			num_blocks_[i] = num_bytes_[i] = 0;
		}
		out_buffer_pos_ = out_buffer_size_ = 0;
		for (auto& b : is_forbidden) b = false;

		const byte forbidden_arr[] = {
			0, 1, 2, 3, 4,
			5, 6, 7, 8, 11,
			12, 14, 15, 16, 17,
			19, 20, 21, 22, 23,
			24, 25, 26, 27, 28,
			29, 30, 31
		};
		for (auto c : forbidden_arr) is_forbidden[c] = true;
		
		buffer_.resize(256 * KB);
		// Exe pattern
		byte p[] = {0x4D, 0x5A, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xFF, 0xFF,};
		exe_pattern.clear();
		for (auto& c : p) exe_pattern.push_back(c);
	}

	void refillRead() {
		const size_t kBufferSize = 4 * KB;
		uint8_t buffer[kBufferSize];
		for (;;) {
			const size_t remain = buffer_.capacity() - buffer_.size();
			const size_t n = stream_->read(buffer, std::min(kBufferSize, remain));
			for (size_t i = 0; i < n; ++i) {
				buffer_.push_back(buffer[i]);
			}
			if (n == 0 || remain == 0) break;
		}
	}

	forceinline bool empty() const {
		return size() == 0;
	}

	forceinline size_t size() const {
		return buffer_.size();
	}

	void put(int c) {
		// Profile can't extend past the end of the buffer.
		if (current_block_.length() > 0) {
			current_block_.pop();
			if (buffer_.size() >= buffer_.capacity()) {
				flush();
			}
			buffer_.push_back(c);
		} else {
			out_buffer_[out_buffer_pos_++] = static_cast<uint8_t>(c);
			auto num_bytes = DetectedBlock::getSizeFromHeaderByte(out_buffer_[0]);
 			if (out_buffer_pos_ == num_bytes) {
				current_block_.read(&out_buffer_[0]);
				if (current_block_.profile() == kProfileEOF) {
					out_buffer_pos_ = 0;
				}
				out_buffer_pos_ = 0;
			}
		}
	}
	
	Profile detect() {
		if (current_block_.length() > 0) {
			return current_block_.profile();
		}
		if (current_block_.profile() == kProfileEOF) {
			return kProfileEOF;
		}
		return kProfileBinary;
	}

	void flush() {
		// TODO: Optimize
		BufferedStreamWriter<4 * KB> sout(stream_);
		while (buffer_.size() != 0) {
			sout.put(buffer_.front());
			buffer_.pop_front();
		}
		sout.flush();
	}

	forceinline uint32_t at(uint32_t index) const {
		assert(index < buffer_.size());
		return buffer_[index];
	}

	int get(Profile& profile) {
		// Profile can't extend past the end of the buffer.
		if (false && current_block_.length() == 0) {
			current_block_ = detectBlock();
		}
		if (current_block_.length() > 0) {
			profile = current_block_.profile();
			return readChar();
		} 
		// Still have some header to read?
		if (out_buffer_pos_ < out_buffer_size_) {
			if (++out_buffer_pos_ == out_buffer_size_) {
				current_block_ = detected_block_;
			}
			overhead_bytes_ += out_buffer_size_;
			profile = kProfileBinary;
			return out_buffer_[out_buffer_pos_  - 1];
		} 
		if (current_block_.profile() == kProfileEOF) {
			profile = kProfileEOF;
			return EOF;
		}
		detected_block_ = detectBlock();
		++num_blocks_[detected_block_.profile()];
		num_bytes_[detected_block_.profile()] += detected_block_.length();
		if (detected_block_.length() < 64) ++small_len_;
		out_buffer_size_ = detected_block_.write(&out_buffer_[0]);
		profile = kProfileBinary;
		out_buffer_pos_ = 1;
		return out_buffer_[0];
	}

	// Read char without detection.
	uint8_t readChar() {
		current_block_.pop();
		return popChar();
	}
	int popChar() {
		if (buffer_.empty()) {
			refillRead();
			if (buffer_.empty()) {
				return EOF;
			}
		}
		auto ret = buffer_.front();
		buffer_.pop_front();
		return ret;
	}
	size_t read(uint8_t* out, size_t count) {
		const auto n = std::min(count, buffer_.size());
		for (size_t i = 0; i < n; ++i) {
			out[i] = buffer_[i];
		}
		buffer_.pop_front(n);
		current_block_.pop(n);
		return n;
	}

	void dumpInfo() {
		std::cout << "Detector overhead " << formatNumber(overhead_bytes_) << " small=" << small_len_ <<std::endl;
		for (size_t i = 0; i < kProfileCount; ++i) {
			std::cout << profileToString(static_cast<Profile>(i)) << "("
				<< formatNumber(num_blocks_[i]) << ") : " << formatNumber(num_bytes_[i]) << std::endl;
		}
	}

	DetectedBlock detectBlock() {
		if (!saved_blocks_.empty()) {
			auto ret = saved_blocks_.front();
			saved_blocks_.pop_front();
			return ret;
		}
		refillRead();
		const size_t buffer_size = buffer_.size();
		if (buffer_size == 0) {
			return DetectedBlock(kProfileEOF, 0);
		}
		if (false) {
			return DetectedBlock(kProfileBinary, static_cast<uint32_t>(buffer_.size()));
		}

		size_t binary_len = 0;
		while (binary_len < buffer_size) {
			UTF8Decoder<true> decoder;
			size_t text_len = 0;
			while (binary_len + text_len < buffer_size) {
				size_t pos = binary_len + text_len;
				if (true && last_word_ == 0x52494646) {
					refillRead();
					// This is pretty bad, need a clean way to do it.
					uint32_t fpos = pos;
					uint32_t chunk_size = readBytes(fpos, 4, false); fpos += 4;
					uint32_t format = readBytes(fpos); fpos += 4;
					// Format subchunk.
					uint32_t subchunk_id = readBytes(fpos); fpos += 4;
					if (format == 0x57415645 && subchunk_id == 0x666d7420) {
						uint32_t subchunk_size = readBytes(fpos, 4, false); fpos += 4;
						if (subchunk_size == 16 || subchunk_size == 18) {
							uint32_t audio_format = readBytes(fpos, 2, false); fpos += 2;
							uint32_t num_channels = readBytes(fpos, 2, false); fpos += 2;
							if (audio_format == 1 && num_channels == 2) {
								fpos += subchunk_size - 6;
								// fpos += 4; // Skip: Sample rate
								// fpos += 4; // Skip: Byte rate
								// fpos += 2; // Skip: Block align
								uint32_t bits_per_sample = readBytes(fpos, 2, false); fpos += 2;
								for (size_t i = 0; i < 5; ++i) {
									uint32_t subchunk2_id = readBytes(fpos, 4); fpos += 4;
									uint32_t subchunk2_size = readBytes(fpos, 4, false); fpos += 4;
									if (subchunk2_id == 0x64617461) {
										if (subchunk2_size >= chunk_size) {
											break;
										}
										saved_blocks_.push_back(DetectedBlock(kProfileWave16, chunk_size));
										return DetectedBlock(kProfileBinary, fpos);
										// Read wave header, TODO binary block as big as fpos?? Need to be able to queue subblocks then.
										// profile_length = fpos + subchunk2_size;
										// profile = kWave;
										// return profile;
									} else {
										fpos += subchunk2_size;
										if (fpos >= buffer_.size()) break;
									}
									}
							}
						} 
					}
				}
				auto c = buffer_[pos];
				last_word_ = (last_word_ << 8) | c;
				decoder.update(c);
				if (decoder.err() || is_forbidden[static_cast<uint8_t>(c)]) {
					break; // Error state?
				}
				++text_len;
			}
			if (text_len > 146) {
				if (binary_len == 0) {
					return DetectedBlock(kProfileText, static_cast<uint32_t>(text_len));
				} else {
					break;
				}
			} else {
				binary_len += text_len;
				if (binary_len >= buffer_size) {
					break;
				}
				++binary_len;
			}
		}
		return DetectedBlock(kProfileBinary, static_cast<uint32_t>(binary_len));
	}

	forceinline size_t readBytes(size_t pos, size_t bytes = 4, bool big_endian = true) {
		if (pos + bytes > buffer_.size()) {
			return 0;
		}
		uint32_t w = 0;
		// Past the end of buffer :(
		if(big_endian) {
			for (size_t i = 0; i < bytes; ++i) {
				w = (w << 8) | buffer_[pos + i];
			}
		} else {
			for (size_t shift = 0, i = 0; i < bytes; ++i, shift += 8) {
				w |= static_cast<uint32_t>(buffer_[pos + i]) << shift;
			}
		}
		return w;
	}
};

// Detector analyzer, analyze a whole stream.
class Analyzer {
public:
	typedef std::vector<Detector::DetectedBlock> Blocks;

	void analyze(Stream* stream) {
		Detector detector(stream);
		detector.init();
		for (;;) {
			auto block = detector.detectBlock();
			if (block.profile() == Detector::kProfileEOF) {
				break;
			}
			for (size_t i = 0; i < block.length(); ++i) {
				auto c = detector.popChar();
				if (c == EOF) {
					block.setLength(i);
					break;
				}
				if (block.profile() == Detector::kProfileText) {
					dict_builder_.addChar(c);
				}
			}
			const size_t size = blocks_.size();
			if (size > 0 && blocks_.back().profile() == block.profile()) {
				// Same type, extend.
				blocks_.back().extend(block.length());
			} else {
				const size_t min_binary_length = 1;
				// replace <text> <bin> <text> with <text> if |<bin>| < min_binary_length.
				if (block.profile() == Detector::kProfileText && size >= 2) {
					auto& b1 = blocks_[size - 1];
					auto& b2 = blocks_[size - 2];
					if (b1.profile() == Detector::kProfileBinary &&
						b2.profile() == Detector::kProfileText && 
						b1.length() < min_binary_length) {
						b2.extend(b1.length() + block.length());
						blocks_.pop_back();
						continue;
					}
				}
				blocks_.push_back(block);
			}
		}
	}
	void dump() {
		uint64_t blocks[Detector::kProfileCount] = { 0 };
		uint64_t bytes[Detector::kProfileCount] = { 0 };
		for (auto& b : blocks_) {
			++blocks[b.profile()];
			bytes[b.profile()] += b.length();
		}
		for (size_t i = 0; i < Detector::kProfileCount; ++i) {
			if (bytes[i] > 0) {
				std::cout << Detector::profileToString(static_cast<Detector::Profile>(i))
					<< " : " << blocks[i] << "(" << prettySize(bytes[i]) << ")" << std::endl;
			}
		}
	}
	Blocks& getBlocks() {
		return blocks_;
	}
	Dict::Builder& getDictBuilder() {
		return dict_builder_;
	}

private:
 	Blocks blocks_;
	Dict::Builder dict_builder_;
};

#endif
