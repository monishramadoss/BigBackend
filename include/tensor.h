#pragma once
#include <vector>
#include <memory>
#include <algorithm>


#include "device.h"
#include "types.h"
#include "storage.h"
#include "views.h"
#include "compute.h"

class compute_job;

class tensor : public std::enable_shared_from_this<tensor>
{
public:
	tensor();
	tensor(byte_* data, std::vector<_int>& shape, Format fmt = Format::kFormatFp32);
	tensor(byte_* data, const std::vector<_int>& shape, Format fmt = Format::kFormatFp32);
	tensor(views, tensor*, Format);

	tensor(const tensor& t);
	tensor(tensor&& t) noexcept;

	tensor& operator=(const tensor& t);
	tensor& operator=(tensor&&) noexcept;

	~tensor();

	tensor reshape(std::vector<int>& new_shape);
	tensor reshape(const std::vector<int>& new_shape);

	tensor& operator[](_int i);

	tensor operator+(tensor& w);
	tensor operator-(tensor& w);
	tensor operator*(tensor& w);
	tensor operator/(tensor& w);

	size_t local_tensor_id{};
	Format dtype = Format::kFormatInvalid;

	[[nodiscard]] _int data_size() const { return size_in_bytes; }
	[[nodiscard]] _int ndim() const { return view.ndim(); }
	[[nodiscard]] _int shape(int idx) const;
	dim_vec& shape() { return view.shape(); }

	byte_* get_data();
	void set_data(const byte_*, _int = 0) const;
	void set_data(tensor&) const;
	[[nodiscard]] tensor* get_parent() const { return parent.get(); }
	std::shared_ptr<tensor> getptr() { return shared_from_this(); }

	[[nodiscard]] static std::shared_ptr<tensor> create() {
		return std::make_shared<tensor>();
	}

private:
	//std::shared_ptr<tensor> gradient;
	std::shared_ptr<tensor> parent;
	std::vector<tensor> children;

	std::vector<int> shard_state;
	std::shared_ptr<base_storage> mdata;
	[[nodiscard]] byte_* get_storage_data() const;
	[[nodiscard]] std::pair<_int, _int> get_offset() const;
	void clear_storage();

	views view;
	_int size_in_bytes = 0;
};
