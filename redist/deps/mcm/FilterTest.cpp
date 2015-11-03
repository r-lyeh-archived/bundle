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

#include "Compressor.hpp"
#include "CM.hpp"
#include "DeltaFilter.hpp"
#include "Filter.hpp"
#include "TurboCM.hpp"
#include "X86Binary.hpp"

#include <numeric>
#include <vector>

static const uint32_t kDataSize = 654321;
static const uint32_t kBufferSize = 4 * KB;
static const uint32_t kTestIterations = 10;

static const uint32_t kIterations = 256;	
static const uint32_t kBenchDataSize = 7654321;

typedef FixedDeltaFilter<2, 2> WavDeltaFilter;

class SimpleFilter : public ByteStreamFilter<4 * KB, 4 * KB> {
public:
	SimpleFilter(Stream* stream) : ByteStreamFilter(stream) { }
	virtual void forwardFilter(byte* out, size_t* out_count, byte* in, size_t* in_count) {
		const auto max_c = std::min(*out_count, *in_count);
		for (auto i = 0; i < max_c; ++i) {
			out[i] = in[i] + 1;
		}
		*out_count = *in_count = max_c;
	}
	virtual void reverseFilter(byte* out, size_t* out_count, byte* in, size_t* in_count) {
		const auto max_c = std::min(*out_count, *in_count);
		for (auto i = 0; i < max_c; ++i) {
			out[i] = in[i] - 1;
		}
		*out_count = *in_count = max_c;
	}
	static uint32_t getMaxExpansion() {
		return 1;
	}
};

class SplitFilter : public ByteStreamFilter<4 * KB, 4 * KB> {
public:
	SplitFilter(Stream* stream) : ByteStreamFilter(stream) { }
	virtual void forwardFilter(byte* out, size_t* out_count, byte* in, size_t* in_count) {
		const auto max_in = std::min(*out_count / 3, *in_count);
		for (auto i = 0; i < max_in; ++i) {
			out[i * 3] = in[i] / 3;
			out[i * 3 + 1] = in[i] / 4;
			out[i * 3 + 2] = in[i] - out[i * 3 + 1] - out[i * 3];
		}
		*out_count = max_in * 3;
		*in_count = max_in;
	}
	virtual void reverseFilter(byte* out, size_t* out_count, byte* in, size_t* in_count) {
		const auto max_out = std::min(*out_count, *in_count / 3);
		for (auto i = 0; i < max_out; ++i) {
			out[i] = in[i * 3] + in[i * 3 + 1] + in[i * 3 + 2];
		}
		*out_count = max_out;
		*in_count = max_out * 3;
	}
	static uint32_t getMaxExpansion() {
		return 3;
	}
};

class Delta16 : public ByteBufferFilter<0x10000> {
public:
	Delta16(Stream* stream) : ByteBufferFilter(stream), prev_(0), prev2_(0) {
	}
	virtual void forwardFilter(byte* ptr, size_t count) {
		for (auto i = 0; i + 1 < count; i += 2) {
			uint16_t c = ptr[i] + (static_cast<uint32_t>(ptr[i + 1]) << 8);
			uint16_t new_c = c - prev2_;
			ptr[i] = static_cast<uint8_t>(new_c);
			ptr[i + 1] = static_cast<uint8_t>(new_c >> 8);
			prev2_ = prev_;
			prev_ = c;
		}
	}
	virtual void reverseFilter(byte* ptr, size_t count) {
		for (auto i = 0; i + 1 < count; i += 2) {
			uint16_t c = ptr[i] + (static_cast<uint32_t>(ptr[i + 1]) << 8);
			uint16_t new_c = c + prev2_;
			ptr[i] = static_cast<uint8_t>(new_c);
			ptr[i + 1] = static_cast<uint8_t>(new_c >> 8);
			prev2_ = prev_;
			prev_ = new_c;	
		}
	}
	static uint32_t getMaxExpansion() {
		return 1;
	}
	void dumpInfo() const {
	}
	void setSpecific(uint32_t s) {
	}

private:
	uint16_t prev_;
	uint16_t prev2_;
};

class Delta8 : public ByteBufferFilter<0x10000> {
public:
	Delta8(Stream* stream) : ByteBufferFilter(stream) {
		std::fill(prev_, prev_ + 4, 0U);
	}
	virtual void forwardFilter(byte* ptr, uint32_t count) {
		for (uint32_t i = 0; i < count; i += 2) {
			uint8_t c = ptr[i];
			ptr[i] = c - prev_[3];
			memmove(prev_ + 1, prev_, 3);
			prev_[0] = c;
		}
	}
	virtual void reverseFilter(byte* ptr, uint32_t count) {
		for (uint32_t i = 0; i + 1 < count; i += 2) {
			uint8_t c = ptr[i];
			ptr[i] = c + prev_[3];
			memmove(prev_ + 1, prev_, 3);
			prev_[0] = ptr[i];
		}
	}
	static uint32_t getMaxExpansion() {
		return 1;
	}
	void dumpInfo() const {
	}
	void setSpecific(uint32_t s) {
	}

private:
	uint8_t prev_[4];
};

class ColorTrans : public ByteBufferFilter<0x5000 * 3> {
public:
	ColorTrans(Stream* stream) : ByteBufferFilter(stream) {
	}
	virtual void forwardFilter(byte* ptr, uint32_t count) {
		for (uint32_t i = 0; i + 2 < count; i += 3) {
			ptr[i + 1] -= ptr[i];
			ptr[i + 2] -= ptr[i];
		}
	}
	virtual void reverseFilter(byte* ptr, uint32_t count) {
		for (uint32_t i = 0; i + 2 < count; i += 3) {
			ptr[i + 1] += ptr[i];
			ptr[i + 2] += ptr[i];
		}
	}
	static uint32_t getMaxExpansion() {
		return 1;
	}
	void dumpInfo() const {
	}
	void setSpecific(uint32_t s) {
	}

private:
};

class BlockSplit : public ByteBufferFilter<0x10000> {
public:
	BlockSplit(Stream* stream) : ByteBufferFilter(stream) {
	}
	virtual void forwardFilter(byte* ptr, uint32_t count) {
		if (count != kBlockSize) {
			return;
		}
		byte out[kBlockSize];
		uint32_t pos[4] = { 0 };
		pos[1] = kBlockSize / 4;
		pos[2] = kBlockSize / 2;
		pos[3] = kBlockSize * 3 / 4;
		for (uint32_t i = 0; i < count; ++i) {
			out[pos[i % 4]++] = ptr[i];
		}
		memcpy(ptr, out, count);
	}
	virtual void reverseFilter(byte* ptr, uint32_t count) {
		if (count != 0x10000) {
			return;
		}
		byte out[kBlockSize];
		uint32_t pos = 0;
		for (uint32_t i = 0; i < 4; ++i) {
			for (uint32_t j = 0; j < kBlockSize / 4; ++j) {
				out[i + j * 4] = ptr[pos++];
			}
		}
		memcpy(ptr, out, count);
	}
	static uint32_t getMaxExpansion() {
		return 1;
	}
	void dumpInfo() const {
	}
	void setSpecific(uint32_t s) {
	}

private:
	static const uint32_t kBlockSize = 0x10000;
};

template<class FilterType>
void testFilter() {
	Store store_comp;
	std::vector<byte> data;
	uint32_t size = (rand() * 7654321 + rand()) % kDataSize;
	for (uint32_t i = 0; i < size; ++i) {
		data.push_back(rand() % 256);
	}
	std::vector<byte> out_data;
	store_comp.compress(&FilterType(&ReadMemoryStream(&data)), &WriteVectorStream(&out_data), std::numeric_limits<uint64_t>::max());
	std::vector<byte> result;
	WriteVectorStream wvs(&result);
	FilterType reverse_filter(&wvs); 
	store_comp.decompress(&ReadMemoryStream(&out_data), &reverse_filter, std::numeric_limits<uint64_t>::max());
	reverse_filter.flush();
	// Check tht the shit matches.
	check(result.size() == data.size());
	for (uint32_t i = 0; i < data.size(); ++i) {
		byte a = result[i];
		byte b = data[i];
		check(a == b);
	}
}

template<class FilterType>
void benchFilter(const std::vector<byte>& data) {
	CM<kCMTypeMax> comp(6);
	// data = randomArray(kBenchDataSize);
	check(!data.empty());
	const uint64_t expected_sum = std::accumulate(data.begin(), data.end(), 0UL);
	std::vector<byte> out_data;
	out_data.resize(20 * MB + static_cast<uint32_t>(data.size() * FilterType::getMaxExpansion() * 1.2));
	uint64_t start = clock();
	uint64_t write_count;
	uint64_t old_sum = 0;
	uint32_t best_size = std::numeric_limits<uint32_t>::max();
	uint32_t best_spec;
	for (uint32_t i = 0; i < kIterations; ++i) {
		WriteMemoryStream wms(&out_data[0]);
		ReadMemoryStream rms(&data);
		FilterType f(&rms); // , 1 + (i & 3), 1 + (i / 4));
		// f.setSpecific(100 + i);
		comp.setOpt(i);
		clock_t start = clock();
		comp.compress(&f, &wms, std::numeric_limits<uint64_t>::max());
		write_count = wms.tell();
		f.dumpInfo();
		if (write_count < best_size) {
			best_size = static_cast<uint32_t>(write_count);
			best_spec = i;
		}
		std::cout << "Cur=" << i << " size=" << write_count << " best(" << best_spec << ")=" << best_size << " time=" << clock() - start << std::endl;
#if 0
		uint64_t checksum = std::accumulate(&out_data[0], &out_data[write_count], 0UL);
		if (old_sum != 0) {
			check(old_sum == checksum);
		}
		old_sum = checksum;
#endif
	}	
	std::cout << "Forward: " << data.size() << "->" << write_count << " rate=" << prettySize(computeRate(data.size() * kIterations, clock() - start)) << "/S" << std::endl;

	std::vector<byte> result;
	result.resize(data.size());
	start = clock();
	for (uint32_t i = 0; i < kIterations; ++i) {
		WriteMemoryStream wvs(&result[0]);
		FilterType reverse_filter(&wvs); 
		comp.decompress(&ReadMemoryStream(&out_data[0], &out_data[0] + write_count), &reverse_filter, std::numeric_limits<uint64_t>::max());
		reverse_filter.flush();
	}
	uint64_t rate = computeRate(data.size() * kIterations, clock() - start);
	std::cout << "Reverse: " << prettySize(rate) << "/S" << std::endl;
	// Check tht the shit matches.
	check(result.size() == data.size());
	for (uint32_t i = 0; i < data.size(); ++i) {
		check(result[i] == data[i]);
	}
}
	
void runFilterTests() {
#ifdef ENABLE_TEST
	for (uint32_t i = 0; i < kTestIterations; ++i) {
		testFilter<SimpleFilter>();
		testFilter<SplitFilter>();
		testFilter<X86BinaryFilter>();
		testFilter<X86AdvancedFilter>();
		testFilter<Delta16>();
		testFilter<FixedDeltaFilter<1, 1>>();
		testFilter<FixedDeltaFilter<2, 1>>();
		testFilter<FixedDeltaFilter<1, 2>>();
		testFilter<IdentityFilter>();
		std::cout << "Running test " << i << std::endl;
	}
	std::cout << "Done running " << kTestIterations << " test iterations" << std::endl;

	std::vector<byte> data = loadFile("vcfiu.hlp", 4 * MB);
	//std::vector<byte> data = loadFile("rafale.bmp", 4 * MB);
	//std::vector<byte> data = loadFile("ohs.doc", 5 * MB);
	//std::vector<byte> data = loadFile("world95.txt", 4 * MB);
	//std::vector<byte> data = loadFile("english.dic", 4 * MB);
	//std::vector<byte> data = loadFile("enwik8.txt", 12 * MB );
	//std::vector<byte> data = loadFile("fp.log", 24 * MB);
	//std::vector<byte> data = loadFile("mso97.dll", 24 * MB);
	//std::vector<byte> data = loadFile("acrord32.exe", 24 * MB);
	//std::vector<byte> data = loadFile("flashmx.pdf", 5 * MB);
	//std::vector<byte> data = loadFile("enwik46.txt", 5 * MB);
	//std::vector<byte> data = loadFile("A10.jpg", 5 * MB);
	//std::vector<byte> data = loadFile("include.tar", 25 * MB);
	//std::vector<byte> data = loadFile("calgary.tar", 5 * MB);
	//std::vector<byte> data = loadFile("mxc.tar", 60 * MB);
	//std::vector<byte> data = loadFile("magic.txt", 60 * MB);
	//std::vector<byte> data = loadFile("test.dll", 5 * MB);
	//std::vector<byte> data = loadFile("include.tar", 4 * MB);
	// Count freqs.
	uint32_t freq[256] = { 0 };
	for (byte c : data) {
		++freq[c];
	}
	for (uint32_t i = 0; i < 256; ++i) {
		std::cout << i << "=" << freq[i] << std::endl;
	}
	//benchFilter<Delta16>(data);
	//benchFilter<X86AdvancedFilter>(data);
	//benchFilter<Delta8>(data);
	//benchFilter<FixedDeltaFilter<2, 1>>(data);
	//benchFilter<FixedDeltaFilter<2, 2>>(data);
	//benchFilter<FixedDeltaFilter<1, 1>>(data);
	benchFilter<X86AdvancedFilter>(data);
	//benchFilter<IdentityFilter>(data);
	//benchFilter<ColorTrans>(data);
#endif
}
