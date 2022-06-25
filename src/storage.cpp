//#include "storage.h"
//
//
//storage::storage(_int size)
//{
//	
//}
//
//storage::storage(const storage& bs) : data(bs.data)
//{
//	
//}
//
//void storage::set_data(const byte_* src, _int dst_offset, _int src_offset, _int size)
//{
//	
//}
//
//storage::~storage()
//{
//	data = nullptr;
//}
//
//
//#ifdef VULKAN
//#include "vulkan.hpp"
//
//vk_storage::vk_storage(_int size): byte_size(size), device(get_vk_device()), dev_data(nullptr)
//{
//	VkBufferCreateInfo bufferCreateInfo{};
//	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//	bufferCreateInfo.size = byte_size;
//	bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
//	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//
//	if (vkCreateBuffer(device.get_device(), &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
//		throw std::runtime_error("failed to create buffer");
//}
//
//
//vk_storage::vk_storage(const vk_storage& vkbs) = default;
//
//
//
//
//#endif
