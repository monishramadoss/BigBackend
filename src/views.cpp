#include <algorithm>
#include <vector>

#include "views.h"


views::views(_int type_size, dim_vec dst_shape) : t_size(type_size), mShape(std::move(dst_shape))
{
	rewrite_stride_size(mShape, mStride, mSize);
	d_size = mSize[0];
}

//views::views(views& v) : t_size(v.t_size), d_size(v.d_size),
//shape_(std::move(v.shape_)), stride_(std::move(v.stride_)),  size_(std::move(v.size_)) { 

views::views(): t_size(0), d_size(0)
{
}

views& views::operator=(const views&) = default;

_int views::ndim() const
{
	return mShape.size();
}

_int views::size_in_bytes() const
{
	return t_size * d_size;
}

_int views::size(_int idx) const
{
	return mSize[idx];
}

_int views::shape(size_t idx) const
{
	return mShape[idx];
}

dim_vec& views::shape()
{
	return mShape;
}

views::views(views&& v) noexcept : offset(std::move(v.offset)), t_size(v.t_size), d_size(v.d_size),
                                   mShape(std::move(v.mShape)), mStride(std::move(v.mStride)), mSize(std::move(v.mSize))
{
}

views::views(const views& v) : t_size(v.t_size), d_size(v.d_size)
{
	offset.resize(v.offset.size());
	mShape.resize(v.ndim());
	mStride.resize(v.ndim());
	mSize.resize(v.ndim() + 1);

	std::copy(v.offset.begin(), v.offset.end(), offset.begin());
	std::copy(v.mShape.begin(), v.mShape.end(), mShape.begin());
	std::copy(v.mStride.begin(), v.mStride.end(), mStride.begin());
	std::copy(v.mSize.begin(), v.mSize.end(), mSize.begin());
};


views views::reshape(std::vector<int>& new_shape) const
{
	std::vector<_int> _new_shape(new_shape.size(), 0);
	int _size = std::accumulate<std::vector<int>::const_iterator, int>(new_shape.begin(), new_shape.end(), 1,
	                                                                   std::multiplies<int>());

	for (_int i = 0; i < new_shape.size(); ++i)
	{
		if (new_shape[i] < 0)
			_new_shape[i] = mSize[0] / static_cast<_int>(-_size);
		else
			_new_shape[i] = new_shape[i];
	}

	if (_size < 0)
		_size = std::accumulate<std::vector<int>::const_iterator, int>(new_shape.begin(), new_shape.end(), 1,
		                                                               std::multiplies<int>());

	if (d_size != _size)
		throw std::bad_alloc();

	return views(t_size, _new_shape);
}

views views::reshape(const std::vector<int>& new_shape) const
{
	std::vector<_int> _new_shape(new_shape.size(), 0);
	int _size = std::accumulate<std::vector<int>::const_iterator, int>(new_shape.begin(), new_shape.end(), 1,
	                                                                   std::multiplies<int>());

	for (_int i = 0; i < new_shape.size(); ++i)
	{
		if (new_shape[i] < 0)
			_new_shape[i] = mSize[0] / static_cast<_int>(-_size);
		else
			_new_shape[i] = new_shape[i];
	}

	if (_size < 0)
		_size = std::accumulate<std::vector<int>::const_iterator, int>(new_shape.begin(), new_shape.end(), 1,
		                                                               std::multiplies<int>());

	if (d_size != _size)
		throw std::bad_alloc();

	return views(t_size, _new_shape);
}


views views::select(int axis, _int idx) const
{
	std::vector<_int> dst_shape = mShape;
	dst_shape.erase(dst_shape.begin() + axis);
	if (dst_shape.empty())
		dst_shape.push_back(1);

	views view(t_size, dst_shape);
	axis_idx(axis, idx, t_size, mShape, mStride, view.offset);
	return view;
}
