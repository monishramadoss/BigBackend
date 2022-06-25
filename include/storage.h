//#pragma once
//#include <fstream>
//#include <string>
//
//#include "device.h"
//#include "device_memory.h"
//#include "types.h"
//
//
//
//class storage final
//{
//public:
//	explicit storage(_int size);
//	storage(const storage& bs);
//	[[nodiscard]] byte_* get_data() const { return data->ptr + data->offset; }
//	void set_data(const byte_* src, _int dst_offset, _int src_offset, _int size);
//
//	~storage();
//protected:
//	block* data;
//};
//
//
//
//#ifdef VULKAN
//#include <vulkan/vulkan.h>
//
//class vk_storage final
//{
//public:
//	vk_storage(_int size);
//	vk_storage(const vk_storage& vkbs);
//
//	[[nodiscard]] VkBuffer get_buffer() const { return buffer; }
//	[[nodiscard]] size_t get_size() const { return dev_data->size; }
//	[[nodiscard]] size_t get_offset() const { return dev_data->offset; }
//	
//
//private:
//	VkBuffer buffer{};
//	vk_block* dev_data;
//};
//#endif
//
