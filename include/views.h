#pragma once
#include <vector>

#include "types.h"
#include "tensor_utils.h"

class views
{
public:
	views() : t_size(0), d_size(0) {}
	explicit views(_int, dim_vec);
	//views(views&) = default;
	views(views&&) noexcept;
	views(const views&);

	views& operator=(const views&) = default;

	views select(int, _int) const;
//	views select(const std::vector<std::string>&) const;

	[[nodiscard]] _int ndim() const { return shape_.size(); }
	views& reshape(std::vector<int>&);
	views& reshape(const std::vector<int>&);
	[[nodiscard]] _int size_in_bytes() const { return t_size * d_size; }
	[[nodiscard]] _int size(_int idx = 0) const { return size_[idx]; }
	dim_vec& shape() { return shape_; }

	offset offset;

private:
	_int t_size;
	_int d_size;

	dim_vec shape_;
	dim_vec stride_;
	dim_vec size_;

};
