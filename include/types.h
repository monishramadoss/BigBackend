#pragma once
#include<vector>

enum class Format
{
	kFormatInvalid = -1,

	kFormatFp16 = 0,
	kFormatFp32 = 1,
	kFormatFp64 = 2,

	kFormatInt8 = 3,
	kFormatInt16 = 4,
	kFormatInt32 = 5,
	kFormatInt64 = 6,

	kFormatUInt8 = 7,
	kFormatUInt16 = 8,
	kFormatUInt32 = 9,
	kFormatUInt64 = 10,

	kFormatBool = 11,
	kFormatNone = -1
};



inline char element_size(Format fmt)
{
	if (fmt == Format::kFormatFp32 || fmt == Format::kFormatInt32 || fmt == Format::kFormatBool)
		return 4;
	if (fmt == Format::kFormatFp64 || fmt == Format::kFormatInt64)
		return 8;
	if (fmt == Format::kFormatFp16 || fmt == Format::kFormatInt16)
		return 2;
	if (fmt == Format::kFormatInt8 || fmt == Format::kFormatUInt8)
		return 1;
	if (fmt < Format::kFormatNone && fmt >= Format::kFormatFp16)
		printf("Unsupported format %d", fmt);
	else
		printf("Invalid format %d", fmt);
	return 0;
}

typedef size_t _int;
typedef char byte_;

using offset = std::vector<std::pair<_int, _int>>;
using dim_vec = std::vector<_int>;
using data_buffer = std::vector<byte_>;
