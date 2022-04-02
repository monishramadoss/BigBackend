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
}

#ifdef VULKAN
#include "vulkan.hpp"

vk_device_storage::vk_device_storage(_int size) : byte_size(size)
{
	VkBufferCreateInfo bufferCreateInfo {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


	if(vkCreateBuffer(dev.get_device(), &bufferCreateInfo, nullptr, &device_buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("cannot create buffer correctly");
	}


	vk_data = dev.allocate_memory(&device_buffer);
}

vk_device_storage::vk_device_storage(const byte_* src, _int size) : byte_size(size)
{
	VkBufferCreateInfo bufferCreateInfo {};
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(dev.get_device(), &bufferCreateInfo, nullptr, &device_buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("cannot create buffer correctly");
	}

	
	vk_data = dev.allocate_memory(&device_buffer);
	std::memcpy(vk_data->ptr + vk_data->offset, src, byte_size);
}

vk_device_storage::~vk_device_storage()
{
	vkDestroyBuffer(dev.get_device(), device_buffer, nullptr);
	dev.free_memory(vk_data);
}

vk_device_storage::vk_device_storage(const vk_device_storage& vkbs) = default;

VkBuffer vk_device_storage::get_buffer() const { return device_buffer; }

#endif
