#pragma once
#include <functional>
#include <numeric>
#include <vector>

#include "types.h"
#include "tensor.h"

inline _int size(dim_vec& shape)
{
	return std::accumulate<dim_vec::const_iterator, _int>(shape.begin(), shape.end(), 1, std::multiplies<_int>());
}

template <typename T>
byte_* data_filln(dim_vec& shape, T c)
{
	const _int data_size = size(shape);
	T* dst = new T[data_size];
	std::fill_n<T*, _int, T>(dst, data_size, c);
	return reinterpret_cast<byte_*>(dst);
}

template <typename T>
byte_* data_zeros(dim_vec& shape)
{
	return data_filln<T>(shape, T(0));
}

template <typename T>
byte_* data_ones(dim_vec& shape)
{
	return data_filln<T>(shape, T(1));
}

template <typename T>
byte_* data_arange(int64_t min, int64_t max, int64_t step = 1)
{
	const auto data_size = static_cast<_int>(ceil((max - min) / step));

	T* dst = new T[data_size];
	for (int64_t i = min; i < max; i += step)
		dst[i] = T(i);


	return reinterpret_cast<byte_*>(dst);
}

template <typename T>
byte_* data_arange(int max)
{
	return data_arange<T>(0, max, 1);
}


tensor filln(dim_vec& shape, float n);
tensor filln(dim_vec& shape, int n);
tensor filln(dim_vec& shape, bool n);


tensor zeros(dim_vec& shape);
tensor ones(dim_vec& shape);
tensor arange(int min, int max, int step = 1);
tensor arange(int max);
