#include "Util.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>

bool fileExists(const char* name) {
    std::ifstream fin(name, std::ios_base::in);
    return fin.good();
}


std::string prettySize(uint64_t size) {
	uint64_t units;
	const char* name;
	if (size >= GB) {
		units = GB;
		name = "GB";
	} else if (size >= MB) {
		units = MB;
		name = "MB";
	} else if (size >= KB) {
		units = KB;
		name = "KB";
	} else {
		units = 1;
		name = "B";
	}
	std::ostringstream oss;
	oss << static_cast<float>(static_cast<double>(size) / static_cast<double>(units)) << name;
	return oss.str();
}

double clockToSeconds(clock_t c) {
	return double(c) / double(CLOCKS_PER_SEC);
}

std::string errstr(int err) {
#ifdef _MSC_VER
	char buffer[1024];
	strerror_s(buffer, sizeof(buffer), err);
	return buffer;
#else
	// return strerror(err);
	return "UNIMPLEMENTED";
#endif
}

std::vector<byte> randomArray(uint32_t size) {
	std::vector<byte> ret;
	for (uint32_t i = 0; i < size; ++i) {
		ret.push_back(rand() % 256);
	}
	return ret;
}

uint64_t computeRate(uint64_t size, uint64_t delta_time) {
	if (delta_time == 0) {
		return 0;
	}
	double seconds = static_cast<double>(delta_time) / static_cast<double>(CLOCKS_PER_SEC);
	return static_cast<uint64_t>(size / seconds);
}

std::vector<byte> loadFile(const std::string& name, uint32_t max_size) {
	std::vector<byte> ret;
	std::ifstream fin(name.c_str(), std::ios_base::in | std::ios_base::binary);
	for (uint32_t i = 0; i < max_size; ++i) {
		int c = fin.get();
		if (fin.eof()) {
			break;
		}
		ret.push_back(static_cast<byte>(static_cast<uint>(c)));
	}
	return ret;
}

std::string formatNumber(uint64_t n) {
	std::string ret;
	while (n >= 1000) {
		std::ostringstream oss;
		oss << std::setw(3) << std::setfill('0') << n % 1000;
		ret = "," + oss.str() + ret;
		n /= 1000;
	}
	return std::to_string(n) + ret;
}

std::string trimExt(const std::string& str) {
	std::streamsize start = 0, pos;
	if ((pos = str.find_last_of('\\')) != std::string::npos) {
		start = std::max(start, pos + 1);
	}
	if ((pos = str.find_last_of('/')) != std::string::npos) {
		start = std::max(start, pos + 1);
	}
	return str.substr(static_cast<uint32_t>(start));
}
