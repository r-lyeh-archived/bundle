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

#include "Archive.hpp"

#include <algorithm>
#include <cstring>

#include "LZ.hpp"
#include "X86Binary.hpp"
#include "Wav16.hpp"

static const bool kTestFilter = false;
static const size_t kSizePad = 10;

Archive::Header::Header() : major_version_(kCurMajorVersion), minor_version_(kCurMinorVersion) {
	memcpy(magic_, getMagic(), kMagicStringLength);
}

void Archive::Header::read(Stream* stream) {
	stream->read(reinterpret_cast<uint8_t*>(magic_), kMagicStringLength);
	major_version_ = stream->get16();
	minor_version_ = stream->get16();
}

void Archive::Header::write(Stream* stream) {
	stream->write(reinterpret_cast<uint8_t*>(magic_), kMagicStringLength);
	stream->put16(major_version_);
	stream->put16(minor_version_);
}

bool Archive::Header::isArchive() const {
	return memcmp(magic_, getMagic(), kMagicStringLength) == 0;
}

bool Archive::Header::isSameVersion() const {
	return major_version_ == kCurMajorVersion && minor_version_ == kCurMinorVersion;
}

Archive::Algorithm::Algorithm(const CompressionOptions& options, Detector::Profile profile) : profile_(profile) {
	mem_usage_ = options.mem_usage_;
	algorithm_ = Compressor::kTypeStore;
	filter_ = FilterType::kFilterTypeNone;

	if (profile == Detector::kProfileWave16) {
		algorithm_ = Compressor::kTypeWav16;
		// algorithm_ = Compressor::kTypeStore;
	} else {
		switch (options.comp_level_) {
		case kCompLevelStore:
			algorithm_ = Compressor::kTypeStore;
			break;
		case kCompLevelTurbo:
			algorithm_ = Compressor::kTypeCMTurbo;
			break;
		case kCompLevelFast:
			algorithm_ = Compressor::kTypeCMFast;
			break;
		case kCompLevelMid:
			algorithm_ = Compressor::kTypeCMMid;
			break;
		case kCompLevelHigh:
			algorithm_ = Compressor::kTypeCMHigh;
			break;
		case kCompLevelMax:
			algorithm_ = Compressor::kTypeCMMax;
			break;
		}
	}
	switch (profile) {
	case Detector::kProfileBinary:
		lzp_enabled_ = true;
		filter_ = kFilterTypeX86;
		break;
	case Detector::kProfileText:
		lzp_enabled_ = true;
		filter_ = kFilterTypeDict;
		break;
	}
	// OVerrrides.
	if (options.lzp_type_ == kLZPTypeEnable) lzp_enabled_ = true;
	else if (options.lzp_type_ == kLZPTypeDisable) lzp_enabled_ = false;
	// Force filter.
	if (options.filter_type_ != kFilterTypeAuto) {
		filter_ = options.filter_type_;
	}
}

Archive::Algorithm::Algorithm(Stream* stream) {
	read(stream);
}

void Archive::init() {
	opt_var_ = 0;
}

Archive::Archive(Stream* stream, const CompressionOptions& options) : stream_(stream), options_(options) {
	init();
	header_.write(stream_);
}

Archive::Archive(Stream* stream) : stream_(stream), opt_var_(0) {
	init();
	header_.read(stream_);
}

Compressor* Archive::Algorithm::createCompressor() {
	switch (algorithm_) {
	case Compressor::kTypeWav16:
		return new Wav16;
	case Compressor::kTypeStore:
		return new Store;
	case Compressor::kTypeCMTurbo:
		return new CM<kCMTypeTurbo>(mem_usage_, lzp_enabled_, profile_);
		// return new CMRolz;
	case Compressor::kTypeCMFast:
		return new CM<kCMTypeFast>(mem_usage_, lzp_enabled_, profile_);
	case Compressor::kTypeCMMid:
		return new CM<kCMTypeMid>(mem_usage_, lzp_enabled_, profile_);
	case Compressor::kTypeCMHigh:
		return new CM<kCMTypeHigh>(mem_usage_, lzp_enabled_, profile_);
	case Compressor::kTypeCMMax:
		return new CM<kCMTypeMax>(mem_usage_, lzp_enabled_, profile_);
	}
	return nullptr;
}

void Archive::Algorithm::read(Stream* stream) {
	mem_usage_ = static_cast<uint8_t>(stream->get());
	algorithm_ = static_cast<Compressor::Type>(stream->get());
	lzp_enabled_ = stream->get() != 0 ? true : false;
	filter_ = static_cast<FilterType>(stream->get());
	profile_ = static_cast<Detector::Profile>(stream->get());
}

void Archive::Algorithm::write(Stream* stream) {
	stream->put(mem_usage_);
	stream->put(algorithm_);
	stream->put(lzp_enabled_);
	stream->put(filter_);
	stream->put(profile_);
}

std::ostream& operator<<(std::ostream& os, CompLevel comp_level) {
	switch (comp_level) {
	case kCompLevelStore: return os << "store";
	case kCompLevelTurbo: return os << "turbo";
	case kCompLevelFast: return os << "fast";
	case kCompLevelMid: return os << "mid";
	case kCompLevelHigh: return os << "high";
	case kCompLevelMax: return os << "max";
	}
	return os << "unknown";
}

Filter* Archive::Algorithm::createFilter(Stream* stream, Analyzer* analyzer) {
	switch (filter_) {
	case kFilterTypeDict:
		if (analyzer) {
			auto& builder = analyzer->getDictBuilder();
			Dict::CodeWordGeneratorFast generator;
			Dict::CodeWordSet code_words;
			generator.generateCodeWords(builder, &code_words);
			auto dict_filter = new Dict::Filter(stream, 0x3, 0x4, 0x6);
			dict_filter->addCodeWords(code_words.getCodeWords(), code_words.num1_, code_words.num2_, code_words.num3_);
			return dict_filter;
		} else {
			return new Dict::Filter(stream);
		}
	case kFilterTypeX86:
		return new X86AdvancedFilter(stream);
	}
	return nullptr;
}

Archive::SolidBlock::SolidBlock() {
}

class BlockSizeComparator {
public:
	bool operator()(const Archive::SolidBlock* a, const Archive::SolidBlock* b) const {
		return a->total_size_ < b->total_size_;
	}
};

void Archive::constructBlocks(Stream* in, Analyzer* analyzer) {
	// Compress blocks.
	uint64_t total_in = 0;
	for (size_t p_idx = 0; p_idx < static_cast<size_t>(Detector::kProfileCount); ++p_idx) {
		auto profile = static_cast<Detector::Profile>(p_idx);
		// Compress each stream type.
		uint64_t pos = 0;
		FileSegmentStream::FileSegments seg;
		seg.base_offset_ = 0;
		seg.stream_ = in;
		for (const auto& b : analyzer->getBlocks()) {
			const auto len = b.length();
			if (b.profile() == profile) {
				FileSegmentStream::SegmentRange range;
				range.offset_ = pos;
				range.length_ = len;
				seg.ranges_.push_back(range);
			}
			pos += len;
		}
		seg.calculateTotalSize();
		if (seg.total_size_ > 0) {
			auto* solid_block = new Archive::SolidBlock();
			solid_block->algorithm_ = Algorithm(options_, profile);
			solid_block->segments_.push_back(seg);
			solid_block->total_size_ = seg.total_size_;
			blocks_.blocks_.push_back(solid_block);
		}
	}
	std::sort(blocks_.blocks_.rbegin(), blocks_.blocks_.rend(), BlockSizeComparator());
}

Compressor* Archive::createMetaDataCompressor() {
	return new CM<kCMTypeFast>(6, true, Detector::kProfileText);
}

void Archive::writeBlocks() {
	std::vector<uint8_t> temp;
	WriteVectorStream wvs(&temp);
	// Write out the blocks into temp.
	blocks_.write(&wvs);
	// Compress overhead.
	std::unique_ptr<Compressor> c(createMetaDataCompressor());
	ReadMemoryStream rms(&temp[0], &temp[0] + temp.size());
	auto start_pos = stream_->tell();
	stream_->leb128Encode(temp.size());
	c->compress(&rms, stream_);
	stream_->leb128Encode(static_cast<uint64_t>(1234u));
	//std::cout << "Compressed metadata " << temp.size() << " -> " << stream_->tell() - start_pos << std::endl << std::endl;
}

void Archive::readBlocks() {
	auto metadata_size = stream_->leb128Decode();
	//std::cout << "Metadata size=" << metadata_size << std::endl;
	// Decompress overhead.
	std::unique_ptr<Compressor> c(createMetaDataCompressor());
	std::vector<uint8_t> metadata;
	WriteVectorStream wvs(&metadata);
	auto start_pos = stream_->tell();
	c->decompress(stream_, &wvs, metadata_size);
	auto cmp = stream_->leb128Decode();
	check(cmp == 1234u);
	ReadMemoryStream rms(&metadata);
	blocks_.read(&rms);
}

void Archive::Blocks::write(Stream* stream) { 
	stream->leb128Encode(blocks_.size());
	for (auto* block : blocks_) {
		block->write(stream);
	}
}

void Archive::Blocks::read(Stream* stream) { 
	size_t num_blocks = stream->leb128Decode();
	check(num_blocks < 1000000);  // Sanity check.
	blocks_.clear();
	for (size_t i = 0; i < num_blocks; ++i) {
		auto* block = new SolidBlock;
		block->read(stream);
		blocks_.push_back(block);
	}
}

void Archive::SolidBlock::write(Stream* stream) { 
	algorithm_.write(stream);
	stream->leb128Encode(segments_.size());
	for (auto& seg : segments_) {
		seg.write(stream);
	}
}

void Archive::SolidBlock::read(Stream* stream) { 
	algorithm_.read(stream);
	size_t num_segments = stream->leb128Decode();
	check(num_segments < 10000000);
	segments_.resize(num_segments);
	for (auto& seg : segments_) {
		seg.read(stream);
		seg.calculateTotalSize();
		//std::cout << Detector::profileToString(algorithm_.profile()) << " size " << seg.total_size_ << std::endl;
	}
}

void testFilter(Stream* stream, Analyzer* analyzer) {
	std::vector<uint8_t> comp;
	stream->seek(0);
	auto start = clock();
	{
		auto& builder = analyzer->getDictBuilder();
		Dict::CodeWordGeneratorFast generator;
		Dict::CodeWordSet code_words;
		generator.generateCodeWords(builder, &code_words);
		auto dict_filter = new Dict::Filter(stream, 0x3, 0x4, 0x6);
		dict_filter->addCodeWords(code_words.getCodeWords(), code_words.num1_, code_words.num2_, code_words.num3_);
		WriteVectorStream wvs(&comp);
		Store store;
		store.compress(dict_filter, &wvs, std::numeric_limits<uint64_t>::max());
	}
	uint64_t size = stream->tell();
	//std::cout << "Filter comp " << size << " -> " << comp.size() << " in " << clockToSeconds(clock() - start) << "s" << std::endl;
	// Test revser speed
	start = clock();
	stream->seek(0);
	VoidWriteStream voids;
	{
		ReadMemoryStream rms(&comp);
		Store store;
		Dict::Filter filter_out(&voids);
		store.decompress(&rms, &filter_out, size);
		filter_out.flush();
	}
	//std::cout << "Void decomp " << voids.tell() << " <- " << comp.size() << " in "  << clockToSeconds(clock() - start) << "s" << std::endl;
	// Test reverse.
	start = clock();
	stream->seek(0);
	VerifyStream vs(stream, size);
	{
		ReadMemoryStream rms(&comp);
		Store store;
		Dict::Filter filter_out(&vs);
		store.decompress(&rms, &filter_out, size);
		filter_out.flush();
		vs.summary();
	}
	//std::cout << "Verify decomp " << vs.tell() << " <- " << comp.size() << " in "  << clockToSeconds(clock() - start) << "s" << std::endl << std::endl;
}

// Analyze and compress.
void Archive::compress(Stream* in) {
	Analyzer analyzer;
	auto start_a = clock();
	//std::cout << "Analyzing" << std::endl;
	{
		ProgressThread thr(in, stream_);
		analyzer.analyze(in);
	}
	//std::cout << std::endl;
	//analyzer.dump();
	//std::cout << "Analyzing took " << clockToSeconds(clock() - start_a) << "s" << std::endl << std::endl;

	if (kTestFilter) {
		auto* dict = new Dict;
		testFilter(in, &analyzer);
	}

	constructBlocks(in, &analyzer);
	writeBlocks();

	for (auto* block : blocks_.blocks_) {
		auto start_pos = stream_->tell();
		
		auto start = clock();
		auto out_start = stream_->tell();
		for (size_t i = 0; i < kSizePad; ++i) stream_->put(0);

		FileSegmentStream segstream(&block->segments_, 0u);	
		Algorithm* algo = &block->algorithm_;
		//std::cout << "Compressing " << Detector::profileToString(algo->profile())
		//	<< " stream size=" << formatNumber(block->total_size_) << "\t" << std::endl;
		std::unique_ptr<Filter> filter(algo->createFilter(&segstream, &analyzer));
		Stream* in_stream = &segstream;
		if (filter.get() != nullptr) in_stream = filter.get();
		auto in_start = in_stream->tell();
		std::unique_ptr<Compressor> comp(algo->createCompressor());
		comp->setOpt(opt_var_);
		{
			ProgressThread thr(&segstream, stream_, true, out_start);
			comp->compress(in_stream, stream_);
		}
		auto after_pos = stream_->tell();

		// Fix up the size.
		stream_->seek(out_start);
		const auto filter_size = in_stream->tell() - in_start;
		stream_->leb128Encode(filter_size);
		stream_->seek(after_pos);

		// Dump some info.
		//std::cout << std::endl;
		//std::cout << "Compressed " << formatNumber(segstream.tell()) << " -> " << formatNumber(after_pos - out_start)
		//	<< " in " << clockToSeconds(clock() - start) << "s" << std::endl << std::endl;
	}
}

// Decompress.
void Archive::decompress(Stream* out) {
	readBlocks();
	for (auto* block : blocks_.blocks_) {
		block->total_size_ = 0;
		auto start_pos = stream_->tell();
		for (auto& seg : block->segments_) {
			seg.stream_ = out;
			block->total_size_ += seg.total_size_;
		}

		// Read size.
		auto out_start = stream_->tell();
		auto block_size = stream_->leb128Decode();
		while (stream_->tell() < out_start + kSizePad) {
			stream_->get();
		}

		auto start = clock();
		FileSegmentStream segstream(&block->segments_, 0u);	
		Algorithm* algo = &block->algorithm_;
		//std::cout << "Decompressing " << Detector::profileToString(algo->profile())
		//	<< " stream size=" << formatNumber(block->total_size_) << "\t" << std::endl;
		std::unique_ptr<Filter> filter(algo->createFilter(&segstream, nullptr));
		Stream* out_stream = &segstream;
		if (filter.get() != nullptr) out_stream = filter.get();
		std::unique_ptr<Compressor> comp(algo->createCompressor());
		comp->setOpt(opt_var_);
		{
			ProgressThread thr(&segstream, stream_, false, out_start);
			comp->decompress(stream_, out_stream, block_size);
			if (filter.get() != nullptr) filter->flush();
		}
		//std::cout << std::endl << "Decompressed " << formatNumber(segstream.tell()) << " <- " << formatNumber(stream_->tell() - out_start)
		//	<< " in " << clockToSeconds(clock() - start) << "s" << std::endl << std::endl;
	}
}
