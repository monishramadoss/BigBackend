#include "storage_manager.h"


storage_manager::storage_manager() : host_memory_allocator(4096), host(get_avalible_device)
{
#ifdef VULKAN
	std::copy(vk_devices.begin(), vk_devices.end(), vkdev.begin()); // NOLINT(cppcoreguidelines-prefer-member-initializer)
	for (auto& gpu : vkdev)
		vk_memory_allocators.emplace_back(gpu.get_device(), gpu.get_mem_properties(), 16384);
#endif
}

byte_* storage_manager::allocate_block(size_t size)
{
	block* blk = host_memory_allocator.allocate(size);
	host_mapping[blk->ptr + blk->offset] = blk;
	m_used_memory += size;
	return blk->ptr + blk->offset;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void storage_manager::copy_block(byte_* dst, byte_* src, size_t size)
{
	byte_* d = nullptr;
	byte_* s = nullptr;

	block* dst_block = host_mapping[dst];
	block* src_block = host_mapping[src];


	if (src_block == nullptr || dst_block == nullptr)
		memcpy(dst, src, size);
	else
	{
		memcpy(dst_block->ptr + dst_block->offset, src_block->ptr + src_block->offset, size);
	}
}

void storage_manager::free_block(byte_* src)
{
	block* blk = host_mapping[src];
	if (blk != nullptr)
	{
		m_used_memory -= blk->size;
		host_memory_allocator.deallocate(blk);
	}
	else
	{
		host_mapping.erase(src);
		free(src);
	}
}

#ifdef VULKAN

void storage_manager::allocate_vk_block(byte_* ptr, size_t device_id)
{
	block* blk = host_mapping[ptr];
	vk_block* vkblk = vk_memory_allocators[device_id].allocate(blk->size);
	vk_memory_mapping[blk] = vkblk;
	vk_device_memory_mapping[vkblk] = device_id;
}


void storage_manager::free_vk_block(byte_* ptr)
{
	block* blk = host_mapping[ptr];
	vk_block* vkblk = vk_memory_mapping[blk];
	const size_t device_id = vk_device_memory_mapping[vkblk];
	vk_memory_mapping.erase(blk);
	vk_device_memory_mapping.erase(vkblk);
	vk_memory_allocators[device_id].deallocate(vkblk);
}


#endif
