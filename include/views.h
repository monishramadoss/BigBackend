#pragma once
#include <vector>

#include "types.h"
#include "tensor_utils.h"

class views
{
public:
	views();

	explicit views(_int, dim_vec);
	//views(views&) = default;
	views(views&&) noexcept;
	views(const views&);

	views& operator=(const views&);

	views select(int, _int) const;
	//	views select(const std::vector<std::string>&) const;

	[[nodiscard]] _int ndim() const;
	views reshape(std::vector<int>&) const;
	views reshape(const std::vector<int>&) const;
	[[nodiscard]] _int size_in_bytes() const;
	[[nodiscard]] _int size(_int idx = 0) const;
	[[nodiscard]] _int shape(size_t idx) const;
	dim_vec& shape();

	offset _offset;

private:
	_int t_size;
	_int d_size;

	dim_vec mShape;
	dim_vec mStride;
	dim_vec mSize;
};
