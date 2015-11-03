/*	MCM file compressor

	Copyright (C) 2015, Google Inc.
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

#ifndef _SSE_HPP_
#define _SSE_HPP_

#include "Model.hpp"

// template <size_t kProbBits, size_t kStemBits = 5, class StationaryModel = fastBitModel<int, kProbBits, 8, 30>>
template <size_t kProbBits, size_t kStemBits = 5, class StationaryModel = bitLearnModel<kProbBits, 8, 30>>
class SSE {
	static const size_t kStems = (1 << kStemBits) + 1;
	static const size_t kMaxP = 1 << kProbBits;
	static const size_t kProbMask = (1 << (kProbBits - kStemBits)) - 1;
	static const size_t kRound = 1 << (kProbBits - kStemBits - 1);
	size_t pw;
	size_t opt;
	size_t count;
public:
	std::vector<StationaryModel> models;

	SSE() : opt(0) {
	}

	void setOpt(size_t var) {
		opt = var;
	}

	template <typename Table>
	void init(size_t num_ctx, const Table* table) {
		pw = count = 0;

		check(num_ctx > 0);
		models.resize(num_ctx * kStems);
		for (size_t i = 0; i < kStems; ++i) {
			auto& m = models[i];
			int p = std::min(static_cast<uint32_t>(i << (kProbBits - kStemBits)), static_cast<uint32_t>(kMaxP)); 
			m.init(table != nullptr ? table->sq(p - 2048) : p);
		}
		size_t ctx = 1;
		while (ctx * 2 <= num_ctx) {
			std::copy(&models[0], &models[kStems * ctx], &models[kStems * ctx]);
			ctx *= 2;
		}
		std::copy(&models[0], &models[kStems * (num_ctx - ctx)], &models[ctx * kStems]);
	}

	forceinline int p(size_t p, size_t ctx) {
		dcheck(p < kMaxP);
		const size_t idx = p >> (kProbBits - kStemBits);
		dcheck(idx < kStems);
		const size_t s1 = ctx * kStems + idx;
		const size_t mask = p & kProbMask;
		pw = s1 + (mask >> (kProbBits - kStemBits - 1));
		return (models[s1].getP() * (1 + kProbMask - mask) + models[s1 + 1].getP() * mask) >> (kProbBits - kStemBits);
	}

	forceinline void update(size_t bit) {
#if 0
		// 4 bits to 9 bits.
		const size_t delta1 = 4 * KB * opt;
		const size_t delta2 = delta1 + 0x10 * KB;
		const size_t delta3 = delta2 + 0x100 * KB;
		const size_t delta4 = delta3 + 0x1000 * KB;
		const size_t delta5 = delta4 + 0x10000 * KB;
		const size_t update = 3 +
			(count > delta1) + 
			(count > delta2) + 
			(count > delta3) +
			(count > delta4) +
			(count > delta5);
		count++;
		models[pw].update(bit, update);
#endif
		models[pw].update(bit);
	}
};

template <size_t kProbBits, size_t kStemBits = 5, class StationaryModel = fastBitModel<int, kProbBits, 8, 30>>
class FastSSE {
	static const size_t kStems = 1 << kStemBits;
	static const size_t kMaxP = 1 << kProbBits;
	size_t pw;
	size_t opt;
public:
	std::vector<StationaryModel> models;

	FastSSE() : pw(0), opt(0) {
	}

	void setOpt(size_t var) {
		opt = var;
	}

	template <typename Table>
	void init(size_t num_ctx, const Table* table) {
		check(num_ctx > 0);
		models.resize(num_ctx * kStems);
		for (size_t i = 0; i < kStems; ++i) {
			auto& m = models[i];
			int p = static_cast<uint32_t>(i << (kProbBits - kStemBits)) +
				static_cast<uint32_t>(1U << (kProbBits - kStemBits - 1));
			m.init(table != nullptr ? table->sq(p - 2048) : p);
		}
		size_t ctx = 1;
		while (ctx * 2 <= num_ctx) {
			std::copy(&models[0], &models[kStems * ctx], &models[kStems * ctx]);
			ctx *= 2;
		}
		std::copy(&models[0], &models[kStems * (num_ctx - ctx)], &models[ctx * kStems]);
	}

	forceinline int p(size_t p, size_t ctx) {
		dcheck(p < kMaxP);
		const size_t idx = p >> (kProbBits - kStemBits);
		dcheck(idx < kStems);
		return models[pw = ctx * kStems + idx].getP();
	}

	forceinline void update(size_t bit) {
		models[pw].update(bit);
	}
};

#endif
