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

#ifndef _HUFFMAN_HPP_
#define _HUFFMAN_HPP_

#include <algorithm>
#include <cassert>
#include <set>

#include "Compressor.hpp"
#include "ProgressMeter.hpp"
#include "Range.hpp"

class Huffman {
public:
	class Code {
	public:
		static const uint32_t nonLeaf = 0;
		uint32_t value;
		uint32_t length;

		Code() : value(0), length(nonLeaf) {
		
		}
	};

	template <typename T>
	class Tree {
	public:
		uint32_t value;
		T weight;
		Tree *a, *b;

		forceinline uint32_t getAlphabet() const {
			return value;
		}

		forceinline bool isLeaf() const {
			return a == nullptr && b == nullptr;
		}

		forceinline T getWeight() const {
			return weight;
		}

		void getCodes(Code* codes, uint32_t bits = 0, uint32_t length = 0) const {
			assert(codes != nullptr);
			if (isLeaf()) {
				codes[value].value = bits;
				codes[value].length = length;
			} else {
				a->getCodes(codes, (bits << 1) | 0,  length + 1);
				b->getCodes(codes, (bits << 1) | 1,  length + 1);
			}
		}

		void getLengths(T* lengths, uint32_t cur_len = 0) {
			if (isLeaf()) {
				lengths[value] = cur_len;
			} else {
				a->getLengths(lengths, cur_len + 1);
				b->getLengths(lengths, cur_len + 1);
			}
		}

		void calcDepth(uint32_t cur_depth = 0) {
			if (!isLeaf()) weight = 0;
			if (a != nullptr) {
				a->calcDepth(cur_depth + 1);
				weight += a->getWeight();
			}
			if (b != nullptr) {
				b->calcDepth(cur_depth + 1);
				weight += b->getWeight();
			}
		}

		uint64_t getCost(uint32_t bits = 0) const {
			if (isLeaf())
				return bits * weight;
			else
				return a->getCost(bits + 1) + b->getCost(bits + 1);
		}

		Tree(uint32_t value, T w) : value(value), weight(w), a(nullptr), b(nullptr) {

		}

		Tree(Tree* a, Tree* b)
			: value(0), weight(a->getWeight() + b->getWeight()), a(a), b(b) {
			
		}

		~Tree() {
			delete a;
			delete b;
		}

		void printRatio(const char* name) const {
			//std::cout << "Huffman tree " << name << ": " << getWeight() << " -> " << getCost() / 8 << std::endl;
		}
	};

	typedef Tree<uint32_t> HuffTree;

	class TreeComparator {
	public:
		inline bool operator()(HuffTree* a, HuffTree* b) const {
			return a->getWeight() < b->getWeight();
		}
	};

	typedef std::multiset<HuffTree*, TreeComparator> TreeSet;
public:
	// TODO, not hardcode the size in??
	uint16_t state_trans[256][2];
	Code codes[256];
	
	static const uint16_t start_state = 0;

	forceinline static bool isLeaf(uint16_t state) {
		return (state & 0x100) != 0;
	}

	forceinline uint32_t getTransition(uint16_t state, uint32_t bit) {
		assert(state < 256);
		return state_trans[state][bit];
	}

	forceinline static uint32_t getChar(uint16_t state) {
		assert(isLeaf(state));
		return state ^ 0x100;
	}

	forceinline const Code& getCode(uint32_t index) const {
		return codes[index];
	}

	template <typename T>
	void build(const Tree<T>* tree, uint32_t alphabet_size = 256) {
		typedef const Tree<T> TTree;
		tree->getCodes(codes);
		std::vector<TTree*> work, todo;
		work.push_back(tree);

		std::map<TTree*, uint32_t> tree_map;
		uint32_t cur_state = start_state, cur_state_leaf = cur_state + 0x100;
		bool state_available[256];
		for (auto& b : state_available) b = true;

		// Calculate tree -> state map.
		// TODO: Improve layout to maximize number of cache misses to 2 per byte.
		do {
			std::vector<TTree*> temp;
			for (uint32_t i = 0; i < work.size(); ++i) {
				auto* cur_tree = work[i];
				if (cur_tree->isLeaf()) {
					tree_map[cur_tree] = cur_tree->value | 0x100;
				} else {
					if (true || cur_state < 64) {
						state_available[cur_state] = false;
						tree_map[cur_tree] = cur_state++;
					} else {
						// Try to find a state with matching low 6 bits:
						// todo
					}
					temp.push_back(cur_tree->a);
					temp.push_back(cur_tree->b);
				}
			}
			work.swap(temp);
		} while (!work.empty());

		// Calculate transitions.
		for (auto it : tree_map) {
			auto* t = it.first;
			if (!isLeaf(tree_map[t])) {
				state_trans[tree_map[t]][0] = tree_map[t->a];
				state_trans[tree_map[t]][1] = tree_map[t->b];
			}
		}
	}

	// TODO: Optimize, fix memory leaks.
	// Based off of example from Introduction to Data Compression.
	static HuffTree* buildTreePackageMerge(size_t* frequencies, uint32_t count = 256, uint32_t max_depth = 16) {
		class Package {
		public:
			std::multiset<uint32_t> alphabets;
			uint64_t weight;

			Package() : weight(0) {

			}

			bool operator()(const Package* a, const Package* b) const {
				if (a->weight < b->weight) return true;
				if (a->weight > b->weight) return false;
				return a->alphabets.size() < b->alphabets.size();
			}
		};

		uint32_t package_limit = 2 * count - 2;

		// Set up initial packages.
		typedef std::multiset<Package*, Package> PSet;
		PSet original_set;
		for (uint32_t i = 0; i < count; ++i) {
			auto* p = new Package;
			p->alphabets.insert(i);
			p->weight = 1 + frequencies[i]; // Algorithm can't handle 0 frequencies.
			original_set.insert(p);
		}

		PSet merge_set = original_set;

		// Perform the package merge algorithm.
		for (uint32_t i = 1; i < max_depth; ++i) {
			PSet new_set;
			size_t count = merge_set.size() / 2;
			// Package count pacakges.
			auto it = merge_set.begin();
			for (uint32_t j = 0; j < count; ++j) {
				Package *a = *(it++);
				Package *b = *(it++);
				auto *new_package = new Package;
				new_package->alphabets.insert(a->alphabets.begin(), a->alphabets.end());
				new_package->alphabets.insert(b->alphabets.begin(), b->alphabets.end());
				new_package->weight = a->weight + b->weight;
				new_set.insert(new_package);
			}

			// Merge back into original set.
			merge_set = original_set;
			merge_set.insert(new_set.begin(), new_set.end());

			while (merge_set.size() > package_limit) {
				auto* pend = *merge_set.rbegin();
				merge_set.erase(pend);
				//delete pend;
			}

			// Print packages.
			/*if (false) {
				for (auto* p : merge_set) {
					std::cout << " " << p->weight << "{";
					for (auto a : p->alphabets)
						std::cout << a << ",";
					std::cout << "}, ";
				}
 				std::cout << std::endl;
			}*/
		}

		// Calculate lengths.
		std::vector<uint32_t> lengths(count, 0);
		for (auto* p : merge_set) {
			for (auto a : p->alphabets) { 
				++lengths[a];
			}
		}

		// Might not work for max_depth = 32.
		uint32_t total = 0;
		for (auto l : lengths) {
			assert(l > 0 && l <= max_depth);
			total += 1 << (max_depth - l);
		}

		// Sanity check.
		if (total != 1 << max_depth) {
			std::cerr << "Fatal error constructing huffman table " << total << " vs " << (1 << max_depth) << std::endl;
			return nullptr;
		}

		// Build huffmann tree from the code lengths.
		return buildFromCodeLengths(&lengths[0], count, max_depth, &frequencies[0]);
	}

	static HuffTree* buildFromCodeLengths(uint32_t* lengths, uint32_t count, uint32_t max_depth, size_t* freqs = nullptr) {
		HuffTree* tree = new HuffTree(uint32_t(0), 0);
		typedef std::vector<HuffTree*> TreeVec;
		TreeVec cur_level;
		cur_level.push_back(tree);
		for (uint32_t i = 0; i <= max_depth; ++i) {
			for (uint32_t j = 0; j < count; ++j) {
				if (lengths[j] == i) {
					if (cur_level.empty()) break;
					auto* tree = cur_level.back();
					cur_level.pop_back();
					tree->value = j;
					tree->weight = static_cast<uint32_t>(freqs != nullptr ? freqs[j] : 0);
				}
			}

			TreeVec new_set;
			for (uint32_t i = 0; i < cur_level.size(); ++i) {
				auto* tree = cur_level[i];
				tree->a = new HuffTree(uint32_t(0), 0);
				tree->b = new HuffTree(uint32_t(0), 0);
				new_set.push_back(tree->a);
				new_set.push_back(tree->b);
			}
			cur_level = new_set;
		}
		tree->calcDepth(0);
		return tree;
	}

	// Combine two smallest trees until we hit max depth.
	static HuffTree* buildTree(TreeSet& trees) {
		while (trees.size() > 1) {
			auto it = trees.begin();
			HuffTree *a = *it;
			trees.erase(it);
			it = trees.begin();
			HuffTree *b = *it; 
			trees.erase(it);
			// Reinsert the new tree.
			trees.insert(new HuffTree(a, b));	
		}
		return *trees.begin();
	}

	// Write a huffmann tree to a stream.
	template <typename TEnt, typename TStream>
	static void writeTree(TEnt& ent, TStream& stream, HuffTree* tree, uint32_t alphabet_size, uint32_t max_length) {
		std::vector<uint32_t> lengths(alphabet_size, 0);
		tree->getLengths(&lengths[0]);
		// Assumes we can't have any 0 length codes.
		for (uint32_t i = 0; i < alphabet_size; ++i) {
			assert(lengths[i] > 0 && lengths[i] <= max_length);
			ent.encodeDirect(stream, lengths[i] - 1, max_length);
		}
	}

	// Read a huffmann tree from a stream.
	template <typename TEnt, typename TStream>
	static HuffTree* readTree(TEnt& ent, TStream& stream, uint32_t alphabet_size, uint32_t max_length) {
		std::vector<uint32_t> lengths(alphabet_size, 0);
		for (uint32_t i = 0; i < alphabet_size; ++i) {
			lengths[i] = ent.decodeDirect(stream, max_length) + 1;
		}
		return buildFromCodeLengths(&lengths[0], alphabet_size, max_length, nullptr);
	}

	static const uint32_t alphabet_size = 256;
	static const uint32_t max_length = 16;
public:
	static const uint32_t version = 0;
	void setMemUsage(uint32_t n) {}

	template <typename TOut, typename TIn>
	uint64_t Compress(TOut& sout, TIn& sin) {
		Range7 ent;
		uint32_t count = 0;
		std::vector<size_t> freq(alphabet_size, 0);

		// Get frequencies
		uint32_t length = 0;
		for (;;++length) {
			auto c = sin.read();
			if (c == EOF) break;
			++freq[static_cast<uint32_t>(c)];
		}

		// Print frequencies
		printIndexedArray("frequencies", freq);
		sin.restart();

		// Build length limited tree with package merge algorithm.
		auto* tree = buildTreePackageMerge(&freq[0], alphabet_size, max_length);
		tree->printRatio("LL(16)");

		ProgressMeter meter;
		ent.init();
		writeTree(ent, sout, tree, alphabet_size, max_length);
		build(tree);
		std::cout << "Encoded huffmann tree in ~" << sout.getTotal() << " bytes" << std::endl;

		ent.EncodeBits(sout, length, 31);

		// Encode with huffman codes.
		std::cout << std::endl;
		for (;;) {
			int c = sin.read();
			if (c == EOF) break;
			const auto& huff_code = getCode(c);
			ent.EncodeBits(sout, huff_code.value, huff_code.length);
			meter.addBytePrint(sout.getTotal());
		}
		std::cout << std::endl;
		ent.flush(sout);
		return sout.getTotal();
	}

	template <typename TOut, typename TIn>
	bool DeCompress(TOut& sout, TIn& sin) {
		Range7 ent;
		ProgressMeter meter(true);

		ent.initDecoder(sin);
		auto* tree = readTree(ent, sin, alphabet_size, max_length);
		uint32_t length = ent.DecodeDirectBits(sin, 31);

		// Generate codes.
		build(tree);

		std::cout << std::endl;
		for (uint32_t i = 0; i < length; ++i) {
			uint32_t state = 0;
			do {
				state = getTransition(state, ent.DecodeDirectBit(sin));
			} while (!isLeaf(state));
			sout.write(getChar(state));
			meter.addBytePrint(sin.getTotal());
		}
		std::cout << std::endl;
		return true;
	}
};

class HuffmanStatic : public MemoryCompressor {
	static const uint32_t kCodeBits = 16;
	static const uint32_t kAlphabetSize = 256;
public:
	virtual uint32_t getMaxExpansion(uint32_t in_size) {
		return in_size * 6 / 5 + (kCodeBits * 256 / kBitsPerByte + 100);
	}
	virtual uint32_t compressBytes(byte* in, byte* out, uint32_t count);
	virtual void decompressBytes(byte* in, byte* out, uint32_t count);
};

#endif
