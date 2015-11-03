#ifndef _MATCH_MODEL_HPP_
#define _MATCH_MODEL_HPP_

#include "Memory.hpp"

template <typename Model>
class MatchModel {
public:
	static const size_t kMinMatch = 4; // TODO: Tweak this??????
	static const size_t kSmallMatch = 8;
	static const size_t kMaxCtx = 256U;
	static const size_t kMMShift = 2;  // WTF DIS 4.
	static const size_t kMaxValue = 1 << 12;
	static const size_t kMaxCtxLen = 32;
	static const bool kMultiMatch = false;
	static const bool kExtendMatch = false;
private:
	static const size_t kBitsPerChar = 16;
	std::vector<Model> models;
	Model* model_base; // Bits expected 0 x 8, 1 x 8.

	// Current minimum match
	size_t cur_min_match;
	size_t cur_max_match;
	size_t dist;

	// Current match.
	size_t pos, len;

	// Hash
	uint32_t hash_;
	
	size_t num_length_models_;

	// Hash table
	size_t hash_mask;
	MemMap hash_storage;
	uint32_t* hash_table;
	Model* cur_mdl;
	uint32_t expected_code;
	static const uint32_t kCodeBitShift = sizeof(uint32_t) * 8 - 1;
public:
	typedef CyclicBuffer<uint8_t> Buffer;
	uint32_t opt_var;

	MatchModel() : opt_var(0) {
	}

	void setOpt(uint32_t var) {
		opt_var = var;
	}

	void resize(size_t size) {
		hash_mask = size - 1;
		// Check power of 2.
		assert((hash_mask & (hash_mask + 1)) == 0);
		hash_storage.resize((hash_mask + 1) * sizeof(uint32_t));
		hash_table = (uint32_t*)hash_storage.getData();
	}

	forceinline int getP(const short* st, size_t expected_bit) {
		dcheck(len != 0);
		return st[cur_mdl[expected_bit].getP()];
	}

	forceinline size_t getPos() const {
		return pos;
	}

	forceinline uint32_t getExpectedBit() const {
		return expected_code >> kCodeBitShift;
	}

	forceinline size_t getMinMatch() const {
		return kMinMatch;
	}

	void init(size_t min_match, size_t max_match) {
		hash_ = 0;
		cur_max_match = max_match;
		num_length_models_ = (cur_max_match + 1) * 2;
		models.resize(kMaxCtx * num_length_models_);
		cur_min_match = min_match;
		expected_code = 0;
		pos = len = dist = 0;
		for (auto& m : models) m.init();
		for (size_t c = 0; c < kMaxCtx; ++c) {
			setCtx(c);
			for (size_t i = 0; i < num_length_models_; ++i) {
				const size_t len = kMinMatch + 4 + i / 2;
				if ((i & 1) != 0) {
					model_base[i].setP(kMaxValue - 1 - (kMaxValue / 2) / len); 
				} else {
					model_base[i].setP((kMaxValue / 2) / len); 
				}
			}
		}
		setCtx(0);
		updateCurMdl();
	}

	forceinline size_t getLength() const {
		return len;
	}

	forceinline void resetMatch() {
		len = 0;
	}

	forceinline void setCtx(size_t ctx) {
		model_base = &models[ctx * num_length_models_];
	}

	void search(Buffer& buffer, size_t spos) {
		// Reverse match.
		size_t blast = buffer.getPos() - 1;
		size_t len = sizeof(uint32_t);
		if (*reinterpret_cast<uint32_t*>(&buffer[spos - len]) ==
			*reinterpret_cast<uint32_t*>(&buffer[blast - len])) {
			--spos;
			--blast;
			if (kExtendMatch) {
				for (; len < cur_min_match; ++len) {
					if (buffer[spos - len] != buffer[blast - len]) {
						return;
					}
				}
				for (;buffer[spos - len] == buffer[blast - len] && len < cur_max_match; ++len);
			}
			// Update our match.
			const size_t bmask = buffer.getMask();
			dist = (blast & bmask) - (spos & bmask);
			this->pos = spos + 1;
			this->len = len;
		}
	}

	void fetch(uint32_t ctx) {
		prefetch(&hash_table[(hash_ ^ ctx) & hash_mask]);
	}

	void update(Buffer& buffer) {
		const auto blast = buffer.getPos() - 1;
		const auto bmask = buffer.getMask();
		hash_ = hash_ ^ buffer[blast];
		const auto last_pos = blast & bmask;
		const auto hmask = hash_ & ~bmask;
		// Update the existing match.
		auto& b1 = hash_table[hash_ & hash_mask];
		if (len) {
			len += len < cur_max_match;
			++pos;
		} else {
			if ((b1 & ~bmask) == hmask) {
				search(buffer, b1);
			}
		}
		updateCurMdl();
		b1 = last_pos | hmask;
	}

	uint32_t getHash() const {
		return hash_;
	}

	void setHash(uint32_t h) {
		hash_ = h;
	}

	forceinline void updateCurMdl() {
		if (len) {
			cur_mdl = model_base + 2 * std::min(len - kMinMatch, cur_max_match);
		}
	}

	forceinline uint32_t getExpectedChar(Buffer& buffer) const {
		return buffer[pos + 1];
	}

	void updateExpectedCode(uint32_t code, uint32_t bit_len = 8) {
		expected_code = code << (kCodeBitShift + 1 - bit_len);
	}

	forceinline void updateCurMdl(size_t expected_bit, uint32_t bit) {
		cur_mdl[expected_bit].update(bit);
	}

	forceinline void updateBit(uint32_t bit) {
		if (len) {
			uint32_t diff = (expected_code >> kCodeBitShift) ^ bit;
			cur_mdl[getExpectedBit()].update(bit);
			len &= -(1 ^ diff);
			expected_code <<= 1;
		}
	}
};

#endif
