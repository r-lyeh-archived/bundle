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

#ifndef ARCHIVE_HPP_
#define ARCHIVE_HPP_

#include <thread>

#include "CM.hpp"
#include "Compressor.hpp"
#include "File.hpp"
#include "Stream.hpp"

// Force filter
enum FilterType {
	kFilterTypeNone,
	kFilterTypeDict,
	kFilterTypeX86,
	kFilterTypeAuto,
	kFilterTypeCount,
};

enum CompLevel {
	kCompLevelStore,
	kCompLevelTurbo,
	kCompLevelFast,
	kCompLevelMid,
	kCompLevelHigh,
	kCompLevelMax,
};
std::ostream& operator<<(std::ostream& os, CompLevel comp_level);

enum LZPType {
	kLZPTypeAuto,
	kLZPTypeEnable,
	kLZPTypeDisable,
};

class CompressionOptions {
public:
	static const size_t kDefaultMemUsage = 6;
	static const CompLevel kDefaultLevel = kCompLevelMid;
	static const FilterType kDefaultFilter = kFilterTypeAuto;
	static const LZPType kDefaultLZPType = kLZPTypeAuto;
	CompressionOptions() : mem_usage_(kDefaultMemUsage), comp_level_(kDefaultLevel), filter_type_(kDefaultFilter), lzp_type_(kDefaultLZPType) {
	}

public:
	size_t mem_usage_;
	CompLevel comp_level_;
	FilterType filter_type_;
	LZPType lzp_type_;
};

// File headers are stored in a list of blocks spread out through data.
class Archive {
public:
	class Header {
	public:
		static const size_t kCurMajorVersion = 0;
		static const size_t kCurMinorVersion = 83;
		static const size_t kMagicStringLength = 10;
		
		static const char* getMagic() {
			return "MCMARCHIVE";
		}
		Header();
		void read(Stream* stream);
		void write(Stream* stream);
		bool isArchive() const;
		bool isSameVersion() const;
		uint16_t majorVersion() const {
			return major_version_;
		}
		uint16_t minorVersion() const {
			return minor_version_;
		}

	private:
		char magic_[10]; // MCMARCHIVE
		uint16_t major_version_;
		uint16_t minor_version_;
	};

	class Algorithm {
	public:
		Algorithm() {}
		Algorithm(const CompressionOptions& options, Detector::Profile profile);
		Algorithm(Stream* stream);
		Compressor* createCompressor();
		void read(Stream* stream);
		void write(Stream* stream);
		Filter* createFilter(Stream* stream, Analyzer* analyzer);
		Detector::Profile profile() const {
			return profile_;
		}

	private:
		uint8_t mem_usage_;
		Compressor::Type algorithm_;
		bool lzp_enabled_;
		FilterType filter_;
		Detector::Profile profile_;
	};

	class SolidBlock {
	public:
		Algorithm algorithm_;
		std::vector<FileSegmentStream::FileSegments> segments_;
		// Not stored, obtianed from segments.
		uint64_t total_size_;

		SolidBlock();
		void write(Stream* stream);
		void read(Stream* stream);
	};

	class Blocks {
	public:
		std::vector<SolidBlock*> blocks_;

		void write(Stream* stream);
		void read(Stream* stream);
	};

	// Compression.
	Archive(Stream* stream, const CompressionOptions& options);

	// Decompression.
	Archive(Stream* stream);

	// Construct blocks from analyzer.
	void constructBlocks(Stream* in, Analyzer* analyzer);

	const Header& getHeader() const {
		return header_;
	}

	bool setOpt(size_t var) {
		opt_var_ = var;
		return true;
	}

	void writeBlocks();
	void readBlocks();

	// Analyze and compress.
	void compress(Stream* in);

	// Decompress.
	void decompress(Stream* out);

private:
	Stream* stream_;
	Header header_;
	CompressionOptions options_;
	size_t opt_var_;
	Blocks blocks_;

	void init();
	Compressor* createMetaDataCompressor();
};

#endif
