#include "tensor.h"
#include "storage_manager.h"
//#include <mpi.h>

/*
	float float_values[data_string.size()/sizeof(float)]; // 513
	memcpy(float_values, data_string.data(), data_string.size());
 */


class storage;

tensor::tensor() = default;

tensor::tensor(byte_* src, std::vector<_int>& shape, Format fmt) : local_tensor_id(++global_tensor_id), dtype(fmt),
                                                                   view(element_size(fmt), shape),
                                                                   size_in_bytes(view.size_in_bytes())
{
	auto* dst = global_store_manager.allocate_block(size_in_bytes);
	data_ptr = std::make_shared<byte_*>(dst);
	global_store_manager.copy_block(dst, src, size_in_bytes);
	global_store_manager.free_block(src);
}

tensor::tensor(byte_* src, const std::vector<_int>& shape, Format fmt) : local_tensor_id(++global_tensor_id),
                                                                         dtype(fmt), view(element_size(fmt), shape),
                                                                         size_in_bytes(view.size_in_bytes())
{
	auto* dst = global_store_manager.allocate_block(size_in_bytes);
	data_ptr = std::make_shared<byte_*>(dst);
	global_store_manager.copy_block(dst, src, size_in_bytes);
	global_store_manager.free_block(src);
}

tensor::tensor(views v, tensor* ptr, const Format fmt) : local_tensor_id(++global_tensor_id), dtype(fmt),
                                                         view(std::move(v)), size_in_bytes(view.size_in_bytes())
{
	if (this != ptr && parent == nullptr)
	{
		parent.reset(ptr);
		ptr->children.push_back(*this);
	}
}

tensor::tensor(tensor&& t) noexcept = default;

tensor::tensor(const tensor& t) : local_tensor_id(t.local_tensor_id), dtype(t.dtype),
                                  children(t.children), view(t.view), size_in_bytes(t.size_in_bytes)
{
	// std::cout << "Tensor Copy Operator" << std::endl;
	if (parent == nullptr)
		parent = t.parent;
	data_ptr = t.data_ptr;
}

tensor& tensor::operator=(const tensor& t)
{
	// std::cout << "Tensor Assign Operator" << std::endl;
	if (this == &t && local_tensor_id == t.local_tensor_id)
		return *this;

	parent = std::make_shared<tensor>(tensor(*t.parent));

	local_tensor_id = t.local_tensor_id;
	children = t.children;
	data_ptr = t.data_ptr;
	view = t.view;
	dtype = t.dtype;
	size_in_bytes = t.size_in_bytes;
	return *this;
}

tensor& tensor::operator=(tensor&& t) noexcept
{
	// std::cout << "Tensor Assigned Move Operator" << std::endl;
	if (this == &t)
		return *this;

	if (t.parent != nullptr)
		parent = std::make_shared<tensor>(*t.parent);

	local_tensor_id = t.local_tensor_id;
	children = std::move(t.children);
	data_ptr = t.data_ptr;
	view = t.view;
	dtype = t.dtype;
	size_in_bytes = t.size_in_bytes;
	return *this;
}

tensor::~tensor()
{
	if (data_ptr != nullptr)
		global_store_manager.free_block(*data_ptr);
};

std::pair<_int, _int> tensor::get_offset() const
{
	std::pair<_int, _int> o;

	for (const auto& off : view._offset)
	{
		o.first += off.first;
		o.second = off.second;
	}

	for (std::shared_ptr<tensor> p = parent; p != nullptr; p = p->parent)
	{
		for (const auto& [fst, snd] : p->view._offset)
			o.first += fst;
	}
	return o;
}

void tensor::clear_storage()
{
	if (data_ptr != nullptr || *data_ptr != nullptr)
		global_store_manager.free_block(*data_ptr);
	//mdata.reset();
}

_int tensor::shape(int idx) const
{
	if (idx == -1)
		idx = static_cast<int>(view.ndim()) - 1;
	return view.shape(idx);
}

byte_* tensor::get_data()
{
	_int offset = 0;
	byte_* src = nullptr;

	if (data_ptr != nullptr)
		return *data_ptr;

	for (const auto& [fst, snd] : view._offset)
		offset += fst;

	for (auto& p = parent; p != nullptr; p = p->parent)
	{
		if (src == nullptr)
		{
			for (const auto& [fst, snd] : p->view._offset)
				offset += fst;
			if (p->data_ptr != nullptr)
			{
				src = *p->data_ptr;
				break;
			}
		}
	}

	if (data_ptr == nullptr || *data_ptr == nullptr)
	{
		auto* new_data = global_store_manager.allocate_block(size_in_bytes);
		data_ptr = std::make_shared<byte_*>(new_data);
		global_store_manager.copy_block(new_data, src + offset, size_in_bytes);
	}
	/*if (mdata == nullptr)
	{
		mdata = std::make_shared<storage>(size_in_bytes);
		mdata->set_data(src, 0, offset, size_in_bytes);
	}*/

	//return mdata->get_data();

	return *data_ptr;
}


void tensor::set_data(const byte_* src, _int offset) const
{
	/*if (mdata != nullptr)
		mdata->set_data(src, 0, 0, size_in_bytes);*/

	std::shared_ptr<tensor> p = nullptr;
	for (p = parent; p != nullptr; p = p->parent);
	if (p != nullptr)
		p->set_data(src, view._offset[0].first);
}

void tensor::set_data(tensor& t) const
{
	if (data_ptr != nullptr)
	{
		/*for (const auto& i : view.offset)
			mdata->set_data(t.get_data(), i.first, 0, size_in_bytes);*/
	}
	std::shared_ptr<tensor> p = nullptr;
	for (p = parent; p != nullptr; p = p->parent);
	if (p != nullptr)
		p->set_data(t);
	throw std::runtime_error("Set data of tensor Not finished");
}

tensor tensor::reshape(std::vector<int>& new_shape)
{
	const views new_view = view.reshape(new_shape);
	return {new_view, this, dtype};
}

tensor tensor::reshape(const std::vector<int>& new_shape)
{
	const auto new_view = view.reshape(new_shape);
	return {new_view, this, dtype};
}

tensor& tensor::operator[](const _int i)
{
	children.emplace_back(view.select(0, i), this, dtype);
	return children.back();
}
