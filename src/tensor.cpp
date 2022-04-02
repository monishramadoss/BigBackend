#include "tensor.h"
//#include <mpi.h>

/*
	float float_values[data_string.size()/sizeof(float)]; // 513
	memcpy(float_values, data_string.data(), data_string.size());
 */


class base_storage;

tensor::tensor() = default;

tensor::tensor(byte_* data, std::vector<_int>& shape, Format fmt) : local_tensor_id(++global_tensor_id), dtype(fmt), view(element_size(fmt), shape), size_in_bytes(view.size_in_bytes())
{
	mdata = std::make_shared<base_storage>(data, size_in_bytes);
	delete[] data;
	shard_state.clear();
}

tensor::tensor(byte_* data, const std::vector<_int>& shape, Format fmt) : local_tensor_id(++global_tensor_id), dtype(fmt), view(element_size(fmt), shape), size_in_bytes(view.size_in_bytes())
{
	mdata = std::make_shared<base_storage>(data, size_in_bytes);
	delete[] data;
	shard_state.clear();
}

tensor::tensor(views v, tensor* ptr, const Format fmt) : local_tensor_id(++global_tensor_id), dtype(fmt), view(std::move(v)), size_in_bytes(view.size_in_bytes())
{
	if (this != ptr && parent == nullptr)
	{
		parent.reset(ptr);
		ptr->children.push_back(*this);
	}
}

tensor::tensor(tensor&& t) = default;

tensor::tensor(const tensor& t) : local_tensor_id(t.local_tensor_id), dtype(t.dtype), children(t.children), mdata(t.mdata),
                                 src_kernel(t.src_kernel), dst_kernel(t.dst_kernel), view(t.view), size_in_bytes(t.size_in_bytes)
{
	std::cout << "Copy Operator" << std::endl;
	if(parent == nullptr)
		parent = t.parent;
}

tensor& tensor::operator=(const tensor& t) 
{
	std::cout << "Assign Operator" << std::endl;
	if (this == &t && local_tensor_id == t.local_tensor_id)
		return *this;

	parent = std::make_shared<tensor>(tensor(*t.parent));

	local_tensor_id = t.local_tensor_id;
	children = t.children;
	view = t.view;
	dtype = t.dtype;
	size_in_bytes = t.size_in_bytes;
	mdata = t.mdata;
	src_kernel = t.src_kernel;
	dst_kernel = t.dst_kernel;
	
	return *this;

}

tensor& tensor::operator=(tensor&& t)
{
	std::cout << "Assigned Move Operator" << std::endl;
	if (this == &t)
		return *this;

	parent = std::make_shared<tensor>(tensor(*t.parent));

	local_tensor_id = t.local_tensor_id;
	children = std::move(t.children);
	view = t.view;
	dtype = t.dtype;
	size_in_bytes = t.size_in_bytes;
	mdata = std::move(t.mdata);	
	src_kernel = t.src_kernel;
	dst_kernel = t.dst_kernel;
	return *this;
}


std::pair<_int, _int> tensor::get_offset() const
{
	std::pair<_int, _int> o;

	for (const auto& off : view.offset)
	{
		o.first += off.first;
		o.second = off.second;
	}

	for (std::shared_ptr<tensor> p = parent; p != nullptr; p = p->parent)
	{
		for (const auto& [fst, snd] : p->view.offset)
			o.first += fst;
	}
	return o;
}

void tensor::clear_storage()
{
	mdata.reset();
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

	if (mdata != nullptr)
		return mdata->get_data();

	for(const auto& [fst, snd]: view.offset)
		offset += fst;

	for(auto& p = parent; p != nullptr; p = p->parent)
	{
		if(src == nullptr)
		{
			for (const auto& [fst, snd] : p->view.offset)
				offset += fst;
			if(p->mdata != nullptr)
			{
				src = p->mdata->get_data();
				break;
			}
		}
	}

	if (mdata == nullptr)
	{
		mdata = std::make_shared<base_storage>(size_in_bytes);
		mdata->set_data(src, 0, offset, size_in_bytes);
	}

	return mdata->get_data();
}


void tensor::set_data(const byte_* src, _int offset) const
{
	if (mdata != nullptr)
		mdata->set_data(src, 0, 0, size_in_bytes);

	std::shared_ptr<tensor> p = nullptr;
	for (p = parent; p != nullptr; p = p->parent);
	if (p != nullptr)
		p->set_data(src, view.offset[0].first);
}

void tensor::set_data(tensor& t) const
{
	if (mdata != nullptr)
	{
		for (const auto& i : view.offset)
			mdata->set_data(t.get_data(), i.first, 0, size_in_bytes);
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
	return { new_view, this, dtype};
}

tensor tensor::reshape(const std::vector<int>& new_shape)
{
	const auto new_view = view.reshape(new_shape);
	return { new_view, this, dtype };
}

tensor& tensor::operator[](const _int i)
{
	children.emplace_back(view.select(0, i), this, dtype);
	return children.back();
}

byte_* tensor::get_storage_data() const
{
	if(mdata==nullptr)
	{
		return parent->get_storage_data(); // src;
	}
	else
		return mdata->get_data();
}
