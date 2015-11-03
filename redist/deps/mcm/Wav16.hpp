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

#ifndef _WAV16_HPP_
#define _WAV16_HPP_

#include <cstdlib>
#include <vector>

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

class Wav16 : public Compressor {
public:
	// SS table
	static const uint32_t kShift = 12;
	static const uint32_t kMaxValue = 1 << kShift;
	typedef fastBitModel<int, kShift, 9, 30> StationaryModel;
	// typedef bitLearnModel<kShift, 8, 30> StationaryModel;
	
	static const size_t kSampleShift = 4;
	static const size_t kSamplePr = 16 - kSampleShift;	
	static const size_t kSampleCount = 1u << kSamplePr;
	
	static const size_t kContextBits = 2;
	static const size_t kContextMask = (1u << kContextBits) - 1;
	std::vector<StationaryModel> models_;

	// Range encoder
	Range7 ent;

	// Optimization variable.
	uint32_t opt_var;

	// Noise bits which are just direct encoded.
	size_t noise_bits_;
	size_t non_noise_bits_;

	Wav16() : opt_var(0) {
	}

	bool setOpt(uint32_t var) {
		opt_var = var;
		return true;
	}

	void init() {
		noise_bits_ = 3;
		non_noise_bits_ = 16 - noise_bits_;
		size_t num_ctx = 2 << (non_noise_bits_ + kContextBits);
		models_.resize(num_ctx);
		for (auto& m : models_) {
			m.init();
		}
	}

	template <const bool kDecode, typename TStream>
	uint32_t processSample(TStream& stream, size_t context, size_t channel, uint32_t c = 0) {
		uint32_t code = 0;
		if (!kDecode) {
			code = c << (sizeof(uint32_t) * 8 - 16);
		}
		int ctx = 1;
		context = context * 2 + channel;
		context <<= non_noise_bits_;
		check(context < models_.size());
		for (uint32_t i = 0; i < non_noise_bits_; ++i) {
			auto& m = models_[context + ctx];
			int p = m.getP();
			p += p == 0;
			uint32_t bit;
			if (kDecode) { 
				bit = ent.getDecodedBit(p, kShift);
			} else {
				bit = code >> (sizeof(uint32_t) * 8 - 1);
				code <<= 1;
				ent.encode(stream, bit, p, kShift);
			}
			m.update(bit);
			ctx = ctx * 2 + bit;
			// Encode the bit / decode at the last second.
			if (kDecode) {
				ent.Normalize(stream);
			}
		}

		// Decode noisy bits (direct).
		for (size_t i = 0; i < noise_bits_; ++i) {
			if (kDecode) {
				ctx += ctx + ent.decodeBit(stream);
			} else {
				ent.encodeBit(stream, code >> 31); code <<= 1;
			}
		}
		
		return ctx ^ (1u << 16);
	}

	virtual void compress(Stream* in_stream, Stream* out_stream, uint64_t max_count) {
		BufferedStreamReader<4 * KB> sin(in_stream);
		BufferedStreamWriter<4 * KB> sout(out_stream);
		assert(in_stream != nullptr);
		assert(out_stream != nullptr);
		init();
		ent.init();
		uint16_t last_a = 0, last_b = 0;
		uint16_t last_a2 = 0, last_b2 = 0;
		uint16_t last_a3 = 0, last_b3 = 0;
		uint64_t i = 0;
		for (; i < max_count; i += 4) {
			int c1 = sin.get(), c2 = sin.get();
			uint16_t a = c1 + c2 * 256;
			int c3 = sin.get(), c4 = sin.get();
			uint16_t b = c3 + c4 * 256;
			uint16_t pred_a = 2 * last_a - last_a2;
			uint16_t pred_b = 2 * last_b - last_b2;
			uint16_t pred_a2 = 3 * last_a - 3 * last_a2 - last_a3;
			uint16_t pred_b2 = 3 * last_b - 3 * last_b2 - last_b3;
			uint16_t delta_a = a - pred_a;
			uint16_t delta_b = b - pred_b;
			if (c1 == EOF) {
				break;
			}
			processSample<false>(sout, 0, 0, delta_a);
			processSample<false>(sout, 0, 1, delta_b);
			last_a3 = last_a2;
			last_b3 = last_b2;
			last_a2 = last_a;
			last_b2 = last_b;
			last_a = a;
			last_b = b;
		}
		ent.flush(sout);
		sout.flush();
	}

	virtual void decompress(Stream* in_stream, Stream* out_stream, uint64_t max_count) {
		BufferedStreamReader<4 * KB> sin(in_stream);
		BufferedStreamWriter<4 * KB> sout(out_stream);
		auto start = in_stream->tell();
		init();
		ent.initDecoder(sin);
		uint16_t last_a = 0, last_b = 0;
		uint16_t last_a2 = 0, last_b2 = 0;
		uint16_t last_a3 = 0, last_b3 = 0;
		while (max_count > 0) {
			uint16_t pred_a = 2 * last_a - last_a2;
			uint16_t pred_b = 2 * last_b - last_b2;
			uint16_t a = pred_a + processSample<true>(sin, 0, 0);
			uint16_t b = pred_b + processSample<true>(sin, 0, 1);
			if (max_count > 0) { --max_count; sout.put(a & 0xFF); }
			if (max_count > 0) { --max_count; sout.put(a >> 8); }
			if (max_count > 0) { --max_count; sout.put(b & 0xFF); }
			if (max_count > 0) { --max_count; sout.put(b >> 8); }
			last_a3 = last_a2;
			last_b3 = last_b2;
			last_a2 = last_a;
			last_b2 = last_b;
			last_a = a;
			last_b = b;
		}
		sout.flush();
		size_t remain = sin.remain();
		if (remain > 0) {
			// Go back all the characters we didn't actually read.
			auto target = in_stream->tell() - remain;
			in_stream->seek(target);
		}
	}	
};


#endif
