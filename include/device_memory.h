#pragma once
#include <cinttypes>
#include <cmath>
#include <vector>
#include <memory>
#include <algorithm>
#include "types.h"


inline bool isPowerOfTwo(size_t size)
{
	const bool ret = (size & (size - 1)) == 0;
	return ret;
}


inline size_t power_floor(size_t x)
{
	size_t power = 1;
	while (x >>= 1) power <<= 1;
	return power;
}

inline size_t power_ceil(size_t x)
{
	if (x <= 1) return 1;
	size_t power = 2;
	x--;
	while (x >>= 1) power <<= 1;
	return power;
}

inline size_t nextPowerOfTwo(size_t size)
{
	const size_t _size = power_ceil(size);
	return _size << 1;
}

struct block
{
	size_t size{};
	size_t offset{};
	bool free{};
	int device_id{};
	byte_* ptr = nullptr;

	bool operator ==(const block& block) const
	{
		return offset == block.offset && size == block.size &&
			free == block.free && ptr == block.ptr && device_id == block.device_id;
	}
};

class chunk
{
public:
	chunk(size_t size);
	bool allocate(size_t, size_t, block**);
	bool deallocate(const block* blk) const;
	~chunk();

protected:
	uint16_t m_dealloc_size{};
	size_t m_size;
	std::vector<std::shared_ptr<block>> m_blocks;
	byte_* m_ptr = nullptr;
};

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

struct vk_block
{
	VkDeviceMemory memory{};
	size_t size{};
	size_t offset{};
	bool free{};
	int device_id{};
	byte_* ptr = nullptr;

	bool operator ==(const vk_block& blk) const
	{
		return offset == blk.offset && size == blk.size &&
			free == blk.free && ptr == blk.ptr && device_id == blk.device_id &&
			memory == blk.memory;
	}
};

class vk_chunk
{
public:
	vk_chunk(const VkDevice& dev, size_t size, int memoryTypeIndex);
	[[nodiscard]] int memoryTypeIndex() const { return m_memory_type_index; }

	bool allocate(size_t, size_t, vk_block**);
	bool deallocate(const vk_block* blk) const;

	[[nodiscard]] VkDeviceMemory get_memory() const { return m_memory; }
	~vk_chunk();
private:
	VkDevice m_device;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;
	size_t m_size;
	std::vector<std::shared_ptr<vk_block>> m_blocks;
	int m_memory_type_index;
};


class vk_device_allocator
{
public:
	vk_device_allocator(const VkDevice& dev, size_t size = 4096);
	vk_block* allocate(size_t size, size_t alignment, uint32_t memoryTypeIndex);
	void deallocate(vk_block* blk) const;

	vk_block* transfer_on_buffer(size_t size, size_t alignment, uint32_t memoryTypeBits);
	vk_block* transfer_off_buffer(size_t size, size_t alignment, uint32_t memoryTypeBits);

	void free_transfer_on_buffer(const vk_block*) const;
	void free_transfer_off_buffer(const vk_block*) const;
private:
	size_t m_size;
	size_t m_alignment{};
	VkDevice m_device;
	std::vector<std::shared_ptr<vk_chunk>> m_chunks;

	std::unique_ptr<vk_chunk> m_on_transfer_chunk;
	std::unique_ptr<vk_chunk> m_off_transfer_chunk;
	VkBuffer m_transfer_on_buffer;
	VkBuffer m_transfer_off_buffer;
};


#endif
