#pragma once
#include <fstream>
#include <string>

#include "device.h"
#include "device_memory.h"
#include "types.h"


static _int io_{0};

class base_storage
{
public:
	base_storage(_int size);
	base_storage(const byte_* src, _int size);
	base_storage(const base_storage& bs);
	base_storage(_int size, device& dev);
	[[nodiscard]] byte_* get_data() const { return data->ptr + data->offset; }
	[[nodiscard]] const device& get_device() const { return dev; }
	void set_data(const byte_* src, _int dst_offset, _int src_offset, _int size);
	void offload();
	void onload();
	~base_storage();
protected:
	block* data;
	_int byte_size;

	device dev = get_avalible_device();
	std::string file_name;
	std::fstream afile;
};

#ifdef VULKAN
#include <vulkan/vulkan.h>

class vk_device_storage : base_storage
{
public:
	vk_device_storage(_int size);
	vk_device_storage(const byte_* src, _int size);
	vk_device_storage(const vk_device_storage& vkbs);

	VkBuffer buffer{};
	vk_device device;
	vk_block* dev_data;
};
#endif
