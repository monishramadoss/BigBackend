#pragma once
#include <cinttypes>
#include <cmath>
#include <vector>
#include <memory>
#include <algorithm>
#include "types.h"


inline size_t nextPowerOfTwo(size_t size)
{
	const unsigned power = 1 + std::llroundl(std::log2l(static_cast<long double>(size)));
	return static_cast<size_t>(1) << power;
}


inline bool isPowerOfTwo(size_t size)
{
	size_t mask = 0;
	const size_t power = std::llroundl(std::log2l(static_cast<long double>(size)));

	for (size_t i = 0; i < power; ++i)
		mask += static_cast<size_t>(1) << i;
	return !(size & mask);
}


struct block
{
	size_t size{};
	size_t offset{};
	bool free{};
	int device_id{};
	byte_* ptr = nullptr;
	bool operator == (block const& block) const
	{
		return offset == block.offset && size == block.size &&
			free == block.free && ptr == block.ptr && device_id == block.device_id;
	}
};

class chunk {
public:
	chunk(size_t size);
	bool allocate(size_t, size_t, block**);
	bool deallocate(block* blk);
	~chunk();

protected:
	uint16_t m_dealloc_size{};
	size_t m_size;
	std::vector<std::shared_ptr<block>> m_blocks;
	byte_* m_ptr = nullptr;
};
//
//inline bool chunk::isIn(block* blk) const
//{
//	const auto shared_ptr_block = std::shared_ptr<block>(blk);
//	return std::find(m_blocks.begin(), m_blocks.end(), shared_ptr_block) != m_blocks.end();
//}


class device_allocator 
{
public:
	device_allocator(size_t size);	
	block* allocate(size_t size);
	void deallocate(block*) const;
private:
	size_t m_size;
	block* allocate(size_t size, size_t alignment);
	std::vector<std::shared_ptr<chunk>> m_chunks;
};






#define VULKAN
#ifdef VULKAN

#include <vulkan/vulkan.h>

struct vk_block {
	VkDeviceMemory memory{};
	size_t size{};
	size_t offset{};
	bool free{};
	int device_id{};
	byte_* ptr = nullptr;

	bool operator == ( vk_block const& blk) const
	{
		return offset == blk.offset && size == blk.size &&
			free == blk.free && ptr == blk.ptr && device_id == blk.device_id && 
			memory == blk.memory;
	}
};

class vk_chunk {
public:
	vk_chunk(const VkDevice& dev, size_t size, int memoryTypeIndex);
	[[nodiscard]] int memoryTypeIndex() const { return m_memory_type_index; }

	bool allocate(size_t, size_t, const vk_block*);
	bool deallocate(vk_block* blk);
private:
	VkDevice m_device;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;
	uint16_t m_dealloc_size{};
	size_t m_size;
	std::vector<std::shared_ptr<vk_block>> m_blocks;
	void* m_ptr = nullptr;
	int m_memory_type_index;
};
//
//inline bool vk_chunk::isIn(vk_block* blk) const
//{
//	const auto shared_ptr_block = std::shared_ptr<vk_block>(blk);
//	return std::find(m_blocks.begin(), m_blocks.end(), shared_ptr_block) != m_blocks.end();
//}


class vk_device_allocator
{
public:
	vk_device_allocator(const VkDevice& dev, size_t size, int memoryTypeIndex);
	vk_block* allocate(size_t size, size_t alignment, int memoryTypeIndex);
	void deallocate(vk_block* blk) const;
private:
	size_t m_size;
	size_t m_alignment{};
	VkDevice m_device;
	std::vector<std::shared_ptr<vk_chunk>> m_chunks;
	
};


#endif