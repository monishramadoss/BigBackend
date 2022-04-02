#pragma once
#include <fstream>
#include <string>

#include "device.h"
#include "device_memory.h"
#include "types.h"


static _int io_{0};

class base_storage final
{
public:
	base_storage(_int size);
	base_storage(const byte_* src, _int size);
	base_storage(const base_storage& bs);

	[[nodiscard]] byte_* get_data() const
	{
		return data->ptr + data->offset;
	}
	void set_data(const byte_* src, _int dst_offset, _int src_offset, _int size);
	void offload();
	void onload();
	~base_storage();
protected:
	block* data;
	_int byte_size;

	device dev = get_avalible_device();
	std::string file_name;
	std::fstream  afile;
};

#ifdef VULKAN
#include <vulkan/vulkan.h>
class vk_device_storage final
{
public:
	vk_device_storage(_int size);
	vk_device_storage(const byte_* src, _int size);
	vk_device_storage(const vk_device_storage& vkbs);
	[[nodiscard]] VkBuffer get_buffer() const;
	~vk_device_storage();
protected:
	vk_block* vk_data;
	_int byte_size;

	vk_device dev = std::move(get_vk_device());
	VkBuffer device_buffer{};
	VkBuffer host_buffer{};
};
#endif