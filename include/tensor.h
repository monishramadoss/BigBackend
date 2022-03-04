#pragma once
#include <vector>
#include <memory>
#include <algorithm>

#include "device.h"
#include "types.h"
#include "storage.h"
#include "views.h"


class tensor
{
public:
	tensor() : dtype(Format::kFormatInvalid), size_in_bytes(0) { }
	explicit tensor(byte_* data, std::vector<_int>& shape, Format fmt = Format::kFormatFp32);
	explicit tensor(byte_* data, const std::vector<_int>& shape, Format fmt = Format::kFormatFp32);
	explicit tensor(views, tensor&, const Format);
	//tensor(tensor&) = default;
	tensor(tensor&&) noexcept;
	tensor(const tensor&);

	tensor& operator=(const tensor&) = default;

	[[nodiscard]] _int data_size() const { return size_in_bytes; }
	[[nodiscard]] _int ndim() const { return view.ndim(); }

	byte_* get_data();
	void set_data(const byte_*, _int) const;

	const dim_vec& shape() { return view.shape(); }

	tensor& reshape(std::vector<int>& new_shape)
	{
		view.reshape(new_shape);
		return *this;
	}

	tensor& reshape(const std::vector<int>& new_shape)
	{
		view.reshape(new_shape);
		return *this;
	}
		
	tensor operator[](const _int i)
	{
		const auto v = view.select(0, i);
		return tensor(v, *this, dtype);
	}

	Format dtype;
private:
	//std::shared_ptr<tensor> gradient;
	std::shared_ptr<tensor> parent;

	std::vector<int> shard_state;
	std::shared_ptr<base_storage> data_;
	[[nodiscard]] byte_* get_storage_data() const;
	[[nodiscard]] std::pair<_int, _int> get_offset() const;

	views view;


	_int size_in_bytes;

};
