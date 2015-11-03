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

#ifndef _DICT_HPP_
#define _DICT_HPP_

#include <algorithm>
#include <map>
#include <memory>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

#include "Filter.hpp"

class Dict {
public:
	static const size_t kMaxWordLen = 256;
	static const size_t kInvalidChar = 256;
	typedef std::pair<size_t, std::string> WCPair;
	
	class CompareWCPair {
	public:
		CompareWCPair(size_t extra_cost = 0) : extra_cost_(extra_cost) {
		}
		bool operator()(const WCPair& a, const WCPair& b) const {
			return (a.first - 1) * (std::max(extra_cost_, a.second.length()) - extra_cost_) <
				(b.first - 1) * (std::max(extra_cost_, b.second.length()) - extra_cost_);
		}

	private:
		const size_t extra_cost_;
	};

	// Maybe not very memory efficient.
	class WordCollectionMap {
		std::unordered_map<std::string, uint32_t> words_;
		size_t max_words_;
	public:

		WordCollectionMap(size_t max_words = 10000000) : max_words_(max_words) {
		}
		void getWords(std::vector<WCPair>& out_pairs, size_t min_occurences = 10) {
			for (auto& p : words_) {
				if (p.second > min_occurences) {
					out_pairs.push_back(WCPair(p.second, p.first));
				}
			}
		}
		forceinline size_t size() const {
			return words_.size();
		}
		void clear() {
			words_.clear();
		}
		void addWord(const uint8_t* begin, const uint8_t* end) {
			while (words_.size() > max_words_) {
				for (auto it = words_.begin(); it != words_.end(); ) {
					it->second /= 2;
					if (it->second == 0) {
						it = words_.erase(it);
					} else {
						++it;
					}
				}
			}
			++words_[std::string(begin, end)];
		}
	};

	class CodeWord {
	public:
		uint32_t code_;

		void setCode(uint32_t code) {
			code_ = code;
		}
		CodeWord(uint8_t num_bytes = 0u, uint8_t c1 = 0u, uint8_t c2 = 0u, uint8_t c3 = 0u) {
			code_ = num_bytes;
			code_ = (code_ << 8) | c1;
			code_ = (code_ << 8) | c2;
			code_ = (code_ << 8) | c3;
		}
		uint32_t byte1() const {
			return (code_ >> 16) & 0xFFu;
		}
		uint32_t byte2() const {
			return (code_ >> 8) & 0xFFu;
		}
		uint32_t byte3() const {
			return (code_ >> 0) & 0xFFu;
		}
		size_t numBytes() const {
			auto bytes = code_ >> 24;
			assert(bytes <= 3);
			return bytes;
		}
	};

	typedef std::pair<std::string, CodeWord> CodeWordPair;
	// Simple collection of words and their codes.
	class CodeWordSet {
	public:
		// Map from words to their codes.
		size_t num1_;
		size_t num2_;
		size_t num3_;
		std::vector<std::string> codewords_;

		std::vector<std::string>* getCodeWords() {
			return &codewords_;
		}
	};

	class SuffixSortComparator {
	public:
		SuffixSortComparator(const uint8_t* arr) : arr_(arr) {
		}
		forceinline bool operator()(uint32_t a, uint32_t b) const {
			while (a > 0 && b > 0) {
				if (arr_[--a] != arr_[--b]) {
					return arr_[a] < arr_[b];
				}
			}
			return a < b;
		}
	private:
		const uint8_t* const arr_;
	};

	class Builder {
		static const size_t kSuffixSize = 100 * MB;
		// Suffix array buffer.
		std::vector<uint8_t> buffer_;
		size_t buffer_pos_;
		// Current word.
		static const size_t kMinWordLen = 3;
		static const size_t kMaxWordLen = 0x20;
		static const size_t kMinOccurences = 100;
		uint8_t word_[kMaxWordLen];
		size_t word_pos_;
		// CC: first char EOR whole word.
		WordCollectionMap words_;
	public:
		void addChar(uint8_t c) {
			// Add to current word.
			if (isWordChar(c)) {
				if (word_pos_ < kMaxWordLen) {
					word_[word_pos_++] = c;
				}
			} else {
				if (word_pos_ >= kMinWordLen) {
					bool first_cap = isUpperCase(word_[0]);
					size_t cap_count = first_cap;
					for (size_t i = 1; i < word_pos_; ++i) {
						cap_count += isUpperCase(word_[i]);
					}
					if (cap_count == word_pos_) {
						for (size_t i = 0; i < word_pos_; ++i) {
							word_[i] = makeLowerCase(word_[i]);
						}
					} else if (first_cap && cap_count == 1) {
						word_[0] = makeLowerCase(word_[0]);
					}
					words_.addWord(word_, word_ + word_pos_);
				}
				/*
				for (size_t i = 0; i < word_pos_; ++i) {
					if (buffer_pos_ < buffer_.size()) buffer_[buffer_pos_++] = word_[i];
				}
				if (buffer_pos_ < buffer_.size()) buffer_[buffer_pos_++] = c;
				*/
				word_pos_ = 0;
			}
		}
		void init() {
			buffer_pos_ = 0;
			buffer_.resize(kSuffixSize);
			word_pos_ = 0;
		}
		Builder() {
			init();
		}
		const WordCollectionMap& getWords() const {
			return words_;
		}
		WordCollectionMap& getWords() {
			return words_;
		}
		const std::vector<uint8_t>* getBuffer() const {
			return &buffer_;
		}
	};

	class CodeWordGenerator {
	public:
		virtual void generate(Builder& builder, CodeWordSet& code_words) = 0;
	};

	class CodeWordGeneratorFast {
		static const bool kVerbose = true;
	public:
		void generateCodeWords(Builder& builder, CodeWordSet* words) {
			auto start_time = clock();
			auto* cw = words->getCodeWords();
			cw->clear();

			auto& word_map = builder.getWords();
			std::vector<WCPair> word_pairs;
			const size_t kMinOccurences = 9u;
			word_map.getWords(word_pairs, kMinOccurences);
			word_map.clear();
			const auto occurences = word_pairs.size();
			std::sort(word_pairs.rbegin(), word_pairs.rend(), CompareWCPair(1));

			// Calculate number of 1 byte codewords in case its more than the original max.
			words->num1_ = std::min(static_cast<size_t>(32u), word_pairs.size());
			while (words->num1_ + 1 < word_pairs.size()) {
				// Remain.
				size_t remain = 128 - words->num1_;
				const size_t new_2 = (remain - 1) * (remain - 1);
				if (remain == 0 || new_2 < word_pairs.size() - words->num1_) {
					break;
				}
				++words->num1_;
			}

			size_t save1 = 0, save2 = 0, save3 = 0; // Number of bytes saved.
			size_t num1 = words->num1_, num2 = 0, num3 = 0; // Number of first byte which are 1b/2b/3b.
			size_t count1 = 0, count2 = 0, count3 = 0; /// Number of words.
			for (size_t i = 0; i < words->num1_; ++i) {
				const auto& p = word_pairs[i];
				++count1;
				save1 += (p.first - 1) * (p.second.length() - 1);
				cw->push_back(p.second);
			}
			std::sort(cw->begin(), cw->end());
			word_pairs.erase(word_pairs.begin(), word_pairs.begin() + count1);
			std::sort(word_pairs.rbegin(), word_pairs.rend(), CompareWCPair(2));

			// 2 byte codes.
			for (num3 = 0; num3 < 32 && num3 + num1 < 128; ++num3) {
				const size_t count3 = num3 * 128 * 128;
				const size_t count2 = (128u - num3 - num1) * 128;
				if (count2 + count3 >= word_pairs.size()) break;
			}
			words->num3_ = num3;
			const size_t end2 = 256 - words->num3_;
			words->num2_ = end2 - std::min(static_cast<size_t>(128 + num1), end2);
			for (size_t b1 = 128u + num1; b1 < end2; ++b1) {
				for (size_t b2 = 128u; b2 < 256; ++b2) {
					if (count2 < word_pairs.size()) {
						const auto& p = word_pairs[count2++];
						cw->push_back(p.second);
						save2 += (p.first - 1) * (p.second.length() - 2);
					}
				}
			}
			word_pairs.erase(word_pairs.begin(), word_pairs.begin() + count2);
			std::sort(word_pairs.rbegin(), word_pairs.rend(), CompareWCPair(3));

			// 3 byte codes.
			for (size_t b1 = end2; b1 < 256; ++b1) {
				for (size_t b2 = 128u; b2 < 256; ++b2) {
					for (size_t b3 = 128u; b3 < 256; ++b3) {
						if (count3 < word_pairs.size()) {
							const auto& p = word_pairs[count3++];
							cw->push_back(p.second);
							save3 += (p.first - 1) * (p.second.length() - 3);
						}
					}
				}
			}
			word_pairs.erase(word_pairs.begin(), word_pairs.begin() + count3);
			
			if (false) {
				// High mode, incomplete.
				// Total / count.
				typedef std::pair<uint64_t, uint64_t> MeanPair;
				std::unordered_map<std::string, MeanPair> words2b;
				std::unordered_map<std::string, MeanPair> words3b;
				auto it = cw->begin() + count1;
				for (; it != cw->begin() + count1 + count2; ++it) {
					words2b.insert(std::make_pair(*it, MeanPair(0, 0)));
				}
				for (; it != cw->end(); ++it) {
					words3b.insert(std::make_pair(*it, MeanPair(0, 0)));
				}
				
				// Expensive suffix sort approach.
				auto* buffer = builder.getBuffer();
				// Find interesting indexes.
				std::vector<uint32_t> indexes;
				size_t num_words = 0;
				static const size_t kSuffixOffset = 0;
				const uint8_t* arr = &buffer->operator[](0);
				for (size_t pos = 0; pos < buffer->size(); ) {
					if (isWordChar(arr[pos])) {
						size_t len = 0;
						for (;pos + len < buffer->size() && isWordChar(arr[pos + len]); ++len) {
						}
						std::string s(arr + pos, arr + pos + len);
						if (words2b.find(s) != words2b.end() || words3b.find(s) != words3b.end()) {
							indexes.push_back(pos + kSuffixOffset);
						}
						++num_words;
						pos += len;
					} else {
						++pos;
					}
				}

				SuffixSortComparator cmp(arr);
				auto start = clock();
				//std::cout << "sorting " << indexes.size() << "/" << num_words << std::endl;
				std::sort(indexes.begin(), indexes.end(), cmp);
				//std::cout << "sorting took " << clockToSeconds(clock() - start) << std::endl;

				// Use suffix to build avg indexes.
				// Whether or not do dump suffixes.
				// std::ofstream fout("suffixes.txt");
				for (size_t i = 0; i < indexes.size(); ++i) {
					size_t pos = indexes[i] - kSuffixOffset;
					check(isWordChar(arr[pos]));
					size_t len = 0;
					for (;pos + len < buffer->size() && isWordChar(arr[pos + len]); ++len) {
					}
					std::string s(arr + pos, arr + pos + len);

#if 0
					auto end = pos + len;
					std::string str(std::max(arr, arr + pos - 12), std::min(arr + end, arr + buffer->size()));
					for (auto& c : str) {
						if (c == '\n') c = '_';
						if (c == '\t') c = '_';
					}
					fout << str << std::endl;
#endif

					auto it = words2b.find(s);
					if (it == words2b.end()) {
						it = words3b.find(s);
						check(it != words3b.end());
					}
					it->second.first += i;
					++it->second.second;
				}

				// Re-sort based on averages.
				std::vector<std::pair<uint32_t, std::string>> sort_arr;
				// 2b first
				for (auto& p : words2b) {
					sort_arr.push_back(std::make_pair(static_cast<uint32_t>(p.second.first / p.second.second), p.first));
				}
				std::sort(sort_arr.begin(), sort_arr.end());
				it = cw->begin() + count1;
				for (auto& p : sort_arr) {
					*(it++) = p.second;
				}

				// 3b first
				sort_arr.clear();
				for (auto& p : words3b) {
					sort_arr.push_back(std::make_pair(static_cast<uint32_t>(p.second.first / p.second.second), p.first));
				}
				std::sort(sort_arr.begin(), sort_arr.end());
				for (auto& p : sort_arr) {
					*(it++) = p.second;
				}

			} else {
				std::sort(cw->begin() + count1, cw->begin() + count1 + count2);
				std::sort(cw->begin() + count1 + count2, cw->end());
			}
			if (kVerbose) {
				// Remaining chars.
				size_t remain = 0;
				for (const auto& p : word_pairs) remain += (p.first - 1) * (p.second.length() - 3);
				/*std::cout << "Constructed dict words=" << count1 << "+" << count2 << "+" << count3 << "=" << occurences
					<< " save=" << save1 << "+" << save2 << "+" << save3 << "=" << save1 + save2 + save3
					<< " extra=" << remain
					<< " time=" << clockToSeconds(clock() - start_time) << "s"
					<< std::endl;*/

			}
		}
	};

	// Encodes / decods words / code words.
	class Filter : public ByteStreamFilter<16 * KB, 16 * KB> {
		// Capital conersion.
		size_t escape_char_;
		size_t escape_cap_first_;
		size_t escape_cap_word_;
		// Currrent word
		uint8_t word_[kMaxWordLen];
		size_t word_pos_;
		// Read / write dict buffer.
		std::vector<uint8_t> dict_buffer_;
		size_t dict_buffer_pos_;
		size_t dict_buffer_size_;

		// Encoding data structures.
		std::unordered_map<std::string, CodeWord> encode_map_;

		// State
		uint8_t last_char_;

		// Decode data structures
		std::vector<std::string> words1b;
		size_t word1bstart;
		std::vector<std::string> words2b;
		size_t word2bstart;
		std::vector<std::string> words3b;
		size_t word3bstart;

		// Optimizations
		uint8_t is_word_char_[256];
	public:
		// Serialize to and from.
		// num words
		// escape
		// escape cap first
		// escape cap word
		// 1b count
		// 2b count
		// 3b count
		void writeDict(std::vector<uint8_t>* dict) {
			WriteVectorStream wvs(dict);
		}
		void readDict() {

		}

		// Creates an encodable dictionary array.
		void addCodeWords(std::vector<std::string>* words, uint8_t num1, uint8_t num2, uint8_t num3) {
			// Create the dict array.
			WriteVectorStream wvs(&dict_buffer_);
			// Save space for dict size.
			dict_buffer_.push_back(0u);
			dict_buffer_.push_back(0u);
			dict_buffer_.push_back(0u);
			dict_buffer_.push_back(0u);
			// Encode escapes and such.
			dict_buffer_.push_back(static_cast<uint8_t>(escape_char_));
			dict_buffer_.push_back(static_cast<uint8_t>(escape_cap_first_));
			dict_buffer_.push_back(static_cast<uint8_t>(escape_cap_word_));
			dict_buffer_.push_back(num1);
			dict_buffer_.push_back(num2);
			dict_buffer_.push_back(num3);
			// Encode words.
			for (const auto& s : *words) {
				wvs.writeString(&s[0]);
			}
			// Save size.
			dict_buffer_pos_ = 0;
			dict_buffer_size_ = dict_buffer_.size();
			dict_buffer_[0] = static_cast<uint8_t>(dict_buffer_size_ >> 24);
			dict_buffer_[1] = static_cast<uint8_t>(dict_buffer_size_ >> 16);
			dict_buffer_[2] = static_cast<uint8_t>(dict_buffer_size_ >> 8);
			dict_buffer_[3] = static_cast<uint8_t>(dict_buffer_size_ >> 0);
			// Generate the actual encode map.
			generate(*words, num1, num2, num3, true);
			//std::cout << "Dictionary words=" << words->size() << " size=" << prettySize(dict_buffer_.size()) << std::endl;
		}
		void createFromBuffer() {
			ReadMemoryStream rms(&dict_buffer_);
			size_t pos = 0;
			// Save space for dict size.
			for (size_t i = 0; i < 4; ++i) {
				rms.get();
			}
			// Encode escapes and such.
			escape_char_ = rms.get();
			escape_cap_first_ = rms.get();
			escape_cap_word_ = rms.get();
			size_t num1 = rms.get();
			size_t num2 = rms.get();
			size_t num3 = rms.get();
			word1bstart = 128;
			word2bstart = word1bstart + num1;
			word3bstart = word2bstart + num2;
			// Encode words.
			std::vector<std::string> words;	
			while (rms.tell() != dict_buffer_.size()) {
				words.push_back(rms.readString());
			}
			// Generate the actual encode map.
			generate(words, num1, num2, num3, false);
			//std::cout << "Dictionary words=" << words.size() << " size=" << prettySize(dict_buffer_.size()) << std::endl;
		}
		void generate(std::vector<std::string>& words, size_t num1, size_t num2, size_t num3, bool encode) {
			static const size_t start = 128u;
			size_t end1 = start + num1;
			size_t end2 = end1 + num2;
			size_t end3 = end2 + num3;
			size_t idx = 0;
			for (size_t b1 = start; b1 < end1; ++b1) {
				if (idx < words.size()) {
					if (encode) {
						encode_map_.insert(std::make_pair(words[idx++], CodeWord(1, static_cast<uint8_t>(b1))));
					} else {
						words1b.push_back(words[idx++]);
					}
				}
			}
			for (size_t b1 = end1; b1 < end2; ++b1) {
				for (size_t b2 = start; b2 < 256; ++b2) {
					if (idx < words.size()) {
						if (encode) {
							encode_map_.insert(std::make_pair(words[idx++], CodeWord(2, static_cast<uint8_t>(b1), static_cast<uint8_t>(b2))));
						} else {
							words2b.push_back(words[idx++]);
						}
					}
				}
			}
			for (size_t b1 = end2; b1 < end3; ++b1) {
				for (size_t b2 = start; b2 < 256; ++b2) {
					for (size_t b3 = start; b3 < 256; ++b3) {
						if (idx < words.size()) {
							if (encode) {
								encode_map_.insert(std::make_pair(
									words[idx++], CodeWord(3, static_cast<uint8_t>(b1), static_cast<uint8_t>(b2), static_cast<uint8_t>(b3))));
							} else {
								words3b.push_back(words[idx++]);
							}
						}
					}
				}
			}
		}
		virtual void forwardFilter(uint8_t* out, size_t* out_count, uint8_t* in, size_t* in_count) {
			uint8_t* in_ptr = in;
			uint8_t* out_ptr = out;
			const uint8_t* const in_limit = in + *in_count;
			const uint8_t* const out_limit = out + *out_count;
			const size_t remain_dict = dict_buffer_size_ - dict_buffer_pos_;
			if (remain_dict > 0) {
				const size_t max_write = std::min(remain_dict, *out_count);
				std::copy(&dict_buffer_[0] + dict_buffer_pos_, &dict_buffer_[0] + dict_buffer_pos_ + max_write, out_ptr);
				out_ptr += max_write;
				dict_buffer_pos_ += max_write;
			}
			while (in_ptr < in_limit && out_ptr + 4 < out_limit) {
				if (!is_word_char_[last_char_]) {
					if (is_word_char_[*in_ptr]) {
						size_t word_len = 0;
						while (word_len < kMaxWordLen && in_ptr + word_len < in_limit && is_word_char_[in_ptr[word_len]]) {
							++word_len;
						}
						if (in_ptr + word_len >= in_limit && word_len != in_limit - in) {
							// If the word is all the remaining chars and not the whole string, then it may be a prefix.
							break;
						}
						const size_t max_out = static_cast<size_t>(out_limit - out_ptr);
						if (word_len >= 3 && word_len <= kMaxWordLen) {
							std::string word(in_ptr, in_ptr + word_len);
							size_t upper_count = 0;
							for (size_t i = 0; i < word_len; ++i) {
								upper_count += isUpperCase(in_ptr[i]);
							}
							if (upper_count == word_len) {
								for (auto& c : word) c = makeLowerCase(c);
							} else if (upper_count == 1 && isUpperCase(in_ptr[0])) {
								word[0] = makeLowerCase(word[0]);
							}
							auto it = encode_map_.find(word);
							if (it != encode_map_.end()) {
								if (upper_count == word_len) {
									*(out_ptr++) = escape_cap_word_;
								} else if (upper_count == 1 && isUpperCase(in_ptr[0])) {
									*(out_ptr++) = escape_cap_first_;
								}
								auto& code_word = it->second;
								const auto num_bytes = code_word.numBytes();
								dcheck(num_bytes >= 1 && num_bytes <= 3);
								*(out_ptr++) = code_word.byte1();
								if (num_bytes > 1) *(out_ptr++) = code_word.byte2();
								if (num_bytes > 2) *(out_ptr++) = code_word.byte3();
								in_ptr += word_len;
								last_char_ = 'a';
								continue;
							}
						}
					}
					if (*in_ptr == escape_char_ || *in_ptr == escape_cap_first_ || *in_ptr == escape_cap_word_ || *in_ptr >= 128) {
						*(out_ptr++) = escape_char_;
					}
				}
				*(out_ptr++) = last_char_ = *(in_ptr++);
			}
			dcheck(in_ptr <= in_limit);
			dcheck(out_ptr <= out_limit);
			*in_count = in_ptr - in;
			*out_count = out_ptr - out;
		}
		virtual void reverseFilter(uint8_t* out, size_t* out_count, uint8_t* in, size_t* in_count) {
			const uint8_t* in_ptr = in;
			uint8_t* out_ptr = out;
			const uint8_t* const in_limit = in + *in_count;
			const uint8_t* const out_limit = out + *out_count;
			while (dict_buffer_.size() < dict_buffer_size_ && in_ptr < in_limit) {
				dict_buffer_.push_back(*(in_ptr++));
				if (dict_buffer_.size() == 4) {
					dict_buffer_size_ = static_cast<uint32_t>(dict_buffer_[0]) << 24;
					dict_buffer_size_ |= static_cast<uint32_t>(dict_buffer_[1]) << 16;
					dict_buffer_size_ |= static_cast<uint32_t>(dict_buffer_[2]) << 8;
					dict_buffer_size_ |= static_cast<uint32_t>(dict_buffer_[3]) << 0;
				} else if (dict_buffer_.size() == dict_buffer_size_) {
					createFromBuffer();
				}
			}
			const auto* max = in_ptr + 4 < in_limit ? in_limit - 4 : in_limit;
			const size_t start_byte = 128;
			while (in_ptr < max && out_ptr + kMaxWordLen < out_limit) {
				int c = *(in_ptr++);
				if (!is_word_char_[last_char_]) {
					const bool first_cap = c == escape_cap_first_;
					const bool all_cap = c == escape_cap_word_;
					if (c >= 128 || first_cap || all_cap) {
						if (first_cap || all_cap) c = *(in_ptr++);
						std::string* word;
						assert(c >= 128);
						if (c < word2bstart) {
							word = &words1b[c - word1bstart];
						} else if (c < word3bstart) {
							int c2 = *(in_ptr++);
							assert(c2 >= 128);
							word = &words2b[(c - word2bstart) * 128 + c2 - start_byte];
						} else {
							assert(c >= word3bstart);
							int c2 = *(in_ptr++);
							int c3 = *(in_ptr++);
							assert(c2 >= start_byte);
							assert(c3 >= start_byte);
							word = &words3b[(c - word3bstart) * 128 * 128 + (c2 - start_byte) * 128 + c3 - start_byte];
						}
						const size_t word_len = word->length();
						auto* word_start = &word->operator[](0);
						std::copy(word_start, word_start + word_len, out_ptr);
						const size_t capital_c = all_cap ? word_len : static_cast<size_t>(first_cap);
						for (size_t i = 0; i < capital_c; ++i) {
							out_ptr[i] = makeUpperCase(out_ptr[i]);
						}
						out_ptr += word_len;
						last_char_ = out_ptr[-1];
						continue;
					}
					if (c == escape_char_) {
						c = *(in_ptr++);
					}
				}
				*(out_ptr++) = last_char_ = c;
			}
			dcheck(in_ptr <= in_limit);
			dcheck(out_ptr <= out_limit);
			*in_count = in_ptr - in;
			*out_count = out_ptr - out;
		}
		Filter(Stream* stream) : ByteStreamFilter(stream), dict_buffer_pos_(0), dict_buffer_size_(4), last_char_(0) {
			init();
		}
		Filter(Stream* stream, size_t escape_char, size_t escape_cap_first = kInvalidChar,
			size_t escape_cap_word = kInvalidChar)
			: ByteStreamFilter(stream)
			, escape_char_(escape_char)
			, escape_cap_first_(escape_cap_first)
			, escape_cap_word_(escape_cap_word)
			, last_char_(0) {
			init();
		}
		void init() {
			for (size_t i = 0; i < 256; ++i) {
				is_word_char_[i] = isWordChar(i);
			}
		}
	};
	
	Dict() {

	}
};

#if 0
class SimpleDict : public ByteStreamFilter<16 * KB, 16 * KB> {
	static const size_t kMaxWordLen = 255;
public:
	SimpleDict(Stream* stream, const bool verbose = true || kIsDebugBuild, size_t min_occurences = 10)
		: ByteStreamFilter(stream), built_(false), verbose_(verbose), min_occurences_(min_occurences) {
	}

	virtual void forwardFilter(uint8_t* out, size_t* out_count, uint8_t* in, size_t* in_count) {
		if (false) {
			size_t count = std::min(*out_count, *in_count);
			std::copy(in, in + count, out);
			*in_count = *out_count = count;
			return;
		}
		if (!built_) {
			buildDict();
			built_ = true;
		}
		// TODO: Write the dictionary if required.
		uint8_t* in_ptr = in;
		uint8_t* out_ptr = out;
		uint8_t* in_limit = in + *in_count;
		uint8_t* out_limit = out + *out_count;
		if (dict_pos_ < dict_buffer_.size()) {
			size_t max_cpy = std::min(static_cast<size_t>(out_limit - out_ptr), dict_buffer_.size() - dict_pos_);
			std::copy(&dict_buffer_[0] + dict_pos_, &dict_buffer_[0] + dict_pos_ + max_cpy, out_ptr);
			out_ptr += max_cpy;
			dict_pos_ += max_cpy;
		}
		size_t word_pos = 0;
		char word_buffer[kMaxWordLen + 1];
		while (in_ptr < in_limit) {
			if (out_ptr + 2 >= out_limit) break;
			uint8_t c;
			if (in_ptr + word_pos < in_limit && word_pos < kMaxWordLen && isWordChar(c = in_ptr[word_pos])) {
				word_buffer[word_pos++] = c;
			} else {
				if (word_pos) {
					for (size_t len = word_pos; len >= 3; --len) {
						std::string s(word_buffer, word_buffer + len);
						word_pos = 0;
						auto it1 = one_byte_map_.find(s);
						if (it1 != one_byte_map_.end()) {
							*(out_ptr++) = it1->second.c1_;
							in_ptr += s.length();
							goto cont;
						}
						auto it2 = two_byte_map_.find(s);
						if (it2 != two_byte_map_.end()) {
							*(out_ptr++) = it2->second.c1_;
							*(out_ptr++) = it2->second.c2_;
							in_ptr += s.length();
							goto cont;
						}
					}
				}
				c = *(in_ptr++);
				if (is_char_codes_[c]) {
					*(out_ptr++) = escape_char_;
				}
				*(out_ptr++) = c;
				cont:;
			}
		}
		dcheck(in_ptr <= in_limit);
		dcheck(out_ptr <= out_limit);
		*in_count = in_ptr - in;
		*out_count = out_ptr - out;
	}
	virtual void reverseFilter(uint8_t* out, size_t* out_count, uint8_t* in, size_t* in_count) {
		size_t count = std::min(*out_count, *in_count);
		std::copy(in, in + count, out);
		*in_count = *out_count = count;
	}
	void setOpt(size_t n) {}
	void dumpInfo() {
	}

private:
	typedef std::pair<size_t, std::string> WordCountPair;

	struct DictEntry {
		DictEntry(uint8_t c1 = 0, uint8_t c2 = 0) : c1_(c1), c2_(c2) {}
		uint8_t c1_, c2_;
	};

	void buildDict() {
		dict_pos_ = 0;
		if (verbose_) std::cout << std::endl;
		auto stream_pos = stream_->tell();
		uint64_t counts[256] = { 0 };
		std::map<std::string, size_t> words;
		uint8_t word_buffer[kMaxWordLen + 1];
		size_t pos = 0;
		BufferedStreamReader<4 * KB> reader(stream_);
		for (;;) {
			const int c = reader.get();
			if (c == EOF) break;
			if (isWordChar(c)) {
				word_buffer[pos++] = c;
				if (pos >= kMaxWordLen) {
					pos = 0;
				}
			} else {
				if (pos > 3) {
					++words[std::string(word_buffer, word_buffer + pos)];
				}
				pos = 0;
			}
			++counts[static_cast<uint8_t>(c)];
		}

		std::vector<WordCountPair> sorted_words;
		for (auto it = words.begin(); it != words.end(); ++it) {
			// Remove all the words which hve less than 10 occurences.
			if (it->second >= min_occurences_) {
				sorted_words.push_back(std::make_pair(it->second, it->first));
			}
		}
		words.clear();

		uint64_t total = std::accumulate(counts, counts + 256, 0U);
		// Calculate candidate code bytes.
		char_codes_.clear();
		uint64_t escape_size = 0;
		std::vector<std::pair<uint64_t, byte>> code_pairs;
		for (size_t i = 0; i < 256; ++i) {
			code_pairs.push_back(std::make_pair(counts[i], static_cast<byte>(i)));
		}
		std::sort(code_pairs.begin(), code_pairs.end());
		size_t idx = 0;
		std::cout << std::endl;
		for (auto& p : code_pairs) {
			std::cout << idx++ << " b=" << static_cast<size_t>(p.second) << ":" << p.first << std::endl;
		}	
#if 0
		for (size_t i = 0; i < 170; ++i) {
			char_codes_.push_back(code_pairs[i].second);
		}
#else
		for (size_t i = 128; i < 256; ++i) {
			char_codes_.push_back(i);
		}
#endif
		for (auto& c : char_codes_) {
			escape_size += counts[c];
		}
		// TODO: Properly calculate the escape char?
		escape_char_ = char_codes_.back();
		char_codes_.pop_back();

		Comparator c1(1), c2(2);

		size_t optimal_b1 = char_codes_.size() / 2;
		uint64_t optimal_save = 0;
		if (verbose_) std::cout << "Word size= " << sorted_words.size() << std::endl;
		// b1 is number of 1 byte codes.
		if (!kIsDebugBuild)
		for (size_t b1 = 0; b1 <= char_codes_.size(); ++b1) {
			std::vector<WordCountPair> new_words = sorted_words;
			// b2 is the number of two byte codes.
			const size_t b2 = (char_codes_.size() - b1) * char_codes_.size(); // Two byte code words
		
			std::sort(new_words.rbegin(), new_words.rend(), c1);
			uint64_t save_total1 = 0;
			for (size_t i = 0; i < b1; ++i) {
				save_total1 += c1.wordCost(new_words[i]);
			}
			// Erase the saved 1 byte words.
			new_words.erase(new_words.begin(), new_words.begin() + b1);

			// Re-sort with new criteria.
			std::sort(new_words.rbegin(), new_words.rend(), c2);
			// Calculate 2 byte code savings.
			uint64_t save_total2 = 0;
			for (size_t i = 0; i < std::min(new_words.size(), b2); ++i) {
				save_total2 += c2.wordCost(new_words[i]);
			}
			const auto total_save = save_total1 + save_total2 - escape_size;
			if (total_save > optimal_save) {
				optimal_b1 = b1;
				optimal_save = total_save;
			}
			if (verbose_) {
				std::cout << "len1=" << b1 << ":" << save_total1 << "+" << save_total2 << "-" << escape_size << "=" << total_save << std::endl;
			}
		}
		std::vector<WordCountPair> new_words = sorted_words;
		std::sort(new_words.rbegin(), new_words.rend(), c1);
		uint64_t save_total1 = 0;
		for (size_t i = 0; i < optimal_b1; ++i) {
			one_byte_words_.push_back(new_words[i].second);
		}
		std::sort(one_byte_words_.begin(), one_byte_words_.end());
		// Erase the saved 1 byte words.
		new_words.erase(new_words.begin(), new_words.begin() + optimal_b1);
		// Re-sort with new criteria.
		std::sort(new_words.rbegin(), new_words.rend(), c2);
		// Calculate 2 byte code savings.
		uint64_t save_total2 = 0;
		const size_t b2 = (char_codes_.size() - optimal_b1) * char_codes_.size(); // Two byte code words
		for (size_t i = 0; i < std::min(new_words.size(), b2); ++i) {
			two_byte_words_.push_back(new_words[i].second);
		}
		std::sort(two_byte_words_.begin(), two_byte_words_.end());
		size_t extra = 0;
		for (size_t i = b2;i < new_words.size(); ++i) {
			extra += new_words[i].second.length() * new_words[i].first;
		}

		// Number of 1b words.
		if (verbose_) std::cout << "words=" << sorted_words.size() << " 1b=" << one_byte_words_.size() << " 2b="
			<< two_byte_words_.size() <<  " save=" << optimal_save << " extra=" << extra << std::endl;

		// Go back to start.
		stream_->seek(stream_pos);

		// Create the dict buffer.
		// dict_buffer_
		dict_buffer_.clear();
		dict_buffer_.push_back('D');
		// Dictionary length.
		size_t len_pos_ = dict_buffer_.size();
		dict_buffer_.push_back(0);
		dict_buffer_.push_back(0);
		dict_buffer_.push_back(0);
		dict_buffer_.push_back(0);
		dict_buffer_.push_back(escape_char_);
		// Write char codes.
		dict_buffer_.push_back(char_codes_.size());
		for (auto c : char_codes_) dict_buffer_.push_back(c);
		// Number of 1 byte words.
		dict_buffer_.push_back(one_byte_words_.size());
		std::fill(is_one_byte_char_, is_one_byte_char_ + 256, false);
		std::fill(is_char_codes_, is_char_codes_ + 256, false);
		for (size_t i = 0; i < one_byte_words_.size(); ++i) {
			is_one_byte_char_[char_codes_[i]] = true;
			one_byte_map_[one_byte_words_[i]] = DictEntry(char_codes_[i]);
			writeWord(one_byte_words_[i]);
		}
		// Number of 2 byte codes.
		dict_buffer_.push_back(two_byte_words_.size());
		size_t i = 0;
		for (size_t c1 = one_byte_words_.size(); i < two_byte_words_.size() && c1 < char_codes_.size(); ++c1) {
			for (size_t c2 = 0; i < two_byte_words_.size() && c2 < char_codes_.size(); ++c2) {
				two_byte_map_[two_byte_words_[i]] = DictEntry(char_codes_[c1], char_codes_[c2]);
				writeWord(two_byte_words_[i]);
				i++;
			}
		}
		if (verbose_) std::cout << "Final buffer size " << dict_buffer_.size() << std::endl;
	}

private:
	class Comparator {
	public:
		Comparator(size_t codes_required) {
			SetCodesRequired(codes_required);
		}
		void SetCodesRequired(size_t codes_required) {
			codes_required_ = codes_required;
		}
		size_t wordCost(const WordCountPair& a) const {
			// return a.first;
			if (a.second.length() <= codes_required_) {
				return 0;
			}
			const size_t savings = a.first * (a.second.length() - codes_required_);
			const size_t extra_cost = a.second.length() + 1 + codes_required_;
			if (extra_cost >= savings) {
				return 0;
			}
			return savings - extra_cost;
		}
		bool operator()(const WordCountPair& a, const WordCountPair& b) const {
			return wordCost(a) < wordCost(b);
		}

		size_t codes_required_;
	};

	bool isWordChar(int c) const {
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
	}

	void encode() {
		
	}
	void writeWord(const std::string& s) {
		dcheck(s.length() <= 255);
		dict_buffer_.push_back(s.length());
		for (auto c : s) dict_buffer_.push_back(static_cast<uint8_t>(c));
	}
	std::vector<std::string> one_byte_words_;
	std::map<std::string, DictEntry> one_byte_map_;
	std::vector<std::string> two_byte_words_;
	std::map<std::string, DictEntry> two_byte_map_;
	// Dict buffer for encoding / decoding.
	std::vector<uint8_t> dict_buffer_;
	size_t dict_pos_;
	// Escape code, used to encode non dict char codes.
	uint8_t escape_char_;
	// Char codes
	std::vector<uint8_t> char_codes_;
	// 
	bool is_one_byte_char_[256];
	bool is_char_codes_[256];

	// Options.
	const bool verbose_;
	size_t min_occurences_;

	bool built_;

	class Entry {
	public:
		uint32_t freq;
	};

	// Hash table.
	class Buckets {
	public:

	};

	// Entries.
	static const size_t kDictFraction = 2048;
	std::vector<Entry> entires;
};
#endif

#endif
