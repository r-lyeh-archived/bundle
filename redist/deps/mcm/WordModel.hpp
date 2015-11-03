#ifndef _WORD_MODEL_HPP_
#define _WORD_MODEL_HPP_

#include "UTF8.hpp"

class WordModel {
public:
	// Hashes.
	uint32_t prev;
	uint32_t h1, h2;

	// UTF decoder.
	UTF8Decoder<false> decoder;

	// Length of the model.
	size_t len;
	static const size_t kMaxLen = 31;

	// Transform table.
	static const uint32_t transform_table_size = 256;
	uint32_t transform[transform_table_size];

	uint32_t opt_var;
	void setOpt(uint32_t n) {
		opt_var = n;
	}

	forceinline uint32_t& trans(char c) {
		uint32_t index = (uint32_t)(uint8_t)c;
		check(index < transform_table_size);
		return transform[index];
	}

	WordModel() : opt_var(0) { 
	}

	void init() {
		uint32_t index = 0;
		for (auto& t : transform) t = transform_table_size;
		for (uint32_t i = 'a'; i <= 'z'; ++i) {
			transform[i] = index++;
		}
		for (uint32_t i = 'A'; i <= 'Z'; ++i) {
			transform[i] = transform[makeLowerCase(static_cast<int>(i))];
		}
#if 0
		for (uint32_t i = '0'; i <= '9'; ++i) {
			transform[i] = index++;
		}
#endif
		// 34 38
		trans('"') = index++;
		trans('&') = index++;
		trans('<') = index++;
		trans('{') = index++;
		trans(0xC0) = trans(0xE0) = index++;
		trans(0xC1) = trans(0xE1) = index++;
		trans(0xC2) = trans(0xE2) = index++;
		trans(0xC3) = trans(0xE3) = index++;
		trans(0xC4) = trans(0xE4) = index++;
		trans(0xC5) = trans(0xE5) = index++;
		trans(0xC6) = trans(0xE6) = index++;

		trans(0xC7) = trans(0xE7) = index++;

		trans(0xC8) = trans(0xE8) = index++;
		trans(0xC9) = trans(0xE9) = index++;
		trans(0xCA) = trans(0xEA) = index++;
		trans(0xCB) = trans(0xEB) = index++;

		trans(0xCC) = trans(0xEC) = index++;
		trans(0xCD) = trans(0xED) = index++;
		trans(0xCE) = trans(0xEE) = index++;
		trans(0xCF) = trans(0xEF) = index++;

		trans(0xC8) = trans(0xE8) = index++;
		trans(0xC9) = trans(0xE9) = index++;
		trans(0xCA) = trans(0xEA) = index++;
		trans(0xCB) = trans(0xEC) = index++;

		trans(0xD0) = index++;
		trans(0xF0) = index++;
		trans(0xD1) = trans(0xF1) = index++;

		trans(0xD2) = trans(0xF2) = index++;
		trans(0xD3) = trans(0xF3) = index++;
		trans(0xD4) = trans(0xF4) = index++;
		trans(0xD6) = trans(0xF6) = index++;
		trans(0xD5) = trans(0xF5) = index++;

		trans(0xD2) = trans(0xF2) = index++;
		trans(0xD3) = trans(0xF3) = index++;
		trans(0xD4) = trans(0xF4) = index++;
		trans(0xD6) = trans(0xF6) = index++;
			
		trans(0xD9) = trans(0xF9) = index++;
		trans(0xDA) = trans(0xFA) = index++;
		trans(0xDB) = trans(0xFB) = index++;
		trans(0xDC) = trans(0xFC) = index++;

		if (true) {
			for (size_t i = 128; i < 256; ++i) {
				if (transform[i] == transform_table_size) {
					transform[i] = index++;
				}
			}
		}

		len = 0;
		prev = 0;
		reset();
		decoder.init();
	}

	forceinline void reset() {
		h1 = 0x1F20239A;
		h2 = 0xBE5FD47A;
		len = 0;
	}

	forceinline uint32_t getHash() const {
		return h1 * 11 + h2 * 7;
	}

	forceinline uint32_t getPrevHash() const {
		return prev;
	}

	forceinline uint32_t get01Hash() const {
		return getHash() ^ getPrevHash();
	}

	forceinline size_t getLength() const {
		return len;
	}

	void update(uint8_t c) {
		const auto cur = transform[c];
		if (LIKELY(cur != transform_table_size)) {
			h1 = hashFunc(cur, h1);
			h2 = h1 * 4;
			len += len < kMaxLen;
		} else if (len) {
			prev = rotate_left(getHash(), 21);
			reset();
		}
	}

	forceinline uint32_t hashFunc(uint32_t c, uint32_t h) {
		h *= 61;
		h += c;
		h += rotate_left(h, 10);
		return h ^ (h >> 8);
	}

	void updateUTF(char c) {
		decoder.update(c);
		uint32_t cur = decoder.getAcc();
		if (decoder.done()) {
			if (cur < 256) cur = transform[cur];
			if (LIKELY(cur != transform_table_size)) {
				h1 = hashFunc(cur, h1);
				h2 = h1 * 4;
				len += len < kMaxLen;
			} else if (len) {
				prev = rotate_left(getHash(), 21);
				reset();
			}
		} else {
			h2 = hashFunc(cur, h2);
		}
	}
};

#endif