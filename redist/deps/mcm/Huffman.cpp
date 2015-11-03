/*	MCM file compressor

	Copyright (C) 2014, Google Inc.
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

#include "Huffman.hpp"

uint32_t HuffmanStatic::compressBytes(byte* in, byte* out, uint32_t count) {
	size_t freq[256] = { 0 };

	// Get frequencies
	uint32_t length = 0;
	for (uint32_t i = 0; i < count;++i) {
		++freq[in[i]];
	}

	Huffman huff;

	// Build length limited tree with package merge algorithm.
	auto* tree = huff.buildTreePackageMerge(&freq[0], kAlphabetSize, kCodeBits);
	
#if 0

	ent.init();
	uint32_t lengths[kAlphabetSize] = { 0 };
	tree->getLengths(&lengths[0]);

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
#endif
	return 0;
}

void HuffmanStatic::decompressBytes(byte* in, byte* out, uint32_t count) {

}
