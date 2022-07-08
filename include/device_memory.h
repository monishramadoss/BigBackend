#pragma once

#include "types.h"


#include <map>
#include <string>
#include <fstream>
#include <cinttypes>
#include <vector>
#include <memory>


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
	byte_* ptr = nullptr;
	//void* ptr;
	char block_type{};

	bool operator ==(const block& block) const
	{
		return offset == block.offset && size == block.size &&
			free == block.free && ptr == block.ptr && block_type == block.block_type;
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

class host_allocator
{
public:
	host_allocator(size_t size);
	block* allocate(size_t size);
	void deallocate(block*) const;
private:
	size_t m_size;
	block* allocate(size_t size, size_t alignment);
	std::vector<std::shared_ptr<chunk>> m_chunks;
};

struct offline_chunk
{
	size_t offset;
	size_t size;
	std::string file_name;
	bool free;
};

static _int io_{0};

class off_device_cache
{
	//TODO make like device_allocator which chunks to be written places
public:
	off_device_cache() : local_rank(io_)
	{
		io_++;
		const auto file_name = "_" + std::to_string(local_rank) + ".dat";
		file_store[file_name].open(file_name, std::ios::out | std::ios::binary);
	}

	void to(byte_* data, size_t size)
	{
		//TODO include with thread pool.
	}

	void from(offline_chunk* chunk, byte_* data, size_t size)
	{
		//TODO figure out how to manage storage elements
		chunk->free = true;
	}

private:
	std::map<std::string, std::fstream> file_store;
	std::vector<offline_chunk> mChunks;
	size_t local_rank;
	size_t total_size{0};
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
	VkBuffer buf = VK_NULL_HANDLE;
	VkImage img = VK_NULL_HANDLE;

	bool operator==(const vk_block& blk) const
	{
		return offset == blk.offset && size == blk.size &&
			free == blk.free && ptr == blk.ptr && device_id == blk.device_id &&
			memory == blk.memory && buf == blk.buf && img == blk.img;
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


class vk_allocator
{
public:
	vk_allocator();
	vk_allocator(const VkDevice& dev, const VkPhysicalDeviceMemoryProperties& properties, size_t size = 4096);
	vk_block* allocate(size_t size, bool make_buffer = true);
	void deallocate(vk_block* blk);

	static VkBuffer& get_buffer(vk_block* blk) { return blk->buf; }
	static VkImage& get_image(vk_block* blk) { return blk->img; }

private:
	size_t m_size;
	size_t m_alignment{};
	VkDevice m_device;
	VkPhysicalDeviceMemoryProperties properties;

	std::vector<std::shared_ptr<vk_chunk>> m_chunks;

	/* TODO conversion of buffer to image and vice versa...
	*	copy_block 2 buffer
	*	copy_block 2 Image
	*	Image 2 buffer
	*	Buffer 2 Image
	*/

};

#endif

#ifdef CUDA

struct cu_block
{
	size_t offset;
	size_t size;
	bool free;
	int device_id;
	void* ptr;

	bool operator==(const cu_block& blk)
	{
		return offset == blk.offest && size == blk.size && free == blk.free && device_id == blk.device_id && ptr == blk.ptr;
	}

};

#endif
