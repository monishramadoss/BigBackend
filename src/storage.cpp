#include "storage.h"


base_storage::base_storage(_int size) : byte_size(size)
{
	io_ += 1;
	file_name = "data_" + std::to_string(io_) + ".dat";

	afile.open(file_name, std::ios::out | std::ios::in | std::ios::binary);
	data = dev.allocate_memory(size);
}

base_storage::base_storage(const byte_* src, _int size) : byte_size(size)
{
	io_++;
	file_name = "data_" + std::to_string(io_) + ".dat";

	afile.open(file_name, std::ios::out | std::ios::binary);
	data = dev.allocate_memory(byte_size);
	dev.copy_memory(data, 0, src, byte_size);
}

base_storage::base_storage(const base_storage& bs) : data(bs.data), byte_size(bs.byte_size), file_name(bs.file_name)
{
	afile.open(file_name, std::ios::out | std::ios::binary);
}

base_storage::base_storage(_int size, device& dev) : byte_size(size), dev(dev)
{
	io_++;
	file_name = "data_" + std::to_string(io_) + ".dat";

	afile.open(file_name, std::ios::out | std::ios::binary);
	data = dev.allocate_memory(byte_size);
}

void base_storage::set_data(const byte_* src, _int dst_offset, _int src_offset, _int size)
{
	dev.copy_memory(data, dst_offset, src + src_offset, size);
}

void base_storage::offload()
{
	afile.write(data->ptr + data->offset, byte_size);
}

void base_storage::onload()
{
	afile.read(data->ptr + data->offset, byte_size);
}

base_storage::~base_storage()
{
	dev.free_memory(data);
	data = nullptr;
}


#ifdef VULKAN
#include "vulkan.hpp"

vk_device_storage::vk_device_storage(_int size) : base_storage(size, get_vk_device()), dev(get_vk_device())
{
	auto* vk_data = dev.get_device_memory(data);
	dev.set_device_memory(vk_data, data);
}

vk_device_storage::vk_device_storage(const byte_* src, _int size) : base_storage(size, get_vk_device()),
                                                                    dev(get_vk_device())
{
	auto* vk_data = dev.get_device_memory(data);
	dev.set_device_memory(vk_data, data);
}


vk_device_storage::vk_device_storage(const vk_device_storage& vkbs) = default;

#endif
