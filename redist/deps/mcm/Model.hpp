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

#ifndef _MODEL_HPP_
#define _MODEL_HPP_

#include "Compressor.hpp"
#include <assert.h>
#pragma warning(disable : 4146)

#pragma pack(push)
#pragma pack(1)
template <typename T, const int max>
class floatBitModel
{
	float f;
public:
	floatBitModel()
	{
		init();
	}

	void init()
	{
		f = 0.5f;
	}

	inline void update(T bit, T dummy)
	{
		f += ((float)(bit^1) - f) * 0.02;
		if (f < 0.001) f = 0.001;
		if (f > 0.999) f = 0.999;
	}

	inline uint32_t getP() const
	{
		return (uint32_t)(f * (float)max);
	}
};

// Count stored in high bits
#pragma pack(push)
#pragma pack(1)

// Bit probability model (should be rather fast).
template <typename T, const uint32_t _shift, const uint32_t _learn_rate = 5, const uint32_t _bits = 15>
class safeBitModel {
protected:
	T p;
	static const uint32_t pmax = (1 << _bits) - 1;
public:
	static const uint32_t shift = _shift;
	static const uint32_t learn_rate = _learn_rate;
	static const uint32_t max = 1 << shift;

	void init() {
		p = pmax / 2;
	}

	safeBitModel() {
		init();
	}

	inline void update(T bit) {
		int round = 1 << (_learn_rate - 1);
		p += ((static_cast<int>(bit) << _bits) - static_cast<int>(p) + round) >> _learn_rate;
	}

	inline uint32_t getP() const {
		uint32_t ret = p >> (_bits - shift);
		ret += ret == 0;
		return ret;
	}
};

template <const uint32_t _shift, const uint32_t _learn_rate = 5, const uint32_t _bits = 15> 
class bitLearnModel{
	static const uint32_t kCountBits = 8;
	static const uint32_t kCountMask = (1 << kCountBits) - 1;
	static const uint32_t kInitialCount = 2;
	// Count is in low kCountBits.
	uint32_t p;
public:
	static const uint32_t shift = _shift;
	static const uint32_t learn_rate = _learn_rate;
	static const uint32_t max = 1 << shift;

	forceinline void init(int new_p = 1 << (_shift - 1)) {
		setP(new_p, kInitialCount << 5);
	}

	forceinline bitLearnModel() {
		init();
	}

	forceinline void update(uint32_t bit) {
		const size_t count = p & kCountMask;
		const size_t learn_rate = 2 + (count >> 5);
#if 0
		auto delta = ((static_cast<int>(bit) << 31) - static_cast<int>(p & ~kCountMask)) >> learn_rate;
		p += delta & ~kCountMask;
		p += count < kCountMask;
		p &= 0x7FFFFFFF;
#else
		const int m[2] = {kCountMask, (1u << 31) - 1};
		p = p + (((m[bit] - static_cast<int>(p)) >> learn_rate) & ~kCountMask);
		p += count < kCountMask;
#endif
	}

	forceinline uint32_t getCount() {
		return p & kCountMask;
	}

	forceinline void setP(uint32_t new_p, uint32_t count) {
		p = new_p << (31 - shift) | count;
	}

	forceinline uint32_t getP() const {
		return p >> (31 - shift);
	}
};

// Bit probability model (should be rather fast).
template <typename T, const uint32_t _shift, const uint32_t _learn_rate = 5, const uint32_t _bits = 15>
class fastBitModel {
protected:
	T p;
	static const bool kUseRounding = false;
	static const T pmax = (1 << _bits) - 1;
public:
	static const uint32_t shift = _shift;
	static const uint32_t learn_rate = _learn_rate;
	static const uint32_t max = 1 << shift;

	forceinline void init(int new_p = 1u << (_shift - 1)) {
		p = new_p << (_bits - shift);
	}
	forceinline fastBitModel() {
		init();
	}
	forceinline void update(T bit) {
		update(bit, learn_rate);
	}
	forceinline void update(T bit, int32_t learn_rate) {
		const int round = kUseRounding ? (1 << (learn_rate - 1)) : 0;
		p += ((static_cast<int>(bit) << _bits) - static_cast<int>(p) + round) >> learn_rate;
	}
	forceinline void setP(uint32_t new_p) {
		p = new_p << (_bits - shift);
	}
	forceinline uint32_t getP() const {
		return p >> (_bits - shift);
	}
};

// Bit probability model (should be rather fast).
template <typename T, const uint32_t _shift, const uint32_t _learn_rate = 5>
class fastBitSTModel {
protected:
	T p;
public:
	static const uint32_t shift = _shift;
	static const uint32_t learn_rate = _learn_rate;
	static const uint32_t max = 1 << shift;

	void init() {
		p = 0;
	}

	fastBitSTModel() {
		init();
	}

	template <typename Table>
	inline void update(T bit, Table& table) {
		return;
		// Calculate err first.
		int err = (static_cast<int>(bit) << shift) - table.sq(getSTP());
		p += err >> 10;
		const T limit = 2048 << shift; // (_bits - shift); 
		if (p < -limit) p = -limit;
		if (p > limit) p = limit;
	}

	template <typename Table>
	inline void setP(uint32_t new_p, Table& table) {
		p = table.st(new_p) << shift; 
	}

	// Return the stretched probability.
	inline uint32_t getSTP() const {
		return p + (1 << shift - 1) >> shift;
	}
};

// Bit probability model (should be rather fast).
template <typename T, const uint32_t _shift, const uint32_t _learn_rate = 5, const uint32_t _bits = 15>
class fastBitSTAModel {
protected:
	T p;
	static const uint32_t pmax = (1 << _bits) - 1;
public:
	static const uint32_t shift = _shift;
	static const uint32_t learn_rate = _learn_rate;
	static const uint32_t max = 1 << shift;

	void init() {
		p = pmax / 2;
	}

	fastBitSTAModel() {
		init();
	}

	inline void update(T bit) {
		int round = 1 << (_learn_rate - 1);
		p += ((static_cast<int>(bit) << _bits) - static_cast<int>(p) + 00) >> _learn_rate;
	}

	inline void setP(uint32_t new_p) {
		p = new_p << (_bits - shift);
	}

	inline int getSTP() const {
		return (p >> (_bits - shift)) - 2048;
	}
};

template <typename T, const uint32_t _shift, const uint32_t _learn_rate = 5>
class fastBitStretchedModel : public fastBitModel<T, _shift, _learn_rate> {
public:
	static const uint32_t shift = _shift;
	static const uint32_t learn_rate = _learn_rate;
	static const uint32_t max = 1 << shift;

	inline uint32_t getP() const {
		return getP() - (1 << (shift - 1));
	}
};

#pragma pack(pop)

// Semistationary model.
template <typename T>
class fastCountModel {
	T n[2];
public:
	inline uint32_t getN(uint32_t i) const {
		return n[i];
	}

	inline uint32_t getTotal() const {
		return n[0] + n[1];
	}

	fastCountModel() {
		init();
	}

	void init() {
		n[0] = n[1] = 0;
	}

	void update(uint32_t bit) {
		n[bit] += n[bit] < 0xFF;
		n[1 ^ bit] = n[1 ^ bit] / 2 + (n[1 ^ bit] != 0);
	}

	inline uint32_t getP() const {
		uint32_t a = getN(0);
		uint32_t b = getN(1);
		if (!a && !b) return 1 << 11;
		if (!a) return 0;
		if (!b) return (1 << 12) - 1;
		return (a << 12) / (a + b);
	}
};


template <typename Predictor, const uint32_t max>
class bitContextModel {
	static const uint32_t bits = _bitSize<max - 1>::value;
	Predictor pred[max];
public:
	void init() {
		for (auto& mdl : pred) mdl.init();
	}

	// Returns the cost of a symbol.
	template <typename CostTable>
	inline uint32_t cost(const CostTable& table, uint32_t sym, uint32_t limit = max) {
		assert(limit <= max);
		assert(sym < limit);
		uint32_t ctx = 1, total = 0;
		for (uint32_t bit = bits - 1; bit != uint32_t(-1); --bit) {
			if ((sym >> bit | 1) << bit < limit) {
				uint32_t b = (sym >> bit) & 1;
				total += table.cost(pred[ctx].getP(), b);
				ctx += ctx + b;
			}
		}
		return total;
	}

	template <typename TEnt, typename TStream>
	inline void encode(TEnt& ent, TStream& stream, uint32_t sym, uint32_t limit = max) {
		uint32_t ctx = 1;
		assert(limit <= max);
		assert(sym < limit);
		for (uint32_t bit = bits - 1; bit != uint32_t(-1); --bit)
			if ((sym >> bit | 1) << bit < limit) {
				uint32_t b = (sym >> bit) & 1;
				ent.encode(stream, b, pred[ctx].getP(), Predictor::shift);
				pred[ctx].update(b);
				ctx += ctx + b;
			}
	}

	inline void update(uint32_t sym, uint32_t limit = max) {
		uint32_t ctx = 1;
		assert(limit <= max);
		assert(sym < limit);
		for (uint32_t bit = bits - 1; bit != uint32_t(-1); --bit)
			if ((sym >> bit | 1) << bit < limit) {
				uint32_t b = (sym >> bit) & 1;
				pred[ctx].update(b);
				ctx += ctx + b;
			}
	}

	template <typename TEnt, typename TStream>
	inline uint32_t decode(TEnt& ent, TStream& stream, uint32_t limit = max) {
		uint32_t ctx = 1, sym = 0;
		assert(limit <= max);
		assert(sym < limit);
		for (uint32_t bit = bits - 1; bit != uint32_t(-1); --bit) {
			if ((sym >> bit | 1) << bit < limit) {
				uint32_t b = ent.decode(stream, pred[ctx].getP(), Predictor::shift);
				sym |= b << bit;
				pred[ctx].update(b);
				ctx += ctx + b;
			}
		}
		return sym;
	}
};

#pragma pack(pop)

#endif