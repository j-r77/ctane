#ifndef INC_UTIL_STRINGUTIL_H
#define INC_UTIL_STRINGUTIL_H

#include <sstream>
#include <cstdarg>

// trim from start
static inline std::string &ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
									std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
						 std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
	return ltrim(rtrim(s));
}

inline
std::string concat(int count, ...) {
	std::stringstream ss;
	va_list args;
    va_start(args, count);
	for (int i = 0; i < count; i++) {
		ss << va_arg(args, const char*);
	}
	va_end(args);
	return ss.str();
}

inline
std::string concatCsv(int count, ...) {
	std::stringstream ss;
	va_list args;
    va_start(args, count);
	for (int i = 0; i < count; i++) {
		ss << va_arg(args, const char*);
		if (i < count - 1) {
			ss << ",";
		}
	}
	va_end(args);
	return ss.str();
}
#endif