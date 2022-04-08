#pragma once
#include <vector>

#include "types.h"
#include "tensor_utils.h"

class views
{
public:
	views() : t_size(0), d_size(0)
	{
	}

	explicit views(_int, dim_vec);
	//views(views&) = default;
	views(views&&) noexcept;
	views(const views&);

	views& operator=(const views&) = default;

	views select(int, _int) const;
	//	views select(const std::vector<std::string>&) const;

	[[nodiscard]] _int ndim() const { return mShape.size(); }
	views reshape(std::vector<int>&) const;
	views reshape(const std::vector<int>&) const;
	[[nodiscard]] _int size_in_bytes() const { return t_size * d_size; }
	[[nodiscard]] _int size(_int idx = 0) const { return mSize[idx]; }
	[[nodiscard]] _int shape(size_t idx) const { return mShape[idx]; }
	dim_vec& shape() { return mShape; }

	offset offset;

private:
	_int t_size;
	_int d_size;

	dim_vec mShape;
	dim_vec mStride;
	dim_vec mSize;
};
