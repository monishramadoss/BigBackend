#include <algorithm>
#include <vector>

#include "views.h"


views::views(_int type_size, dim_vec dst_shape) : t_size(type_size), shape_(std::move(dst_shape))
{
	rewrite_stride_size(shape_, stride_, size_);
	d_size = size_[0];
}

//views::views(views& v) : t_size(v.t_size), d_size(v.d_size),
//shape_(std::move(v.shape_)), stride_(std::move(v.stride_)),  size_(std::move(v.size_)) { 

views::views(views&& v) noexcept : offset(std::move(v.offset)), t_size(v.t_size), d_size(v.d_size),
shape_(std::move(v.shape_)), stride_(std::move(v.stride_)), size_(std::move(v.size_)) { }

views::views(const views& v) = default;
// : offset(v.offset), t_size(v.t_size), d_size(v.d_size),
// shape_(v.shape_), stride_(v.stride_), size_(v.size_) { }

views& views::reshape(std::vector<int>& S) {
	int _size = std::accumulate<std::vector<int>::const_iterator, int>(S.begin(), S.end(), 1, std::multiplies<int>());

	for (int& i : S)
	{
		if (i == -1) {
			i = -static_cast<int>(size_[0]) / _size;
			break;
		}
	}

	_size = std::accumulate<std::vector<int>::const_iterator, int>(S.begin(), S.end(), 1, std::multiplies<int>());

	if (count(shape_) != _size)
		throw std::bad_alloc();

	shape_.clear(); stride_.clear(); size_.clear();

	shape_.resize(S.size());
	for (_int i = 0; i < S.size(); ++i) {
		shape_[i] = static_cast<_int>(S[i]);
	}
	rewrite_stride_size(shape_, stride_, size_);
	return *this;
}

views& views::reshape(const std::vector<int>& S) {
	std::vector<_int> _s(S.size(), 0);
	int _size = std::accumulate<std::vector<int>::const_iterator, int>(S.begin(), S.end(), 1, std::multiplies<int>());

	for (_int i = 0; i < S.size(); ++i)
	{
		if (S[i] < 0)
			_s[i] = size_[0] / static_cast<_int>(-_size);
		else
			_s[i] = S[i];
	}

	if (_size < 0)
		_size = std::accumulate<std::vector<int>::const_iterator, int>(S.begin(), S.end(), 1, std::multiplies<int>());

	if (d_size != _size)
		throw std::bad_alloc();

	shape_.clear(); stride_.clear(); size_.clear();

	shape_.resize(S.size());
	shape_ = _s;

	rewrite_stride_size(shape_, stride_, size_);
	return *this;
}


views views::select(int axis, _int idx) const
{
	std::vector<_int> dst_shape = shape_;
	dst_shape.erase(dst_shape.begin() + axis);
	if (dst_shape.empty())
		dst_shape.push_back(1);
		
	views view(t_size, dst_shape);
	axis_idx(axis, idx, t_size, shape_, stride_, view.offset);
	return view;
}

