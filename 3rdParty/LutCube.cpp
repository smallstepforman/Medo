#include <cctype>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <locale>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include "LutCube.h"

namespace timecube {
namespace {

// 5.3  A line of text shall not be longer than 250 bytes. Lines of text do not contain newline characters.
constexpr size_t LINE_LEN = 250 + 1 + 1;

struct FileCloser {
	void operator()(std::FILE *f) { std::fclose(f); }
};

[[noreturn]] void throw_system_error()
{
	throw std::system_error{ errno, std::system_category() };
}

void read_line(char *buf, FILE *f)
{
	do {
		if (!std::fgets(buf, LINE_LEN, f)) {
			if (std::feof(f))
				throw std::runtime_error{ "end of file" };
			else
				throw_system_error();
		}
	} while (buf[0] == '#' || buf[0] == '\n');
}

const char *skip_space(const char *buf)
{
	while (*buf && (*buf == ' ' || *buf == '\t')) {
		++buf;
	}
	return buf;
}

template <class T>
const char *parse_number(const char *buf, T *dst)
{
	const char *first = buf;

	while (*buf != '\0' && *buf != ' ' && *buf != '\t' && *buf != '\n') {
		++buf;
	}

	std::istringstream ss{ std::string{ first, buf } };
	ss.imbue(std::locale::classic());

	char eof_test;
	if (!(ss >> *dst) || (ss >> eof_test))
		throw std::runtime_error{ "invalid number" };

	return buf;
}

bool is_keyword(const char *buf, const char *keyword)
{
	size_t len = std::strlen(keyword);
	return !std::strncmp(buf, keyword, len) && (buf[len] == ' ' || buf[len] == '\t' || buf[len] == '\n');
}

std::string parse_title(const char *buf)
{
	const char *first;

	buf += std::strlen("TITLE");
	buf = skip_space(buf);

	if (*buf++ != '"')
		throw std::runtime_error{ "missing opening quote in TITLE" };

	first = buf;

	if (!(buf = std::strchr(buf, '"')))
		throw std::runtime_error{ "missing closing quote in TITLE" };

	return{ first, buf };
}

void parse_domain_minmax(const char *buf, float dst[3])
{
	buf += std::strlen("DOMAIN_MIN");
	buf = skip_space(buf);

	buf = parse_number(buf, dst + 0);
	buf = skip_space(buf);
	buf = parse_number(buf, dst + 1);
	buf = skip_space(buf);
	buf = parse_number(buf, dst + 2);
}

uint_least32_t parse_lut_size(const char *buf)
{
	uint_least32_t n;

	buf += std::strlen("LUT_1D_SIZE");
	buf = skip_space(buf);
	buf = parse_number(buf, &n);

	return n;
}

void parse_lut_entry(const char *buf, float dst[3])
{
	buf = parse_number(buf, dst + 0);
	buf = skip_space(buf);
	buf = parse_number(buf, dst + 1);
	buf = skip_space(buf);
	buf = parse_number(buf, dst + 2);
}

size_t lut_size(uint_least32_t n, bool is_3d)
{
	uint_least32_t size = is_3d ? n * n * n : n;
#if UINT_LEAST32_MAX > SIZE_MAX
	if (size > SIZE_MAX)
		throw std::length_error{ "LUT exceeds memory capacity" };
#endif
	return size;
}

} // namespace


Cube read_cube_from_file(const char *path)
{
	Cube cube;

	std::unique_ptr<std::FILE, FileCloser> file_uptr{ std::fopen(path, "r") };
	std::FILE *file = file_uptr.get();
	char buf[LINE_LEN];

	if (!file)
		throw_system_error();

	// Headers.
	bool has_lut_size = false;

	while (true) {
		read_line(buf, file);

		if (is_keyword(buf, "TITLE")) {
			try {
				cube.title = parse_title(buf);
			} catch (...) {
				// Non-fatal.
			}
		} else if (is_keyword(buf, "DOMAIN_MIN")) {
			parse_domain_minmax(buf, cube.domain_min);
		} else if (is_keyword(buf, "DOMAIN_MAX")) {
			parse_domain_minmax(buf, cube.domain_max);
		} else if (is_keyword(buf, "LUT_1D_SIZE")) {
			if (has_lut_size)
				throw std::runtime_error{ "duplicate LUT declaration" };

			cube.n = parse_lut_size(buf);
			cube.is_3d = false;
			has_lut_size = true;
		} else if (is_keyword(buf, "LUT_3D_SIZE")) {
			if (has_lut_size)
				throw std::runtime_error{ "duplicate LUT declaration" };

			cube.n = parse_lut_size(buf);
			cube.is_3d = true;
			has_lut_size = true;
		} else if (std::isdigit(buf[0], std::locale::classic()) || buf[0] == '+' || buf[0] == '-' || buf[0] == '.') {
			break;
		}
	}
	if (!has_lut_size)
		throw std::runtime_error{ "missing LUT declaration" };

	if (cube.n < 2 || cube.n > (cube.is_3d ? 256U : 65536U))
		throw std::runtime_error{ "invalid LUT size" };
	if (cube.domain_min[0] > cube.domain_max[0] || cube.domain_min[1] > cube.domain_max[1] || cube.domain_min[2] > cube.domain_max[2])
		throw std::runtime_error{ "invalid domain" };

	// LUT.
	size_t size = lut_size(cube.n, cube.is_3d);

	cube.lut.insert(cube.lut.end(), 3, 0.0f);
	parse_lut_entry(buf, &*(cube.lut.end() - 3));

	for (unsigned i = 1; i < size; ++i) {
		read_line(buf, file);

		cube.lut.insert(cube.lut.end(), 3, 0.0f);
		parse_lut_entry(buf, &*(cube.lut.end() - 3));
	}

	return cube;
}

} // namespace timecube
