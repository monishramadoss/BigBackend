#include "storage.h"


base_storage::base_storage(_int d_size) : byte_size(d_size), device_offset(0)
{
	io_ += 1;
	file_name = "data_" + std::to_string(io_) + ".dat";
	afile.open(file_name, std::ios::out | std::ios::in | std::ios::binary);
	data.reset(dev.allocate_memory(d_size, device_offset));
}

base_storage::base_storage(const byte_* src, _int size) : byte_size(size), device_offset(0)
{
	io_++;
	file_name = "data_" + std::to_string(io_) + ".dat";
	afile.open(file_name, std::ios::out | std::ios::binary);
	data.reset(dev.allocate_memory(size, device_offset));
	std::memcpy(data.get(), src, byte_size);
}


base_storage::base_storage(const base_storage& bs) : byte_size(bs.byte_size), device_offset(bs.device_offset), file_name(bs.file_name)
{

	afile.open(file_name, std::ios::out | std::ios::binary);
	data.reset(dev.allocate_memory(bs.byte_size, device_offset));
	std::memcpy(data.get(), bs.data.get(), byte_size);
}

void base_storage::set_data(const byte_* src, const _int dst_offset, const _int src_offset, const _int size) const
{

	memcpy(data.get() + dst_offset, src + src_offset, size);
}

void base_storage::offload()
{
	afile.write(data.get(), byte_size);
}

void base_storage::onload()
{
	afile.read(data.get(), byte_size);
}

base_storage::~base_storage()
{
	data.reset();
	dev.free_memory(data.get());
}

batch_storage::batch_storage(byte_* src, _int d_size, _int batch_size) : base_storage(0), batch_size(batch_size)
{
	d_size /= batch_size;
	for(_int i = 0; i < batch_size; ++i)
	{
		b_data.emplace_back(src + i * d_size, d_size);
	}
}

batch_storage::batch_storage(_int d_size, _int batch_size) : base_storage(0), batch_size(batch_size)
{
	d_size /= batch_size;
	for(_int i = 0; i < batch_size; ++i)
	{
		b_data.emplace_back(d_size);
	}
}

void batch_storage::set_data(const byte_* src, _int idx, _int dst_offset, _int src_offset, _int size) const
{
	b_data[idx].set_data(src, dst_offset, src_offset, size);
}

void batch_storage::set_data(base_storage& src, _int idx)
{
	//b_data[idx] = src;
	//b_data[idx];
}
