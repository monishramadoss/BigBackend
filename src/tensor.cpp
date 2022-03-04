#include "tensor.h"
//#include <mpi.h>

/*
	float float_values[data_string.size()/sizeof(float)]; // 513
	memcpy(float_values, data_string.data(), data_string.size());
 */


class base_storage;

tensor::tensor(byte_* data, std::vector<_int>& shape, Format fmt) : dtype(fmt), size_in_bytes(element_size(fmt))
{
	view = views(element_size(fmt), shape);
	size_in_bytes *= view.size(0);
	data_ = std::make_shared<base_storage>(data, size_in_bytes);
	delete[] data;
	shard_state.clear();
}

tensor::tensor(byte_* data, const std::vector<_int>& shape, Format fmt) : dtype(fmt), size_in_bytes(element_size(fmt))
{
	view = views(element_size(fmt), shape);
	size_in_bytes *= view.size(0);
	data_ = std::make_shared<base_storage>(data, size_in_bytes);
	delete[] data;
	shard_state.clear();
}

tensor::tensor(views v, tensor& ptr, const Format fmt) : dtype(fmt), view(std::move(v)), size_in_bytes(element_size(fmt))
{
	parent = std::make_shared<tensor>(ptr);
	size_in_bytes *= view.size(0);
}

tensor::tensor(tensor&& t) noexcept : dtype(t.dtype), parent(std::move(t.parent)),
                                      data_(std::move(t.data_)), view(std::move(t.view)), size_in_bytes(t.size_in_bytes) {}

tensor::tensor(const tensor& t) : dtype(t.dtype), parent(t.parent),
                                  data_(t.data_), view(t.view), size_in_bytes(t.size_in_bytes) {}


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

byte_* tensor::get_data()
{
	_int offset = 0;
	byte_* src = nullptr;

	if (data_ != nullptr)
		return data_->get_data();

	for(const auto& [fst, snd]: view.offset)
		offset += fst;

	for(std::shared_ptr<tensor> p = parent; p != nullptr; p = p->parent)
	{
		if(src == nullptr)
		{
			for (const auto& off : p->view.offset)
				offset += off.first;
			if(p->data_ != nullptr)
			{
				src = p->data_->get_data();
				break;
			}
		}
	}

	if (data_ == nullptr)
	{
		data_ = std::make_shared<base_storage>(size_in_bytes);
		data_->set_data(src, 0, offset, size_in_bytes);
	}

	return data_->get_data();
}

byte_* tensor::get_storage_data() const
{
	if(data_==nullptr)
	{
		return parent->get_storage_data(); // src;
	}
	else
		return data_->get_data();
}



void tensor::set_data(const byte_* src, _int offset) const
{
	if(data_ != nullptr)
		data_->set_data(src, 0, 0, size_in_bytes);
	
	std::shared_ptr<tensor> p = nullptr;
	for (p = parent; p != nullptr; p = p->parent);
	if(p != nullptr)
		p->set_data(src, view.offset[0].first);
}
