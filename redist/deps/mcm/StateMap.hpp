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

#ifndef _STATE_MAP_HPP_
#define _STATE_MAP_HPP_

#include <fstream>

#include "Compressor.hpp"

// Non stationary state map.
template <const int pshift = 12>
class NSStateMap {
	static const bool verbose = false;

	struct State {
		byte t[2]; // Next state
		int n[2];
		uint32_t tot;
		int p;
		int c0, c1;
		uint32_t np[2];
		State() {
			np[0] = np[1] = 0;
		}
	};
	static const uint32_t num_states = 256;
	State states[num_states];

	static inline uint32_t getP(uint32_t n0, uint32_t n1, uint32_t shift) {
		if (!n0 && !n1) return 1 << (shift - 1);
		return (n0 << shift) / (n0 + n1);
	}

	uint32_t find_state(uint32_t best, byte* n, uint32_t b) const {
		// How to compare?? hmmm
		uint32_t o = b^1;
		uint32_t p = getP(n[0], n[1], 16);
		for (uint32_t i = 0;i < 256;++i) {
			const State& st = states[i];
			uint32_t stp = getP(st.n[0], st.n[1], 16);
			//if (st.n[0] == n[0] && st.n[1] == n[1]) return i;
			if (st.n[b] >= n[b]) {
				int diff = std::abs(int(st.n[o])-int(n[o]));
				int bdiff = std::abs(int(states[best].n[o])-int(n[o]));
				if (diff <= bdiff) {
					if (diff < bdiff) {
						best = i;
					} else {
						// Same diff, minimize the other guy.
						diff = st.n[b] - n[b];
						bdiff = states[best].n[b] - n[b];
						if (bdiff < 0 || diff < bdiff) {
							best = i;
						}
					}
				}
			}
		}
		if (verbose) {
			std::cerr << "wanted(" << (int)n[0] << "," << (int)n[1] << "), got(" << (int)states[best].n[0] << "," << (int)states[best].n[1] << ")" << std::endl;
		}
		return best;
	}
public:
	static byte const start = 0;
	static const bool count_probs = false;
	static const uint32_t pmax = 1 << pshift;

	NSStateMap() {
		build();
	}

	State* begin() {
		return states;
	}

	State* end() {
		return states + num_states;
	}

	inline uint32_t p(uint32_t state) const {
		return states[state].p;
	}

	inline uint32_t getTransition(const uint32_t state, uint32_t bit) {
		return states[state].t[bit];
	}

	inline uint32_t next(const uint32_t state, uint32_t bit) {
		if (count_probs) {
			++states[state].np[bit];
		}
		return states[state].t[bit];
	}

	inline uint32_t total(uint32_t state) const {
		return states[state].tot;
	}

	void dump(std::ostream& sout) {
		for (uint32_t i = 0; i < num_states; ++i) {
			sout << uint32_t(pmax * double(states[i].np[0]) / double(states[i].np[0] + states[i].np[1])) << ",";
		}
		sout << std::endl;
		uint32_t unused = 0;
		for (uint32_t i = 0; i < num_states; ++i) {
			if (!states[i].np[0] && !states[i].np[1]) ++unused;
			sout << i << ": " << states[i].np[0]  << ", " << states[i].np[1] << std::endl;
		}
		sout << "Warning: " << unused << " unused states" << std::endl;
	}

	typedef std::map<size_t, size_t> StateReMap;
	typedef std::vector<std::pair<size_t, size_t> > StateVec;
	void optimizeStates(bool add_new_start_state = true) {
		StateReMap sm;
		StateVec new_states;
		if (add_new_start_state) {
			// Add an extra 0 state which you cannot return to?
			new_states.push_back(std::make_pair(uint32_t(1), uint32_t(1)));
		}
		optimizeStatesRec(0, sm, new_states);
		for (uint32_t i = 0; i < 256; ++i) {
			states[i].t[0] = states[i].t[1] = 0;
		}
		for (uint32_t i = 0; i < new_states.size(); ++i) {
			states[i].t[0] = static_cast<uint8_t>(new_states[i].first);
			states[i].t[1] = static_cast<uint8_t>(new_states[i].second);
		}
		if (true) {
			states[0].t[0] = states[1].t[0];
			states[0].t[1] = states[1].t[1];
		}
		if (false) {
			std::ofstream fstate_derp("states.txt");
			for (uint32_t i = 0;i < new_states.size();++i) {
				fstate_derp << "{" << (uint32_t)states[i].t[0] << "," << (uint32_t)states[i].t[1] << "},";
			}
		}
	}

	size_t optimizeStatesRec(uint32_t st, StateReMap& sm, StateVec& new_states) {
		auto found = sm.find(st);
		if (found == sm.end()) {
			sm.insert(std::make_pair(st, new_states.size()));
			new_states.push_back(std::make_pair(uint32_t(0), uint32_t(0)));
			size_t a = optimizeStatesRec(states[st].t[0], sm, new_states);
			size_t b = optimizeStatesRec(states[st].t[1], sm, new_states);
			new_states[sm[st]].first = a;
			new_states[sm[st]].second = b;
		}
		return sm[st];
	}

	void build() {
#if 0
		uint8_t State_table[256][2]={
			{  1,  2},{  3,  5},{  4,  6},{  7, 10},{  8, 12},{  9, 13},{ 11, 14}, // 0
			{ 15, 19},{ 16, 23},{ 17, 24},{ 18, 25},{ 20, 27},{ 21, 28},{ 22, 29}, // 7
			{ 26, 30},{ 31, 33},{ 32, 35},{ 32, 35},{ 32, 35},{ 32, 35},{ 34, 37}, // 14
			{ 34, 37},{ 34, 37},{ 34, 37},{ 34, 37},{ 34, 37},{ 36, 39},{ 36, 39}, // 21
			{ 36, 39},{ 36, 39},{ 38, 40},{ 41, 43},{ 42, 45},{ 42, 45},{ 44, 47}, // 28
			{ 44, 47},{ 46, 49},{ 46, 49},{ 48, 51},{ 48, 51},{ 50, 52},{ 53, 43}, // 35
			{ 54, 57},{ 54, 57},{ 56, 59},{ 56, 59},{ 58, 61},{ 58, 61},{ 60, 63}, // 42
			{ 60, 63},{ 62, 65},{ 62, 65},{ 50, 66},{ 67, 55},{ 68, 57},{ 68, 57}, // 49
			{ 70, 73},{ 70, 73},{ 72, 75},{ 72, 75},{ 74, 77},{ 74, 77},{ 76, 79}, // 56
			{ 76, 79},{ 62, 81},{ 62, 81},{ 64, 82},{ 83, 69},{ 84, 71},{ 84, 71}, // 63
			{ 86, 73},{ 86, 73},{ 44, 59},{ 44, 59},{ 58, 61},{ 58, 61},{ 60, 49}, // 70
			{ 94, 87},{ 94, 87},{ 96, 45},{ 96, 45},{ 48, 99},{ 48, 99},{ 88,101}, // 84
			{ 88,101},{ 80,102},{103, 69},{104, 87},{104, 87},{106, 57},{106, 57}, // 91
			{ 62,109},{ 62,109},{ 88,111},{ 88,111},{ 80,112},{113, 85},{114, 87}, // 98
			{114, 87},{116, 57},{116, 57},{ 62,119},{ 62,119},{ 88,121},{ 88,121}, // 105
			{ 90,122},{123, 85},{124, 97},{124, 97},{126, 57},{126, 57},{ 62,129}, // 112
			{ 62,129},{ 98,131},{ 98,131},{ 90,132},{133, 85},{134, 97},{134, 97}, // 119
			{136, 57},{136, 57},{ 62,139},{ 62,139},{ 98,141},{ 98,141},{ 90,142}, // 126
			{143, 95},{144, 97},{144, 97},{ 68, 57},{ 68, 57},{ 62, 81},{ 62, 81}, // 133
			{ 98,147},{ 98,147},{100,148},{149, 95},{150,107},{150,107},{108,151}, // 140
			{108,151},{100,152},{153, 95},{154,107},{108,155},{100,156},{157, 95}, // 147
			{158,107},{108,159},{100,160},{161,105},{162,107},{108,163},{110,164}, // 154
			{165,105},{166,117},{118,167},{110,168},{169,105},{170,117},{118,171}, // 161
			{110,172},{173,105},{174,117},{118,175},{110,176},{177,105},{178,117}, // 168
			{118,179},{110,180},{181,115},{182,117},{118,183},{120,184},{185,115}, // 175
			{186,127},{128,187},{120,188},{189,115},{190,127},{128,191},{120,192}, // 182
			{193,115},{194,127},{128,195},{120,196},{197,115},{198,127},{128,199}, // 189
			{120,200},{201,115},{202,127},{128,203},{120,204},{205,115},{206,127}, // 196
			{128,207},{120,208},{209,125},{210,127},{128,211},{130,212},{213,125}, // 203
			{214,137},{138,215},{130,216},{217,125},{218,137},{138,219},{130,220}, // 210
			{221,125},{222,137},{138,223},{130,224},{225,125},{226,137},{138,227}, // 217
			{130,228},{229,125},{230,137},{138,231},{130,232},{233,125},{234,137}, // 224
			{138,235},{130,236},{237,125},{238,137},{138,239},{130,240},{241,125}, // 231
			{242,137},{138,243},{130,244},{245,135},{246,137},{138,247},{140,248}, // 238
			{249,135},{250, 69},{ 80,251},{140,252},{253,135},{254, 69},{ 80,255}, // 245
			{140,252},{253,135},{254, 69},{ 80,255}};
#else
		uint8_t State_table[256][2] = {{2,12},{144,5},{7,5},{25,14},{54,5},{16,29},{26,140},{169,5},{16,24},{84,4},{16,37},{10,98},{10,212},{45,37},{201,0},{28,202},{32,23},{183,136},{6,217},{188,38},{8,37},{21,69},{8,61},{56,13},{22,37},{27,66},{85,140},{35,41},{107,199},{8,91},{102,217},{100,17},{104,23},{94,5},{96,47},{43,176},{46,48},{22,39},{107,51},{24,51},{251,248},{25,1},{54,34},{53,148},{70,58},{86,23},{22,61},{70,23},{36,63},{61,63},{7,5},{62,65},{90,216},{67,105},{68,34},{150,5},{96,73},{70,47},{72,74},{73,75},{46,63},{74,49},{20,89},{36,79},{20,138},{62,81},{170,14},{82,69},{84,34},{21,233},{96,72},{10,212},{44,59},{44,59},{58,76},{58,76},{75,61},{60,49},{22,39},{77,89},{19,65},{62,91},{90,217},{150,185},{94,34},{92,127},{96,23},{167,207},{146,147},{77,99},{93,102},{11,101},{97,105},{103,249},{104,34},{19,65},{106,57},{122,123},{22,39},{48,109},{0,203},{11,111},{3,83},{108,112},{114,34},{25,245},{116,34},{19,39},{113,230},{48,119},{166,188},{62,121},{140,96},{117,27},{124,4},{194,231},{42,34},{141,127},{164,179},{63,129},{235,204},{107,131},{125,127},{110,217},{134,4},{132,133},{18,245},{201,14},{62,175},{48,139},{128,183},{11,151},{137,30},{35,204},{135,4},{144,4},{175,149},{142,217},{62,183},{48,51},{201,1},{189,171},{145,133},{95,178},{150,33},{152,153},{95,195},{167,179},{35,66},{64,172},{154,9},{107,155},{156,157},{27,239},{158,9},{38,159},{160,161},{67,194},{162,33},{80,143},{163,185},{201,249},{166,9},{165,52},{0,231},{168,140},{110,188},{100,207},{173,176},{177,33},{174,188},{21,0},{151,192},{181,69},{27,154},{252,138},{66,33},{189,9},{228,143},{31,40},{190,90},{184,185},{188,121},{63,187},{186,148},{141,52},{193,83},{128,191},{114,182},{205,220},{197,52},{128,195},{146,212},{196,52},{100,215},{130,199},{198,133},{236,239},{206,233},{252,203},{100,203},{3,66},{146,254},{91,207},{185,194},{229,1},{209,230},{95,211},{100,17},{214,216},{100,17},{130,215},{64,225},{201,204},{218,239},{146,219},{166,230},{249,1},{222,204},{200,223},{134,1},{167,215},{226,174},{131,227},{221,247},{64,237},{232,69},{87,231},{19,65},{238,220},{35,12},{200,17},{180,173},{67,217},{28,101},{197,213},{240,241},{252,253},{242,220},{108,164},{243,244},{201,164},{246,41},{120,213},{201,52},{154,1},{126,55},{115,118},{115,118},{25,216},{38,195},{167,215},{20,138},{64,15},{28,88},{14,17},};
#endif

		for (uint32_t i = 0;i < num_states;++i) {
			State& st = states[i];
			st.n[0] = st.n[1] = 0;
			st.c0 = st.c1 = 0;
			uint32_t n0 = st.n[0];
			uint32_t n1 = st.n[1];
			st.tot = st.n[0] + st.n[1];
			if (!n0) {
				if (!n1) {
					st.p = pmax / 2;
				} else {
					st.p = 1;
				}
			} else if (!n1) {
				st.p = pmax - 1;
			} else 
				st.p = (n0 << pshift) / (n0 + n1);
		}
		uint32_t mid = num_states / 2;
		for (uint32_t i = 0;i < num_states;++i) {
			State& st = states[i];
			for (uint32_t j = 0;j < 2;++j) {
				// How we determine the next state:
				byte n[2];
				n[j] = st.n[j] + 1;
				n[j^1] = st.n[j^1] / 2 + (st.n[j^1] != 0);
				// st.t[j] = find_state(i, n, j);
				st.t[j] = State_table[i][j];
			}
			assert(st.t[0] != i || st.t[1] != i);
		}
		optimizeStates();
	}
};

#endif
