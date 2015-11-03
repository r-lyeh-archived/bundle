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

#ifndef _ENTROPY_HPP_
#define _ENTROPY_HPP_

#include <cmath>

template <const uint32_t shift = 12, const uint32_t fp_shift_ = 8>
class SymbolCostTable {
	static const uint32_t pmax = 1 << shift;
	uint32_t log2_table[pmax];
public:
	static const uint32_t fp_shift = fp_shift_;
	SymbolCostTable() {
		auto factor = double(1 << fp_shift);
		for (uint32_t i = 0;i < pmax;++i) {
			log2_table[i] = int(-log(double(i) / double(pmax)) / log(2.0) * factor);
		}
	}

	inline uint32_t cost(uint32_t p, uint32_t bit) const {
		if (bit == 1) {
			p = pmax - 1 - p;
		}
		return log2_table[p];
	}
};

#endif
