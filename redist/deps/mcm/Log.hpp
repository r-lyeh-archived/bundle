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

#ifndef _LOG_HPP_
#define _LOG_HPP_

#include <cmath>

// squash = ln(p/(1-p))
// stretch = squash^-1 = 1/(1+e^-x)

inline double squash(double p)
{
	if (p < 0.0001) return -999.0;
	if (p > 0.9999) return 999.0;
	return (double) std::log((double) p / (1.0 - (double) p));
}

inline double stretch(double p)
{
	return 1.0 / double(1.0 +	exp((double) -p));
}

inline int roundint(double p)
{
	return int(p + 0.5);
}

// Squash - stretch table
template <typename T, int denom, int minInt, int maxInt, int FP>
struct ss_table {
	static const int total = maxInt - minInt;
	T stretchTable[denom], squashTable[total], *squashPtr;
public:
	// probability = p / Denom
	void build(int delta) {
		squashPtr = &squashTable[0 - minInt];
		// From paq9a
		const size_t num_stems = 33;
		int stems[num_stems] = {
			//1,2,3,6,10,16,27,50,79,126,198,306,465,719,1072,1478,2047,
			1,2,3,6-2,10+9,16+12,27+15,50+18,79+9,126-12,198+1-8,306+5,465-4,719-26,1072+8,1478+57-10-47,2047+32-56-105+22,
			//1,2,3,6,10,16,27,45,73,120,194,310,488,747+2,1101-3,1546-8*delta,2047,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		for (int i = 17; i < num_stems; ++i) {
			stems[i] = 4096 - stems[num_stems - 1 - i];
		}
		// Interpolate between stems.
		const int stem_divisor = total / (num_stems - 1);
		for (int i = minInt; i < maxInt; ++i) {
			const int pos = i - minInt;
			const int stem_idx = pos / stem_divisor;
			const int stem_frac = pos % stem_divisor;
			squashTable[pos] = 
				(stems[stem_idx] * (stem_divisor - stem_frac) + stems[stem_idx + 1] * stem_frac + stem_divisor / 2) / stem_divisor;
		}
		int pi = 0;
		// invert squash()
		for (int x = minInt; x < maxInt; ++x) {
			int i = squashPtr[x];
			for (int j = pi; j < i; ++j) {
				stretchTable[j] = x;
			}
			pi = i;
		}
		for (int x = pi; x < total; ++x) {
			stretchTable[x] = 2047;
		}
	}

	const T* getStretchPtr() const {
		return stretchTable;
	}

	// 0 <= p < denom
	forceinline int st(uint32_t p) const {
		return stretchTable[p];
	}

	// minInt <= p < maxInt
	forceinline uint32_t sq(int p) const {
		if (p <= minInt) {
			return 1;
		}
		if (p >= maxInt) {
			return denom - 1;
		}
		return squashPtr[p];
	}
	forceinline uint32_t squnsafe(int p) const {
		dcheck(p >= minInt);
		dcheck(p < maxInt);
		return squashPtr[p];
	}
};

#endif
