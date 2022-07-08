#include "storage_manager.h"


storage_manager::storage_manager() { }

void* storage_manager::allocate_block(size_t size) 
{
	void* ptr = device::device_lst[current_device]->malloc(size);
	device_map[ptr] = current_device;	
	return ptr;
}


// ReSharper disable once CppMemberFunctionMayBeConst
void storage_manager::copy_block(void* dst, void* src, size_t size)
{
	//auto dst_dev = device_map.find(dst);
	//auto src_dev = device_map.find(src);
	//if (dst_dev == device_map.end())
	//	dst = allocate_block(size);
	//if (src_dev == device_map.end())
	//{
	//	void* ptr_src = allocate_block(size);
	//	std::memcpy(ptr_src, src, size);
	//	src = ptr_src;
	//	src_dev = device_map.find(src);
	//}
	//if (dst_dev->second != src_dev->second)
	//	return;

	//const size_t dev_id = device_map[dst];
	//device::device_lst[dev_id]->memcpy(src, dst, size);
}


void storage_manager::free_block(void* ptr)
{
	auto dev_id = device_map.find(ptr);
	if(dev_id == device_map.end() || dev_id->second == -1)
		return;
	device::device_lst[dev_id->second]->free(ptr);
	//device_map.erase(ptr);
}

#ifdef VULKAN
#include <iostream>
VkBuffer& storage_manager::get_vk_buffer(void* ptr)
{
	auto dev_id = device_map.find(ptr);
	if (dev_id == device_map.end())
		std::cout << "HELP " << std::endl;
	// trigger transfer to vulkan;

	bool dev_type = device::device_lst[dev_id->second]->type() == 1;

		

	auto vkBuf = static_cast<VkBuffer>(device::device_lst[dev_id->second]->get_buffer(ptr));
	return vkBuf;
}


#endif
