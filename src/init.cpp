#include "init.h"

#include <cmath>
#include <iostream>


tensor filln(std::vector<_int>& shape, float n)
{
	auto* data = data_filln<float>(shape, n);
	return tensor(data, shape, Format::kFormatFp32);
}

tensor filln(std::vector<_int>& shape, int n)
{
	auto* data = data_filln<int>(shape, n);
	return tensor(data, shape, Format::kFormatInt32);
}

tensor filln(std::vector<_int>& shape, bool n)
{
	auto* data = data_filln<bool>(shape, n);
	return tensor(data, shape, Format::kFormatBool);
}


tensor zeros(std::vector<_int>& shape)
{
	auto* data = data_zeros<float>(shape);
	return tensor(data, shape);
}

tensor ones(std::vector<_int>& shape)
{
	auto* data = data_ones<float>(shape);
	return tensor(data, shape);
}

tensor arange(int min, int max, int step)
{
	if (max < min)
		std::cerr << "ERROR IN ARANGE" << std::endl;
	auto* data = data_arange<float>(min, max, step);
	double _tmp = (max - min) / step;
	_int data_size = static_cast<_int>(ceil(_tmp));
	return tensor(data, {data_size});
}

tensor arange(int max)
{
	auto* data = data_arange<float>(max);
	return tensor(data, {static_cast<_int>(max)});
}
