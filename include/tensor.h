#pragma once
#include <vector>
#include <memory>
#include <algorithm>


#include "device.h"
#include "types.h"
#include "storage.h"
#include "views.h"

class compute_job;

class tensor
{
public:
	tensor();
	tensor(byte_* data, std::vector<_int>& shape, Format fmt = Format::kFormatFp32);
	tensor(byte_* data, const std::vector<_int>& shape, Format fmt = Format::kFormatFp32);
	tensor(views, tensor*, const Format);
	
	tensor(tensor&& t);
	tensor(const tensor& t);

	tensor& operator=(const tensor& t);
	tensor& operator=(tensor&&);

	~tensor() = default;

	tensor reshape(std::vector<int>& new_shape);
	tensor reshape(const std::vector<int>& new_shape);

	tensor& operator[](const _int i);

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

	void set_src_kernel(compute_job* src) { src_kernel = src; }
	[[nodiscard]] compute_job* get_src_kernel() const { return src_kernel; }
	void set_dst_kernel(compute_job* dst) { dst_kernel = dst; }
	[[nodiscard]] compute_job* get_dst_kernel() const { return dst_kernel; }

	[[nodiscard]] tensor* get_parent() const { return parent.get(); }

private:
	//std::shared_ptr<tensor> gradient;
	std::shared_ptr<tensor> parent;
	std::vector<tensor> children;

	std::vector<int> shard_state;
	std::shared_ptr<base_storage> mdata;
	[[nodiscard]] byte_* get_storage_data() const;
	[[nodiscard]] std::pair<_int, _int> get_offset() const;
	void clear_storage();
	friend compute_job;
	compute_job* src_kernel = nullptr;
	compute_job* dst_kernel = nullptr;
	views view;
	_int size_in_bytes = 0;

};
