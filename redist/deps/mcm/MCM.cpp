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

#include <chrono>
#include <ctime>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>
#include <thread>

#include "Archive.hpp"
#include "CM.hpp"
#include "DeltaFilter.hpp"
#include "Dict.hpp"
#include "File.hpp"
#include "Huffman.hpp"
#include "LZ.hpp"
#include "ProgressMeter.hpp"
#include "Tests.hpp"
#include "TurboCM.hpp"
#include "X86Binary.hpp"

CompressorFactories* CompressorFactories::instance = nullptr;

static void printHeader() {
	std::cout
		<< "======================================================================" << std::endl
		<< "mcm compressor v" << Archive::Header::kCurMajorVersion << "." << Archive::Header::kCurMinorVersion
			<< ", by Mathieu Chartier (c)2015 Google Inc." << std::endl
		<< "Experimental, may contain bugs. Contact mathieu.a.chartier@gmail.com" << std::endl
		<< "Special thanks to: Matt Mahoney, Stephan Busch, Christopher Mattern." << std::endl
		<< "======================================================================" << std::endl;
}

class Options {
public:
	// Block size of 0 -> file size / #threads.
	static const uint64_t kDefaultBlockSize = 0;
	enum Mode {
		kModeUnknown,
		// Compress -> Decompress -> Verify.
		// (File or directory).
		kModeTest,
		// Compress infinite times with different opt vars.
		kModeOpt,
		// In memory test.
		kModeMemTest,
		// Single file test.
		kModeSingleTest,
		// Add a single file.
		kModeAdd,
		kModeExtract,
		kModeExtractAll,
		// Single hand mode.
		kModeCompress,
		kModeDecompress,
	};
	Mode mode;
	bool opt_mode;
	CompressionOptions options_;
	Compressor* compressor;
	uint32_t threads;
	uint64_t block_size;
	FilePath archive_file;
	std::vector<FilePath> files;

	Options()
		: mode(kModeUnknown)
		, opt_mode(false)
		, compressor(nullptr)
		, threads(1)
		, block_size(kDefaultBlockSize) {
	}
	
	int usage(const std::string& name) {
		printHeader();
		std::cout
			<< "Caution: Experimental, use only for testing!" << std::endl
			<< "Usage: " << name << " [command] [options] <infile> <outfile>" << std::endl
			<< "Options: d for decompress" << std::endl
			<< "-{t|f|m|h|x}{1 .. 11} compression option" << std::endl
			<< "t is turbo, f is fast, m is mid, h is high, x is max (default " << CompressionOptions::kDefaultLevel << ")" << std::endl
			<< "0 .. 11 specifies memory with 32mb .. 5gb per thread (default " << CompressionOptions::kDefaultMemUsage << ")" << std::endl
			<< "10 and 11 are only supported on 64 bits" << std::endl
			<< "-test tests the file after compression is done" << std::endl
			// << "-b <mb> specifies block size in MB" << std::endl
			// << "-t <threads> the number of threads to use (decompression requires the same number of threads" << std::endl
			<< "Examples:" << std::endl
			<< "Compress: " << name << " -m9 enwik8 enwik8.mcm" << std::endl
			<< "Decompress: " << name << " d enwik8.mcm enwik8.ref" << std::endl;
		return 0;
	}

	int parse(int argc, char* argv[]) {
		assert(argc >= 1);
		std::string program = trimExt(argv[0]);
		// Parse options.
		int i = 1;
		bool has_comp_args = false;
		for (;i < argc;++i) {
			const std::string arg = argv[i];
			Mode parsed_mode = kModeUnknown;
			if (arg == "-test") parsed_mode = kModeSingleTest; // kModeTest;
			else if (arg == "-memtest") parsed_mode = kModeMemTest;
			else if (arg == "-opt") parsed_mode = kModeOpt;
			else if (arg == "-stest") parsed_mode = kModeSingleTest;
			else if (arg == "c") parsed_mode = kModeCompress;
			else if (arg == "d") parsed_mode = kModeDecompress;
			else if (arg == "a") parsed_mode = kModeAdd;
			else if (arg == "e") parsed_mode = kModeExtract;
			else if (arg == "x") parsed_mode = kModeExtractAll;
			if (parsed_mode != kModeUnknown) {
				if (mode != kModeUnknown) {
					std::cerr << "Multiple commands specified" << std::endl;
					return 2;
				}
				mode = parsed_mode;
				switch (mode) {
				case kModeAdd:
				case kModeExtract:
				case kModeExtractAll:
					{
						if (++i >= argc) {
							std::cerr << "Expected archive" << std::endl;
							return 3;
						}
						// Archive is next.
						archive_file = FilePath(argv[i]);
						break;
					}
				}
			} else if (arg == "-opt") opt_mode = true;
			else if (arg == "-filter=none") options_.filter_type_ = kFilterTypeNone;
			else if (arg == "-filter=dict") options_.filter_type_ = kFilterTypeDict;
			else if (arg == "-filter=x86") options_.filter_type_ = kFilterTypeX86;
			else if (arg == "-filter=auto") options_.filter_type_ = kFilterTypeAuto;
			else if (arg == "-lzp=auto") options_.lzp_type_ = kLZPTypeAuto;
			else if (arg == "-lzp=true") options_.lzp_type_ = kLZPTypeEnable;
			else if (arg == "-lzp=false") options_.lzp_type_ = kLZPTypeDisable;
			else if (arg == "-b") {
				if  (i + 1 >= argc) {
					return usage(program);
				}
				std::istringstream iss(argv[++i]);
				iss >> block_size;
				block_size *= MB;
				if (!iss.good()) {
					return usage(program);
				}
			} else if (arg == "-store") {
				options_.comp_level_ = kCompLevelStore;
				has_comp_args = true;
			} else if (arg[0] == '-') {
				if (arg[1] == 't') options_.comp_level_ = kCompLevelTurbo;
				else if (arg[1] == 'f') options_.comp_level_ = kCompLevelFast;
				else if (arg[1] == 'm') options_.comp_level_ = kCompLevelMid;
				else if (arg[1] == 'h') options_.comp_level_ = kCompLevelHigh;
				else if (arg[1] == 'x') options_.comp_level_ = kCompLevelMax;
				else {
					std::cerr << "Unknown option " << arg << std::endl;
					return 4;
				}
				has_comp_args = true;
				const std::string mem_string = arg.substr(2);
				if (mem_string == "0") options_.mem_usage_ = 0;
				else if (mem_string == "1") options_.mem_usage_ = 1;
				else if (mem_string == "2") options_.mem_usage_ = 2;
				else if (mem_string == "3") options_.mem_usage_ = 3;
				else if (mem_string == "4") options_.mem_usage_ = 4;
				else if (mem_string == "5") options_.mem_usage_ = 5;
				else if (mem_string == "6") options_.mem_usage_ = 6;
				else if (mem_string == "7") options_.mem_usage_ = 7;
				else if (mem_string == "8") options_.mem_usage_ = 8;
				else if (mem_string == "9") options_.mem_usage_ = 9;
				else if (mem_string == "10" || mem_string == "11") {
					if (sizeof(void*) < 8) {
						std::cerr << arg << " only supported with 64 bit" << std::endl;
						return usage(program);
					}
					if (mem_string == "10") options_.mem_usage_ = 10;
					else options_.mem_usage_ = 11;
				}
				else if (!mem_string.empty()) {
					std::cerr << "Unknown mem level " << mem_string << std::endl;
					return 4;
				}
			} else if (!arg.empty()) {
				if (mode == kModeAdd || mode == kModeExtract) {
					// Read in files.
					files.push_back(FilePath(argv[i]));
				} else {
					// Done parsing.
					break;
				}
			}
		}
		if (mode == kModeUnknown && has_comp_args) {
			mode = kModeCompress;
		}
		const bool single_file_mode =
			mode == kModeCompress || mode == kModeDecompress || mode == kModeSingleTest ||
			mode == kModeMemTest || mode == kModeOpt;
		if (single_file_mode) {
			std::string in_file, out_file;
			// Read in file and outfile.
			if (i < argc) {
				in_file = argv[i++];
			}
			if (i < argc) {
				out_file = argv[i++];
			} else {
				if (mode == kModeDecompress) {
					out_file = in_file + ".decomp";
				} else {
					out_file = in_file + ".mcm";
				}
			}
			if (mode == kModeMemTest) {
				// No out file for memtest.
				files.push_back(FilePath(in_file));
			} else if (mode == kModeCompress || mode == kModeSingleTest || mode == kModeOpt) {
				archive_file = FilePath(out_file);
				files.push_back(FilePath(in_file));
			} else {
				archive_file = FilePath(in_file);
				files.push_back(FilePath(out_file));
			}
		}
		if (archive_file.isEmpty() || files.empty()) {
			std::cerr << "Error, input or output files missing" << std::endl;
			usage(program);
			return 5;
		}
		return 0;
	}
};

#if 0
void decompress(Stream* in, Stream* out) {
	Archive archive(in);

	Archive::Algorithm algo(in);
	ProgressThread thr(out, in, false);
	auto start = clock();
	std::unique_ptr<Compressor> comp(algo.createCompressor());
	comp->decompress(in, &f);
	f.flush();
	auto time = clock() - start;
	thr.done();
	std::cout << std::endl << "DeCompression took " << clockToSeconds(time) << "s" << std::endl;
}

void compress(Compressor* comp, File* in, Stream* out, size_t opt_var = 0) {
	
}
#endif

#if 0
int main(int argc, char* argv[]) {
	CompressorFactories::init();
	// runAllTests();
	Options options;
	auto ret = options.parse(argc, argv);
	if (ret) {
		std::cerr << "Failed to parse arguments" << std::endl;
		return ret;
	}
	switch (options.mode) {
	case Options::kModeMemTest: {
		const uint32_t iterations = kIsDebugBuild ? 1 : 1;
		// Read in the whole file.
		size_t length = 0;
		uint64_t long_length = 0;
		std::vector<uint64_t> lengths;
		for (const auto& file : options.files) {
			// lengths.push_back(getFileLength(file.getName()));
			long_length += lengths.back();
		}
		length = static_cast<uint32_t>(long_length);
		check(length < 300 * MB);
		auto in_buffer = new byte[length];
		// Read in the files.
		uint32_t index = 0;
		uint64_t read_pos = 0;
		for (const auto& file : options.files) {
			File f;
			f.open(file.getName(), std::ios_base::in | std::ios_base::binary);
			size_t count = f.read(in_buffer + read_pos, static_cast<size_t>(lengths[index]));
			check(count == lengths[index]);
			index++;
		}
		// Create the memory compressor.
		auto* compressor = new LZFast;
		//auto* compressor = new LZ4;
		//auto* compressor = new MemCopyCompressor;
		auto out_buffer = new byte[compressor->getMaxExpansion(length)];
		uint32_t comp_start = clock();
		uint32_t comp_size;
		static const bool opt_mode = false;
		if (opt_mode) {
			uint32_t best_size = 0xFFFFFFFF;
			uint32_t best_opt = 0;
			for (uint32_t opt = 0; ; ++opt) {
				compressor->setOpt(opt);
				comp_size = compressor->compressBytes(in_buffer, out_buffer, length);
				std::cout << "Opt " << opt << " / " << best_opt << " =  " << comp_size << "/" << best_size << std::endl;
				if (comp_size < best_size) {
					best_opt = opt;
					best_size = comp_size;
				}
			}
		} else {
			for (uint32_t i = 0; i < iterations; ++i) {
				comp_size = compressor->compressBytes(in_buffer, out_buffer, length);
			}
		}

		uint32_t comp_end = clock();
		std::cout << "Compression " << length << " -> " << comp_size << " = " << float(double(length) / double(comp_size)) << " rate: "
			<< prettySize(static_cast<uint64_t>(long_length * iterations / clockToSeconds(comp_end - comp_start))) << "/s" << std::endl;
		memset(in_buffer, 0, length);
		uint32_t decomp_start = clock();
		static const uint32_t decomp_iterations = kIsDebugBuild ? 1 : iterations * 5;
		for (uint32_t i = 0; i < decomp_iterations; ++i) {
			compressor->decompressBytes(out_buffer, in_buffer, length);
		}
		uint32_t decomp_end = clock();
		std::cout << "Decompression took: " << decomp_end - comp_end << " rate: "
			<< prettySize(static_cast<uint64_t>(long_length * decomp_iterations / clockToSeconds(decomp_end - decomp_start))) << "/s" << std::endl;
		index = 0;
		for (const auto& file : options.files) {
			File f;
			f.open(file.getName(), std::ios_base::in | std::ios_base::binary);
			uint32_t count = static_cast<uint32_t>(f.read(out_buffer, static_cast<uint32_t>(lengths[index])));
			check(count == lengths[index]);
			for (uint32_t i = 0; i < count; ++i) {
				if (out_buffer[i] != in_buffer[i]) {
					std::cerr << "File" << file.getName() << " doesn't match at byte " << i << std::endl;
					check(false);
				}
			}
			index++;
		}
		std::cout << "Decompression verified" << std::endl;
		break;
	}
	case Options::kModeSingleTest: {
		
	}
	case Options::kModeOpt:
	case Options::kModeCompress:
	case Options::kModeTest: {
		printHeader();

		int err = 0;
		File fin;
		File fout;
		auto in_file = options.files.back().getName();
		auto out_file = options.archive_file.getName();

		if (err = fin.open(in_file, std::ios_base::in | std::ios_base::binary)) {
			std::cerr << "Error opening: " << in_file << " (" << errstr(err) << ")" << std::endl;
			return 1;
		}

		if (options.mode == Options::kModeOpt) {
			std::cout << "Optimizing " << in_file << std::endl;
			uint64_t best_size = std::numeric_limits<uint64_t>::max();
			size_t best_var = 0;
			for (size_t opt_var = 0; ; ++opt_var) {
				const clock_t start = clock();
				fin.seek(0);
				VoidWriteStream fout;
				Archive archive(&fout, options.options_);
				if (!archive.setOpt(opt_var)) {
					continue;
				}
				archive.compress(&fin);
				clock_t time = clock() - start;
				const auto size = fout.tell();
				if (size < best_size) {
					best_size = size;
					best_var = opt_var;
				}
				std::cout << "opt_var=" << opt_var << " best=" << best_var << "(" << formatNumber(best_size) << ") "
					<< formatNumber(fin.tell()) << "->" << formatNumber(size) << " took " << std::setprecision(3)
					<< clockToSeconds(time) << "s" << std::endl;
			}
		} else {
			const clock_t start = clock();
			if (err = fout.open(out_file, std::ios_base::out | std::ios_base::binary)) {
				std::cerr << "Error opening: " << out_file << " (" << errstr(err) << ")" << std::endl;
				return 2;
			}

			std::cout << "Compressing to " << out_file << " mode=" << options.options_.comp_level_ << " mem=" << options.options_.mem_usage_ << std::endl;

			{
				Archive archive(&fout, options.options_);
				archive.compress(&fin);
			}
			clock_t time = clock() - start;
			fin.seek(0, SEEK_END);
			const uint64_t file_size = fin.tell();
			std::cout << "Compressed " << formatNumber(fin.tell()) << "->" << formatNumber(fout.tell())
				<< " in " << std::setprecision(3) << clockToSeconds(time) << "s"
				<< " bpc=" << double(fout.tell()) * 8.0 / double(fin.tell()) << std::endl;
			std::cout << "Avg rate: " << std::setprecision(3) << double(time) * (1000000000.0 / double(CLOCKS_PER_SEC)) / double(fin.tell()) << " ns/B" << std::endl;

			fout.close();
			
			if (options.mode == Options::kModeSingleTest) {
				if (err = fout.open(out_file, std::ios_base::in | std::ios_base::binary)) {
					std::cerr << "Error opening: " << out_file << " (" << errstr(err) << ")" << std::endl;
					return 1;
				}
				Archive archive(&fout);
				std::cout << "Decompresing & verifying file" << std::endl;		
				fin.seek(0);
				VerifyStream verifyStream(&fin, file_size);
				archive.decompress(&verifyStream);
				verifyStream.summary();
			}
			fin.close();
		}
		break;
#if 0
		// Note: Using overwrite, this is dangerous.
		archive.open(options.archive_file, true, std::ios_base::in | std::ios_base::out);
		// Add the files to the archive so we can compress the segments.
		archive.addNewFileBlock(options.files);
		assert(options.files.size() == 1U);
		// Split the files into as many blocks as we have threads.
		auto files = Archive::splitFiles(options.files, options.threads, 64 * KB);
		MultiCompressionJob multi_job;
		std::vector<Archive::Job*> jobs(files.size());
		// Compress the files in the main thread.
		archive.compressFiles(0, &files[0], 4);
		break;
#endif
	}
	case Options::kModeAdd: {
		// Add a single file.
		break;
	}
	case Options::kModeDecompress: {
		auto in_file = options.archive_file.getName();
		auto out_file = options.files.back().getName();
		File fin;
		File fout;
		int err = 0;
		if (err = fin.open(in_file, std::ios_base::in | std::ios_base::binary)) {
			std::cerr << "Error opening: " << out_file << " (" << errstr(err) << ")" << std::endl;
			return 1;
		}
		if (err = fout.open(out_file, std::ios_base::out | std::ios_base::binary)) {
			std::cerr << "Error opening: " << out_file << " (" << errstr(err) << ")" << std::endl;
			return 2;
		}
		printHeader();
		std::cout << "Decompresing file " << in_file << std::endl;
		Archive archive(&fin);
		const auto& header = archive.getHeader();
		if (!header.isArchive()) {
			std::cerr << "Attempting to decompress non archive file" << std::endl;
			return 1;
		}
		if (!header.isSameVersion()) {
			std::cerr << "Attempting to decompress old version " << header.majorVersion() << "." << header.minorVersion() << std::endl;
			return 1;
		}
		archive.decompress(&fout);
		fin.close();
		fout.close();
		// Decompress the single file in the archive to the output out.
		break;
	}
	case Options::kModeExtract: {
		// Extract a single file from multi file archive .
		break;
	}
	case Options::kModeExtractAll: {
		// Extract all the files in the archive.
		break;
	}
	}
	return 0;
}
#endif
