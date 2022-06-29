#include <stdexcept>
#include "device_memory.h"

#include "vulkan.hpp"


//Chunk 

chunk::chunk(size_t size) : m_size(size)
{
	block blk;
	blk.free = true;
	blk.offset = 0;
	blk.size = size;
	m_ptr = blk.ptr = static_cast<byte_*>(malloc(size));
	m_blocks.push_back(std::make_shared<block>(blk));
}

bool chunk::deallocate(const block* blk) const
{
	std::shared_ptr<block> src_blk = nullptr;

	for (auto& _blk : m_blocks)
	{
		if (*_blk == *blk)
		{
			_blk->free = true;
			return true;
		}
		if (_blk->free)
		{
			src_blk = _blk;
		}
	}
	return false;
}

bool chunk::allocate(size_t size, size_t alignment, block** blk)
{
	if (size > m_size)
		return false;
	for (size_t i = 0; i < m_blocks.size(); ++i)
	{
		if (m_blocks[i]->free)
		{
			size_t new_size = m_blocks[i]->size;
			if (m_blocks[i]->offset % alignment != 0)
				new_size -= alignment - m_blocks[i]->offset % alignment;
			if (new_size >= size)
			{
				m_blocks[i]->size = new_size;
				if (m_blocks[i]->offset % alignment != 0)
					m_blocks[i]->offset += alignment - m_blocks[i]->offset % alignment;
				if (m_ptr != nullptr)
					m_blocks[i]->ptr = m_ptr + m_blocks[i]->offset;
				if (m_blocks[i]->size == size)
				{
					m_blocks[i]->free = false;
					*blk = m_blocks[i].get();
					return true;
				}

				block nextBlock;
				nextBlock.free = true;
				nextBlock.offset = m_blocks[i]->offset + size;
				nextBlock.size = m_blocks[i]->size - size;

				m_blocks.push_back(std::make_shared<block>(nextBlock));
				m_blocks[i]->size = size;
				m_blocks[i]->free = false;

				*blk = m_blocks[i].get();
				return true;
			}
		}
	}
	return false;
}

chunk::~chunk()
{
	for (const auto& blk : m_blocks)
		blk->free = true;

	free(m_ptr);
}


//Device Allocator
host_allocator::host_allocator(size_t size) : m_size(size)
{
	if (!isPowerOfTwo(size))
		throw std::runtime_error("Size must be in allocation of power 2");
	m_chunks.push_back(std::make_shared<chunk>(m_size));
}

block* host_allocator::allocate(size_t size)
{
	return this->allocate(size, 1);
}

block* host_allocator::allocate(size_t size, size_t alignment)
{
	block* blk = nullptr;
	for (const auto& chunk : m_chunks)
	{
		if (chunk->allocate(size, alignment, &blk))
			return blk;
	}

	m_size = size > m_size ? nextPowerOfTwo(size) : m_size;

	m_chunks.push_back(std::make_shared<chunk>(m_size));
	if (!m_chunks.back()->allocate(size, alignment, &blk))
		throw std::bad_alloc();
	return blk;
}

void host_allocator::deallocate(block* blk) const
{
	for (const auto& chunk : m_chunks)
	{
		if (chunk->deallocate(blk))
			return;
	}
}


//Vulkan Allocator
#ifdef VULKAN


vk_chunk::vk_chunk(const VkDevice& dev, size_t size, int memoryTypeIndex): m_device(dev), m_size(size),
                                                                           m_memory_type_index(memoryTypeIndex)
{
	VkMemoryAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = size;
	alloc_info.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(dev, &alloc_info, nullptr, &m_memory) != VK_SUCCESS)
		throw std::runtime_error("CANNOT ALLOCATE VULKAN OBJECT");

	vk_block blk;
	blk.free = true;
	blk.offset = 0;
	blk.size = size;
	blk.memory = m_memory;

	m_blocks.push_back(std::make_shared<vk_block>(blk));
}


bool vk_chunk::allocate(size_t size, size_t alignment, vk_block** blk)
{
	if (size > m_size)
		return false;
	for (size_t i = 0; i < m_blocks.size(); ++i)
	{
		if (m_blocks[i]->free)
		{
			size_t new_size = m_blocks[i]->size;
			if (m_blocks[i]->offset % alignment != 0)
				new_size -= alignment - m_blocks[i]->offset % alignment;
			if (new_size >= size)
			{
				m_blocks[i]->size = new_size;
				if (m_blocks[i]->offset % alignment != 0)
					m_blocks[i]->offset += alignment - m_blocks[i]->offset % alignment;
				if (m_blocks[i]->ptr != nullptr)
					m_blocks[i]->ptr = m_blocks[i]->ptr + m_blocks[i]->offset;
				if (m_blocks[i]->size == size)
				{
					m_blocks[i]->free = false;
					*blk = m_blocks[i].get();
					return true;
				}

				vk_block nextBlock;
				nextBlock.free = true;
				nextBlock.offset = m_blocks[i]->offset + size;
				nextBlock.size = m_blocks[i]->size - size;
				nextBlock.memory = m_memory;

				m_blocks.push_back(std::make_shared<vk_block>(nextBlock));
				m_blocks[i]->size = size;
				m_blocks[i]->free = false;

				*blk = m_blocks[i].get();
				return true;
			}
		}
	}
	return false;
}

bool vk_chunk::deallocate(const vk_block* blk) const
{
	for (const auto& _blk : m_blocks)
	{
		if (*_blk == *blk)
		{
			_blk->free = true;
			return true;
		}
	}

	return false;
}


vk_chunk::~vk_chunk()
{
	bool mem_flag = false;
	for (const auto& blk : m_blocks)
	{
		blk->free = true;
		if (blk->ptr != nullptr)
			mem_flag = true;
	}
	if (mem_flag)
		vkUnmapMemory(m_device, m_memory);
	vkFreeMemory(m_device, m_memory, nullptr);
}


vk_device_allocator::vk_device_allocator(const VkDevice& dev, const VkPhysicalDeviceMemoryProperties& properties,
                                         size_t size): m_size(size), m_device(dev), properties(properties)
{
	if (!isPowerOfTwo(size))
		throw std::runtime_error("Size must be in allocation of power 2");
	//m_chunks.push_back(std::make_shared<vk_chunk>(dev, m_size, 2));
}


vk_block* vk_device_allocator::allocate(size_t size, bool make_buffer)
{
	uint32_t memoryTypeIndex = 0;
	size_t alignment = 0;
	VkBuffer buffer{};
	//	VkImage image;
	if (make_buffer)
	{
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		/*
		bufferCreateInfo.queueFamilyIndexCount = 1;
		bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;
		*/
		/*requiredMemorySize += bufferMemoryRequirements.size;

		if (bufferMemoryRequirements.size % bufferMemoryRequirements.alignment != 0) {
			requiredMemorySize += bufferMemoryRequirements.alignment - bufferMemoryRequirements.size % bufferMemoryRequirements.alignment;
		}*/


		if (vkCreateBuffer(m_device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create buffer");
		}
		VkMemoryRequirements bufferMemoryRequirements;
		vkGetBufferMemoryRequirements(m_device, buffer, &bufferMemoryRequirements);
		alignment = bufferMemoryRequirements.alignment;
		memoryTypeIndex = findMemoryTypeIndex(bufferMemoryRequirements.memoryTypeBits, properties, true);
	}


	vk_block* blk = nullptr;
	for (const auto& chunk : m_chunks)
	{
		if (chunk->allocate(size, alignment, &blk))
			return blk;
	}

	m_size = size > m_size ? nextPowerOfTwo(size) : m_size;
	m_chunks.push_back(std::make_shared<vk_chunk>(m_device, m_size, memoryTypeIndex));
	if (!m_chunks.back()->allocate(size, alignment, &blk))
		throw std::bad_alloc();

	if (make_buffer)
	{
		buffer_map[blk] = buffer;
	}

	return blk;
}

void vk_device_allocator::deallocate(vk_block* blk)
{
	for (const auto& chunk : m_chunks)
	{
		if (chunk->deallocate(blk))
		{
			vkDestroyBuffer(m_device, buffer_map[blk], nullptr);
			return;
		}
	}
}


#endif


//struct Chunk
//{
//	Chunk* next;
//};
//
//class PoolAllocator
//{
//public:
//	PoolAllocator(size_t chunksPerBlock): mChunksPerBlock(chunksPerBlock) {}
//	void* allocate(size_t size)
//	{
//		if (ptr == nullptr)
//		{
//			ptr = allocateBlock(size);
//		}
//		Chunk* freeChunk = ptr;
//		ptr = ptr->next;
//		return freeChunk;
//	}
//	void deallocate(void* ptr, size_t)
//	{
//		static_cast<Chunk*>(ptr)->next = static_cast<Chunk*>(ptr);
//	}
//private:
//	size_t mChunksPerBlock;
//	Chunk* ptr = nullptr;
//
//	[[nodiscard]] Chunk* allocateBlock(size_t chunkSize) const
//	{
//		const size_t blockSize = mChunksPerBlock * chunkSize;
//		Chunk* blockBegin = reinterpret_cast<Chunk*>(malloc(blockSize));
//		Chunk* chunk = blockBegin;
//		for(size_t i = 0; i < mChunksPerBlock-1; ++i)
//		{
//			chunk->next = reinterpret_cast<Chunk*>(reinterpret_cast<char*>(chunk) + chunkSize);
//			chunk = chunk->next;
//		}
//		chunk = chunk->next = nullptr;
//		return blockBegin;
//	}
//};
