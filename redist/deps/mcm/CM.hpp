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

#ifndef _CM_HPP_
#define _CM_HPP_

#include <cstdlib>
#include <vector>
#include "Detector.hpp"
#include "DivTable.hpp"
#include "Entropy.hpp"
#include "Huffman.hpp"
#include "Log.hpp"
#include "MatchModel.hpp"
#include "Memory.hpp"
#include "Mixer.hpp"
#include "Model.hpp"
#include "Range.hpp"
#include "StateMap.hpp"
#include "Util.hpp"
#include "WordModel.hpp"
#include "SSE.hpp"

// Options.
#define USE_MMX 0

// Map from state -> probability.
class ProbMap {
public:
	class Prob {
	public:
		Prob() : p_(2048), stp_(0) {
			count_[0] = count_[1] = 0;
		}
		forceinline int16_t getSTP() {
			return stp_;
		}
		forceinline int16_t getP() {
			return p_;
		}
		forceinline void addCount(size_t bit) {
			++count_[bit];
		}
		template <typename Table>
		void update(const Table& table, size_t min_count = 10) {
			const size_t count = count_[0] + count_[1];
			if (count > min_count) {
				setP(table, 4096U * static_cast<uint32_t>(count_[1]) / count);
				count_[0] = count_[1] = 0;
			}
		}
		template <typename Table>
		void setP(const Table& table, uint16_t p) {
			p_ = p;
			stp_ = table.st(p_);
		}

	private:
		uint16_t count_[2];
		uint16_t p_;
		int16_t stp_;
	};

	Prob& getProb(size_t st) {
		return probs_[st];
	}

private:
	Prob probs_[256];
};

enum CMType {
	kCMTypeTurbo,
	kCMTypeFast,
	kCMTypeMid,
	kCMTypeHigh,
	kCMTypeMax,
};

enum CMProfile {
	kProfileText,
	kProfileBinary,
	kProfileCount,
};

std::ostream& operator << (std::ostream& sout, const CMProfile& pattern);

template <CMType kCMType = kCMTypeHigh>
class CM : public Compressor {
public:
	static const size_t inputs =
		kCMType == kCMTypeTurbo ? 3 : 
		kCMType == kCMTypeFast ? 4 : 
		kCMType == kCMTypeMid ? 6 : 
		kCMType == kCMTypeHigh ? 8 : 
		kCMType == kCMTypeMax ? 10 :
		0;
	
	// Flags
	static const bool kStatistics = false;
	static const bool kFastStats = true;
	static const bool kFixedProbs = false;
	// Currently, LZP isn't as great as it coul be.
	static const bool kUseLZP = true;
	static const bool kUseSSE = true;
	static const bool kUseSSEOnlyLZP = true;
	// Prefetching related flags.
	static const bool kUsePrefetch = false;
	static const bool kPrefetchMatchModel = true;
	static const bool kPrefetchWordModel = true;
	static const bool kFixedMatchProbs = false;

	// SS table
	static const uint32_t kShift = 12;
	static const int kMaxValue = 1 << kShift;
	static const int kMinST = -kMaxValue / 2;
	static const int kMaxST = kMaxValue / 2;
	typedef ss_table<short, kMaxValue, kMinST, kMaxST, 8> SSTable;
	SSTable table;
	
	typedef safeBitModel<unsigned short, kShift, 5, 15> BitModel;
	typedef fastBitModel<int, kShift, 9, 30> StationaryModel;
	typedef fastBitModel<int, kShift, 9, 30> HPStationaryModel;

	// Word model
	WordModel word_model;
	size_t word_model_ctx_map_[WordModel::kMaxLen + 1];

	Range7 ent;
	
	typedef MatchModel<HPStationaryModel> MatchModelType;
	MatchModelType match_model;
	size_t match_model_order_;
	std::vector<int> fixed_match_probs_;

	// Hash table
	size_t hash_mask;
	size_t hash_alloc_size;
	MemMap hash_storage;
	uint8_t *hash_table;

	// If LZP, need extra bit for the 256 ^ o0 ctx
	static const uint32_t o0size = 0x100 * (kUseLZP ? 2 : 1);
	static const uint32_t o1size = o0size * 0x100;
	static const uint32_t o2size = o0size * 0x100 * 0x100;

	// o0, o1, o2, s1, s2, s3, s4
	static const size_t o0pos = 0;
	static const size_t o1pos = o0pos + o0size; // Order 1 == sparse 1
	static const size_t o2pos = o1pos + o1size;
	static const size_t s2pos = o2pos + o2size; // Sparse 2
	static const size_t s3pos = s2pos + o1size; // Sparse 3
	static const size_t s4pos = s3pos + o1size; // Sparse 4
	static const size_t hashStart = s4pos + o1size;

	// Maps from char to 4 bit identifier.
	uint8_t* current_mask_map_;
	uint8_t binary_mask_map_[256];
	uint8_t text_mask_map_[256];

	// Mixers
	uint32_t mixer_mask;
#if USE_MMX
	typedef MMXMixer<inputs, 15, 1> CMMixer;
#else
	typedef Mixer<int, inputs+1, 17, 11> CMMixer;
#endif
	std::vector<CMMixer> mixers;
	CMMixer *mixer_base;
	CMMixer *cur_profile_mixers_;

	// Contexts
	uint32_t owhash; // Order of sizeof(uint32_t) hash.

	// Rotating buffer.
	CyclicBuffer<byte> buffer;

	// Options
	uint32_t opt_var;

	// CM state table.
	static const uint32_t num_states = 256;
	uint8_t state_trans[num_states][2];

	// Huffman preprocessing.
	static const bool use_huffman = false;
	static const uint32_t huffman_len_limit = 16;
	Huffman huff;
	
	// If force profile is true then we dont use a detector.
	bool force_profile_;
	// Current profile.
	CMProfile profile_;
	
	// Mask model.
	uint32_t mask_model_;

	// LZP
	std::vector<CMMixer> lzp_mixers;
	bool lzp_enabled_;
	static const size_t kMaxMatch = 100;
	StationaryModel lzp_mdl_[kMaxMatch];
	size_t min_match_lzp_;

	// SM
	typedef StationaryModel PredModel;
	static const size_t kProbCtx = inputs;
	typedef PredModel PredArray[kProbCtx][256];
	static const size_t kPredArrays = static_cast<size_t>(kProfileCount) + 1;
	PredArray preds[kPredArrays];
	PredArray* cur_preds;

	// SSE
	SSE<kShift> sse;
	size_t sse_ctx;
	SSE<kShift> sse2;
	size_t mixer_sse_ctx;

	// Memory usage
	size_t mem_usage;

	// Fast mode. TODO split this in another compressor?
	// Quickly create a probability from a 2d array.
	HPStationaryModel fast_mix_[256][256];

	// Flags for which models are enabled.
	enum Model {
		kModelOrder0,
		kModelOrder1,
		kModelOrder2,
		kModelOrder3,
		kModelOrder4,

		kModelOrder5,
		kModelOrder6,
		kModelOrder7,
		kModelOrder8,
		kModelOrder9,
		
		kModelOrder10,
		kModelOrder11,
		kModelOrder12,
		kModelSparse2,
		kModelSparse3,
		
		kModelSparse4,
		kModelSparse23,
		kModelSparse34,
		kModelWord1,
		kModelWord2,

		kModelWordMask,
		kModelWord12,
		kModelMask,
		kModelMatchHashE,
		kModelCount,
	};
	static_assert(kModelCount <= 32, "no room in word");
	static const size_t kMaxOrder = 12;
	uint32_t enabled_models_;
	size_t max_order_;

	// Statistics
	uint64_t mixer_skip[2];
	uint64_t match_count_, non_match_count_, other_count_;
	uint64_t lzp_bit_match_bytes_, lzp_bit_miss_bytes_, lzp_miss_bytes_, normal_bytes_;
	uint64_t match_hits_[kMaxMatch], match_miss_[kMaxMatch];

	static const uint64_t kMaxMiss = 512;
	uint64_t miss_len_;
	uint64_t miss_count_[kMaxMiss];
	uint64_t fast_bytes_;
	uint64_t miss_fast_path_;

	// TODO: Get rid of this.
	static const uint32_t eof_char = 126;
	
	forceinline uint32_t hash_lookup(hash_t hash, bool prefetch_addr = kUsePrefetch) {
		hash &= hash_mask;
		uint32_t ret = hash + hashStart;
		if (prefetch_addr) {	
			prefetch(hash_table + ret);
		}
		return ret;
	}

	void setMemUsage(uint32_t usage) {
		mem_usage = usage;
	}

	CM(uint32_t mem = 8, bool lzp_enabled = kUseLZP, Detector::Profile profile = Detector::kProfileDetect)
		: mem_usage(mem), opt_var(0), lzp_enabled_(lzp_enabled) {
		force_profile_ = profile != Detector::kProfileDetect;
		if (force_profile_) {
			profile_  = profileForDetectorProfile(profile);
		} else {
			profile_ = kProfileBinary;
		}
	}

	bool setOpt(uint32_t var) {
		// if (var >= 7 && var < 13) return false;
		opt_var = var;
		word_model.setOpt(var);
		match_model.setOpt(var);
		sse.setOpt(var);
		return true;
	}

	forceinline bool modelEnabled(Model model) const {
		return (enabled_models_ & (1U << static_cast<uint32_t>(model))) != 0;
	}
	forceinline void setEnabledModels(uint32_t models) {
		enabled_models_ = models;
		calculateMaxOrder();
	}
	forceinline void enableModel(Model model) {
		enabled_models_ |= 1 << static_cast<size_t>(model);
		calculateMaxOrder();
	}

	// This is sort of expensive, don't call often.
	void calculateMaxOrder() {
		max_order_ = 0;
		for (size_t order = 0; order <= kMaxOrder; ++order) {
			if (modelEnabled(static_cast<Model>(kModelOrder0 + order))) {
				max_order_ = order;
			}
		}
		max_order_ = std::max(match_model_order_, max_order_);
	}

	void setMatchModelOrder(size_t order) {
		match_model_order_ = order ? order - 1 : 0;
		calculateMaxOrder();
	}
	
	void init();

	forceinline uint32_t hashFunc(uint32_t a, uint32_t b) const {
		b += a;
		b += rotate_left(b, 11);
		return b ^ (b >> 6);
	}

	forceinline CMMixer* getProfileMixers(CMProfile profile) {
		if (force_profile_) {
			return &mixers[0];
		}
		return &mixers[static_cast<uint32_t>(profile) * (mixer_mask + 1)];
	}

	void calcMixerBase() {
		uint32_t mixer_ctx = current_mask_map_[owhash & 0xFF];
		const bool word_enabled = modelEnabled(kModelWord1);
		if (word_enabled) {
			mixer_ctx <<= 3;
			mixer_ctx |= word_model_ctx_map_[word_model.getLength()];
		}
		
		const size_t mm_len = match_model.getLength();
		mixer_ctx <<= 2;
		if (mm_len != 0) {
			if (word_enabled) {
				mixer_ctx |= 1 +
					(mm_len >= match_model.kMinMatch + 2) + 
					(mm_len >= match_model.kMinMatch + 6) + 
					0;
			} else {
				mixer_ctx |= 1 +
					(mm_len >= match_model.kMinMatch + 1) + 
					(mm_len >= match_model.kMinMatch + 4) +
					0;
			}
		}		
		mixer_base = cur_profile_mixers_ + (mixer_ctx << 8);
	}

	void calcProbBase() {
		cur_preds = &preds[static_cast<size_t>(profile_)];
	}

	forceinline byte nextState(uint8_t state, uint32_t bit, uint32_t ctx) {
		if (!kFixedProbs) {
			(*cur_preds)[ctx][state].update(bit, 9);
		}
		return state_trans[state][bit];
	}
	
	forceinline int getP(uint8_t state, uint32_t ctx) const {
		int p = (*cur_preds)[ctx][state].getP();
		if (!kFixedProbs) {
			p = table.st(p);
		}
		return p;
	}

	enum BitType {
		kBitTypeLZP,
		kBitTypeMatch,
		kBitTypeNormal,
	};

	template <const bool decode, BitType kBitType, typename TStream>
	size_t processBit(TStream& stream, size_t bit, size_t* base_contexts, size_t ctx, size_t mixer_ctx) {
		const auto mm_l = match_model.getLength();
	
		uint8_t 
			*no_alias sp0 = nullptr, *no_alias sp1 = nullptr, *no_alias sp2 = nullptr, *no_alias sp3 = nullptr, *no_alias sp4 = nullptr,
			*no_alias sp5 = nullptr, *no_alias sp6 = nullptr, *no_alias sp7 = nullptr, *no_alias sp8 = nullptr, *no_alias sp9 = nullptr;
		uint8_t s0 = 0, s1 = 0, s2 = 0, s3 = 0, s4 = 0, s5 = 0, s6 = 0, s7 = 0, s8 = 0, s9 = 0;

		uint32_t p;
		int p0 = 0, p1 = 0, p2 = 0, p3 = 0, p4 = 0, p5 = 0, p6 = 0, p7 = 0, p8 = 0, p9 = 0;
		if (kBitType == kBitTypeLZP) {
			if (inputs > 0) {
				if (kFixedMatchProbs) {
					p0 = fixed_match_probs_[mm_l * 2 + 1];
				} else {
					p0 = match_model.getP(table.getStretchPtr(), 1);
				}
			}
		} else if (mm_l == 0) {
			if (inputs > 0) {
				sp0 = &hash_table[base_contexts[0] ^ ctx];
				s0 = *sp0;
				p0 = getP(s0, 0);
			}
			if (false && kBitType != kBitTypeLZP && profile_ == kProfileBinary) {
				p0 += getP(hash_table[s2pos + (owhash & 0xFF00) + ctx], 0);
				p0 += getP(hash_table[s3pos + ((owhash & 0xFF0000) >> 8) + ctx], 0);
				p0 += getP(hash_table[s4pos + ((owhash & 0xFF000000) >> 16) + ctx], 0);
			}
		} else {
			if (inputs > 0) {
				if (kFixedMatchProbs) {
					p0 = fixed_match_probs_[mm_l * 2 + match_model.getExpectedBit()];
				} else {
					p0 = match_model.getP(table.getStretchPtr(), match_model.getExpectedBit());
				}
			}
		}
		if (inputs > 1) {
			sp1 = &hash_table[base_contexts[1] ^ ctx];
			s1 = *sp1;
			p1 = getP(s1, 1);
		}
		if (inputs > 2) {
			sp2 = &hash_table[base_contexts[2] ^ ctx];
			s2 = *sp2;
			p2 = getP(s2, 2);
		}
		if (inputs > 3) {
			sp3 = &hash_table[base_contexts[3] ^ ctx];
			s3 = *sp3;
			p3 = getP(s3, 3);
		}
		if (inputs > 4) {
			sp4 = &hash_table[base_contexts[4] ^ ctx];
			s4 = *sp4;
			p4 = getP(s4, 4);
		}
		if (inputs > 5) {
			sp5 = &hash_table[base_contexts[5] ^ ctx];
			s5 = *sp5;
			p5 = getP(s5, 5);
		}
		if (inputs > 6) {
			sp6 = &hash_table[base_contexts[6] ^ ctx];
			s6 = *sp6;
			p6 = getP(s6, 6);
		}
		if (inputs > 7) {
			sp7 = &hash_table[base_contexts[7] ^ ctx];
			s7 = *sp7;
			p7 = getP(s7, 7);
		}
		if (inputs > 8) {
			sp8 = &hash_table[base_contexts[8] ^ ctx];
			s8 = *sp8;
			p8 = getP(s8, 8);
		}
		if (inputs > 9) {
			sp9 = &hash_table[base_contexts[9] ^ ctx];
			s9 = *sp9;
			p9 = getP(s9, 9);
		}
		auto* const cur_mixer = &mixer_base[mixer_ctx];
		int stp = cur_mixer->p(9, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);
		if (stp < kMinST) stp = kMinST;
		if (stp >= kMaxST) stp = kMaxST - 1;
		int mixer_p = table.squnsafe(stp); // Mix probabilities.
		p = mixer_p;
		if (kUseSSE) {
			if (kUseLZP) {
				if (!kUseSSEOnlyLZP || kBitType == kBitTypeLZP || sse_ctx != 0) {
					if (kBitType == kBitTypeLZP) {
						// p = sse2.p(stp + kMaxValue / 2, sse_ctx + mm_l);
						p = sse2.p(stp + kMaxValue / 2, sse_ctx + mm_l);
					} else {
						p = sse.p(stp + kMaxValue / 2, sse_ctx + mixer_ctx);
					}
					p += p == 0;
				} else if (false) {
					// This SSE is disabled for speed.
					p = (p + sse.p(stp + kMaxValue / 2, (owhash & 0xFF) * 256 + mixer_ctx)) / 2;
					p += p == 0;
				}
			} else if (false) {
				p = (p + sse.p(stp + kMaxValue / 2, (owhash & 0xFF) * 256 + mixer_ctx)) / 2;
				p += p == 0;
			}
		}

		if (decode) { 
			bit = ent.getDecodedBit(p, kShift);
		}
		dcheck(bit < 2);

		// Returns false if we skipped the update due to a low error, should happen moderately frequently on highly compressible files.
		const bool ret = cur_mixer->update(mixer_p, bit, kShift, 28, 1, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);
		// Only update the states / predictions if the mixer was far enough from the bounds, helps 15k on enwik8 and 1-2sec.
		if (ret) {
			if (kBitType == kBitTypeLZP) {
				match_model.updateCurMdl(1, bit);
			} else if (mm_l == 0) {
				if (inputs > 0) *sp0 = nextState(s0, bit, 0);
				if (false && kBitType != kBitTypeLZP && profile_ == kProfileBinary) {
					hash_table[s2pos + (owhash & 0xFF00) + ctx] = nextState(hash_table[s2pos + (owhash & 0xFF00) + ctx], bit, 0);
					hash_table[s3pos + ((owhash & 0xFF0000) >> 8) + ctx] = nextState(hash_table[s3pos + ((owhash & 0xFF0000) >> 8) + ctx], bit, 0);
					hash_table[s4pos + ((owhash & 0xFF000000) >> 16) + ctx] = nextState(hash_table[s4pos + ((owhash & 0xFF000000) >> 16) + ctx], bit, 0);
				}
			}
			if (inputs > 1) *sp1 = nextState(s1, bit, 1);
			if (inputs > 2) *sp2 = nextState(s2, bit, 2);
			if (inputs > 3) *sp3 = nextState(s3, bit, 3);
			if (inputs > 4) *sp4 = nextState(s4, bit, 4);
			if (inputs > 5) *sp5 = nextState(s5, bit, 5);
			if (inputs > 6) *sp6 = nextState(s6, bit, 6);
			if (inputs > 7) *sp7 = nextState(s7, bit, 7);
			if (inputs > 8) *sp8 = nextState(s8, bit, 8);
			if (inputs > 9) *sp9 = nextState(s9, bit, 9);
		}
		if (kUseSSE) {
			if (kUseLZP) {
				if (kBitType == kBitTypeLZP) {
					sse2.update(bit);
				} else if (!kUseSSEOnlyLZP || sse_ctx != 0) {
					sse.update(bit);
				} else {
					// sse.update(bit);
				}
			} else {
				// mixer_sse_ctx = mixer_sse_ctx * 2 + ret;
				// sse.update(bit);
			}
		}
		if (kBitType != kBitTypeLZP) {
			match_model.updateBit(bit);
		}
		if (kStatistics) ++mixer_skip[ret];

		if (decode) {
			ent.Normalize(stream);
		} else {
			ent.encode(stream, bit, p, kShift);
		}
		return bit;
	}

	template <const bool decode, typename TStream>
	size_t processNibble(TStream& stream, size_t c, size_t* base_contexts, size_t ctx_add, size_t ctx_add2) {
		uint32_t huff_state = huff.start_state, code = 0;
		if (!decode) {
			if (use_huffman) {
				const auto& huff_code = huff.getCode(c);
				code = huff_code.value << (sizeof(uint32_t) * 8 - huff_code.length);
			} else {
				code = c << (sizeof(uint32_t) * 8 - 4);
			}
		}
		size_t base_ctx = 1;
		for (;;) {
			size_t ctx;
			// Get match model prediction.
			if (use_huffman) {
				ctx = huff_state;
			} else {
				ctx = ctx_add + base_ctx;
			}
			size_t bit = 0;
			if (!decode) {
				bit = code >> (sizeof(uint32_t) * 8 - 1);
				code <<= 1;
			}
			bit = processBit<decode, kBitTypeNormal>(stream, bit, base_contexts, ctx_add2 + ctx, ctx);

			// Encode the bit / decode at the last second.
			if (use_huffman) {
				huff_state = huff.getTransition(huff_state, bit);
				if (huff.isLeaf(huff_state)) {
					return huff_state;
				}
			} else {
				base_ctx = base_ctx * 2 + bit;
				if ((base_ctx & 16) != 0) break;
			}
		}
		return base_ctx ^ 16;
	}

	template <const bool decode, typename TStream>
	size_t processByte(TStream& stream, uint32_t c = 0) {
		size_t base_contexts[inputs] = { o0pos }; // Base contexts
		auto* ctx_ptr = base_contexts;

		uint32_t random_words[] = {
			0x4ec457ce, 0x2f85195b, 0x4f4c033c, 0xc0de7294, 0x224eb711,
			0x9f358562, 0x00d46a63, 0x0fb6c35c, 0xc4dca450, 0x9ddc89f7,
			0x6f4a0403, 0x1fff619f, 0x83e56bd9, 0x0448a62c, 0x4de22c02,
			0x418700b2, 0x7e546bf8, 0xac2bb7a9, 0xc9e6cbcb, 0x4a8b4a07,
			0x486b3b68, 0x9e944172, 0xb11b7dd5, 0xaa0cd8a7, 0x4a6c6fa7,
		};

		const size_t bpos = buffer.getPos();
		const size_t blast = bpos - 1; // Last seen char
		const size_t
			p0 = static_cast<byte>(owhash >> 0),
			p1 = static_cast<byte>(owhash >> 8),
			p2 = static_cast<byte>(owhash >> 16),
			p3 = static_cast<byte>(owhash >> 24);
		if (modelEnabled(kModelOrder0)) {
			*(ctx_ptr++) = o0pos;
		}
		if (modelEnabled(kModelOrder1)) {
			*(ctx_ptr++) = o1pos + p0 * o0size;
		}
		if (modelEnabled(kModelSparse2)) {
			*(ctx_ptr++) = s2pos + p1 * o0size;
		}
		if (modelEnabled(kModelSparse3)) {
			*(ctx_ptr++) = s3pos + p2 * o0size;
		}
		if (modelEnabled(kModelSparse4)) {
			*(ctx_ptr++) = s4pos + p3 * o0size;
		}
		if (modelEnabled(kModelSparse23)) {
			*(ctx_ptr++) = hash_lookup(hashFunc(p2, hashFunc(p1, 0x37220B98))); // Order 23
		}
		if (modelEnabled(kModelSparse34)) {
			*(ctx_ptr++) = hash_lookup(hashFunc(p3, hashFunc(p2, 0x651A833E))); // Order 34
		}
		if (modelEnabled(kModelWordMask)) {
			uint32_t mask = 0xFFFFFFFF;
			mask ^= 1u << 10u;
			mask ^= 1u << 26u;
			mask ^= 1u << 24u;
			mask ^= 1u << 4u;
			mask ^= 1u << 25u;
			mask ^= 1u << 28u;
			mask ^= 1u << 5u;
			mask ^= 1u << 27u;
			mask ^= 1u << 3u;
			mask ^= 1u << 29u;
			//mask ^= 1u << opt_var;
			*(ctx_ptr++) = hash_lookup((owhash & mask) * 0xac2bb7a9 + 19299412415); // Order 34
		}
		if (modelEnabled(kModelOrder2)) {
			*(ctx_ptr++) = o2pos + (owhash & 0xFFFF) * o0size;
		}
		uint32_t h = hashFunc(owhash & 0xFFFF, 0x4ec457ce);
		uint32_t order = 3;
		size_t expected_char = 0;

		size_t mm_len = 0;
		if (match_model_order_ != 0) {
			match_model.update(buffer);
			if (mm_len = match_model.getLength()) {
				match_model.setCtx(h & 0xFF);
				match_model.updateCurMdl();
				expected_char = match_model.getExpectedChar(buffer);
				uint32_t expected_bits = use_huffman ? huff.getCode(expected_char).length : 8;
				size_t expected_code = use_huffman ? huff.getCode(expected_char).value : expected_char;
				match_model.updateExpectedCode(expected_code, expected_bits);
			}
		}

		for (; order <= max_order_; ++order) {
			h = hashFunc(buffer[bpos - order], h);
			if (modelEnabled(static_cast<Model>(kModelOrder0 + order))) {
				*(ctx_ptr++) = hash_lookup(false ? hashFunc(expected_char, h) : h, true);
			}
		}
		dcheck(order - 1 == match_model_order_);

		if (modelEnabled(kModelWord1)) {
			*(ctx_ptr++) = hash_lookup(word_model.getHash(), false); // Already prefetched.
		}
		if (modelEnabled(kModelWord2)) {
			*(ctx_ptr++) = hash_lookup(word_model.getPrevHash(), false);
		}
		if (modelEnabled(kModelWord12)) {
			*(ctx_ptr++) = hash_lookup(word_model.get01Hash(), false); // Already prefetched.
		}
		if (modelEnabled(kModelMask)) {
			// Model idea from Tangelo, thanks Jan Ondrus.
			mask_model_ *= 16;
			mask_model_ += current_mask_map_[p0];
			*(ctx_ptr++) = hash_lookup(hashFunc(0xaa0cd8a7, mask_model_ * 313), true);
		}
		if (modelEnabled(kModelMatchHashE)) {
			*(ctx_ptr++) = hash_lookup(match_model.getHash(), false);
		}
		match_model.setHash(h);
		dcheck(ctx_ptr - base_contexts <= inputs + 1);
		sse_ctx = 0;

		uint64_t cur_pos = kStatistics ? stream.tell() : 0;

		calcMixerBase();
		if (mm_len > 0) {
			miss_len_ = 0;
			if (kStatistics) {
				if (!decode) {
					++(expected_char == c ? match_count_ : non_match_count_);
				}
			}
			if (mm_len > min_match_lzp_) {
				size_t extra_len = mm_len - match_model.getMinMatch();
				cur_preds = &preds[kPredArrays - 1];
				dcheck(mm_len >= match_model.getMinMatch());
				size_t bit = decode ? 0 : expected_char == c;
				sse_ctx = 256 * (1 + expected_char);
#if 1
				// mixer_base = getProfileMixers(profile) + 256 * expected_char;
				bit = processBit<decode, kBitTypeLZP>(stream, bit, base_contexts, expected_char ^ 256, 0);
				// calcMixerBase();
#else 
				auto& mdl = lzp_mdl_[mm_len];
				int p = mdl.getP();
				p += p == 0;
				if (decode) {
					bit = ent.decode(stream, p, kShift);
				} else {
					ent.encode(stream, bit, p, kShift);
				}
				mdl.update(bit, 7);
#endif
				if (kStatistics) {
					const uint64_t after_pos = kStatistics ? stream.tell() : 0;
					(bit ? lzp_bit_match_bytes_ : lzp_bit_miss_bytes_) += after_pos - cur_pos; 
					cur_pos = after_pos;
					++(bit ? match_hits_ : match_miss_)[mm_len + match_model_order_ - 4];
				}
				if (bit) {
					return expected_char;
				}
			} 
			calcProbBase();
		} else {
			++miss_len_;
			if (kStatistics) {
				++other_count_;
				++miss_count_[std::min(kMaxMiss - 1, miss_len_ / 32)];
			}
			if (miss_len_ > miss_fast_path_) {
				if (kStatistics) ++fast_bytes_;

				if (false) {
					if (decode) {
						c = ent.DecodeDirectBits(stream, 8);
					} else {
						ent.EncodeBits(stream, c, 8);
					}
				} else {
					auto* s0 = &hash_table[o1pos + p0 * o0size];
					auto* s1 = &hash_table[o2pos + (owhash & 0xFFFF) * o0size];
					size_t ctx = 1;
					uint32_t ch = c << 24;
					while (ctx < 256) {
						auto* st0 = s0 + ctx;
						auto* st1 = s1 + ctx;
						auto* pr = &fast_mix_[*st0][*st1];
						auto p = pr->getP();
						p += p == 0;
						size_t bit;
						if (decode) {
							bit = ent.getDecodedBit(p, kShift);
							ent.Normalize(stream);
						} else {
							bit = ch >> 31;
							ent.encode(stream, bit, p, kShift);
							ch <<= 1;
						}
						pr->update(bit);
						*st0 = state_trans[*st0][bit];
						*st1 = state_trans[*st1][bit];
						ctx += ctx + bit;
					}
					if (decode) {
						c = ctx & 0xFF;
					}
				}
				return c;
			}
		}
		if (false) {
			match_model.resetMatch();
			return c;
		}
		// Non match, do normal encoding.
		size_t huff_state = 0;
		if (use_huffman) {
			huff_state = processNibble<decode>(stream, c, base_contexts, 0, 0);
			if (decode) {
				c = huff.getChar(huff_state);
			}
		} else {
			size_t n1 = processNibble<decode>(stream, c >> 4, base_contexts, 0, 0);
			if (kPrefetchMatchModel) {
				match_model.fetch(n1 << 4);
			}
			size_t n2 = processNibble<decode>(stream, c & 0xF, base_contexts, 15 + (n1 * 15), 0);
			if (decode) {
				c = n2 | (n1 << 4);
			}
			if (kStatistics) {
				(sse_ctx != 0 ? lzp_miss_bytes_ : normal_bytes_) += stream.tell() - cur_pos;
			}
		}

		return c;
	}

	static CMProfile profileForDetectorProfile(Detector::Profile profile) {
		if (profile == Detector::kProfileText) {
			return kProfileText;
		}
		return kProfileBinary;
	}

	void setDataProfile(CMProfile new_profile) {
		profile_ = new_profile;
		cur_profile_mixers_ = getProfileMixers(profile_);
		mask_model_ = 0;
		word_model.reset();
		setEnabledModels(0);
		size_t idx = 0;
		switch (profile_) {
		case kProfileText: // Text data types (tuned for xml)
#if 0
			if (inputs > idx++) enableModel(kModelOrder0);
			if (inputs > idx++) enableModel(kModelOrder4);
			if (inputs > idx++) enableModel(kModelOrder8);
			if (inputs > idx++) enableModel(kModelOrder2);
			if (inputs > idx++) enableModel(kModelMask);
			if (inputs > idx++) enableModel(kModelWord);
			if (inputs > idx++) enableModel(static_cast<Model>(opt_var));
			setMatchModelOrder(10);
#elif 1
			if (inputs > idx++) enableModel(kModelOrder4);
			if (inputs > idx++) enableModel(kModelWord1);
			if (inputs > idx++) enableModel(kModelOrder6);
			if (inputs > idx++) enableModel(kModelOrder2);
			if (inputs > idx++) enableModel(kModelOrder1);
			if (inputs > idx++) enableModel(kModelMask);
			if (inputs > idx++) enableModel(kModelOrder3);
			if (inputs > idx++) enableModel(kModelOrder8);
			if (inputs > idx++) enableModel(kModelOrder0);
			if (inputs > idx++) enableModel(kModelWord12);
			// if (inputs > idx++) enableModel(static_cast<Model>(opt_var));
			setMatchModelOrder(8);
#else
			if (inputs > idx++) enableModel(kModelOrder0);
			if (inputs > idx++) enableModel(kModelOrder1);
			if (inputs > idx++) enableModel(kModelOrder2);
			if (inputs > idx++) enableModel(kModelOrder3);
			if (inputs > idx++) enableModel(kModelOrder4);
			if (inputs > idx++) enableModel(kModelOrder5);
			if (inputs > idx++) enableModel(kModelOrder6);
			if (inputs > idx++) enableModel(kModelOrder7);
			setMatchModelOrder(0);
#endif
			// if (inputs > idx++) enableModel(static_cast<Model>(opt_var));
			current_mask_map_ = text_mask_map_;
			min_match_lzp_ = lzp_enabled_ ? 9 : kMaxMatch;
			miss_fast_path_ = 1000000;
			break;
		default: // Binary
			assert(profile_ == kProfileBinary);
#if 0
			if (inputs > idx++) enableModel(kModelOrder0);
			if (inputs > idx++) enableModel(kModelOrder4);
			if (inputs > idx++) enableModel(kModelOrder2);
			if (inputs > idx++) enableModel(kModelOrder6);
			// if (inputs > idx++) enableModel(kModelOrder1);
			// if (inputs > idx++) enableModel(kModelOrder3);
			if (inputs > idx++) enableModel(static_cast<Model>(opt_var));
#elif 1
			// Default
			if (inputs > idx++) enableModel(kModelOrder1);
			if (inputs > idx++) enableModel(kModelOrder2);
			if (inputs > idx++) enableModel(kModelSparse34);
			// if (opt_var && inputs > idx++) enableModel(kModelWordMask);
			if (inputs > idx++) enableModel(kModelOrder4);
			if (inputs > idx++) enableModel(kModelSparse23);
			if (inputs > idx++) enableModel(kModelMask);
			if (inputs > idx++) enableModel(kModelSparse4);
			if (inputs > idx++) enableModel(kModelOrder3);
			if (inputs > idx++) enableModel(kModelSparse2);
			if (inputs > idx++) enableModel(kModelSparse3);
#elif 1
			// bitmap profile (rafale.bmp)
			if (inputs > idx++) enableModel(kModelOrder4);
			if (inputs > idx++) enableModel(kModelOrder2);
			if (inputs > idx++) enableModel(kModelOrder12);
			if (inputs > idx++) enableModel(kModelSparse34);
			if (inputs > idx++) enableModel(kModelOrder5);
			if (inputs > idx++) enableModel(kModelMask);
			if (inputs > idx++) enableModel(kModelOrder1);
			if (inputs > idx++) enableModel(kModelOrder7);
#elif 1
			// exe profile (acrord32.exe)
			if (inputs > idx++) enableModel(kModelOrder1);
			if (inputs > idx++) enableModel(kModelOrder2);
			if (inputs > idx++) enableModel(kModelSparse34);
			if (inputs > idx++) enableModel(kModelSparse23);
			if (inputs > idx++) enableModel(kModelMask);
			if (inputs > idx++) enableModel(kModelSparse4);
			if (inputs > idx++) enableModel(kModelOrder3);
			if (inputs > idx++) enableModel(kModelSparse2);
			if (inputs > idx++) enableModel(kModelSparse3);
			if (inputs > idx++) enableModel(kModelOrder0);
#endif
			setMatchModelOrder(6);
			current_mask_map_ = binary_mask_map_;
			min_match_lzp_ = lzp_enabled_ ? 0 : kMaxMatch;
			// miss_fast_path_ = 1500;
			miss_fast_path_ = 100000;
			break;
		}
		calcProbBase();
	};

	void update(uint32_t c) {;
		if (modelEnabled(kModelWord1) || modelEnabled(kModelWord2) || modelEnabled(kModelWord12)) {
			word_model.update(c);
			if (word_model.getLength() > 2) {
				if (kPrefetchWordModel) {
					hash_lookup(word_model.getHash(), true);
				}
			}
			if (kPrefetchWordModel && modelEnabled(kModelWord12)) {
				hash_lookup(word_model.get01Hash(), true);
			}
		}
		buffer.push(c);
		owhash = (owhash << 8) | static_cast<byte>(c);
	}

	virtual void compress(Stream* in_stream, Stream* out_stream, uint64_t max_count);
	virtual void decompress(Stream* in_stream, Stream* out_stream, uint64_t max_count);
};

#endif
