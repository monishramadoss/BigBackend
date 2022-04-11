#include <stdexcept>
#include "device_memory.h"


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
	for (auto& _blk : m_blocks)
	{
		if (*_blk == *blk)
		{
			_blk->free = true;
			return true;
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
device_allocator::device_allocator(size_t size) : m_size(size)
{
	if (!isPowerOfTwo(size))
		throw std::runtime_error("Size must be in allocation o fpower 2");
	m_chunks.push_back(std::make_shared<chunk>(m_size));
}

block* device_allocator::allocate(size_t size)
{
	return this->allocate(size, 1);
}

block* device_allocator::allocate(size_t size, size_t alignment)
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

void device_allocator::deallocate(block* blk) const
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


vk_device_allocator::vk_device_allocator(const VkDevice& dev, size_t size) : m_size(size), m_device(dev)
{
	if (!isPowerOfTwo(size))
		throw std::runtime_error("Size must be in allocation of power 2");
	//m_chunks.push_back(std::make_shared<vk_chunk>(dev, m_size, 2));
}


vk_block* vk_device_allocator::allocate(size_t size, size_t alignment, uint32_t memoryTypeIndex)
{
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
	return blk;
}

void vk_device_allocator::deallocate(vk_block* blk) const
{
	for (const auto& chunk : m_chunks)
	{
		if (chunk->deallocate(blk))
			return;
	}
}





void allocate_unique_chunk(const VkDevice& dev, std::unique_ptr<vk_chunk>& chunk, size_t size, size_t alignment, uint32_t memoryTypeBits, vk_block* blk, void* data)
{
	auto property_flag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
//
//VkBufferCreateInfo bufferInfo = {};
//bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//bufferInfo.size = m_staging_size;
//bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
//bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//bufferInfo.queueFamilyIndexCount = 1;
//bufferInfo.pQueueFamilyIndices = &m_staging_queue_index.back();
//
//if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_staging_buffer) != VK_SUCCESS)
//throw std::runtime_error("failed to create buffer!");
//
//VkMemoryRequirements memoryRequirements;
//vkGetBufferMemoryRequirements(m_device, m_staging_buffer, &memoryRequirements);
//
//VkMemoryAllocateInfo alloc_info{};
//alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//alloc_info.allocationSize = m_staging_size;
//alloc_info.memoryTypeIndex = findMemoryTypeIndex(memoryRequirements.memoryTypeBits, m_memory_properties, false);
//
//if (vkAllocateMemory(m_device, &alloc_info, nullptr, &m_staging_memory) != VK_SUCCESS)
//throw std::runtime_error("CANNOT ALLOCATE VULKAN OBJECT");
//
//void* data = nullptr;
//vkBindBufferMemory(m_device, m_staging_buffer, m_staging_memory, 0);
//vkMapMemory(m_device, m_staging_memory, 0, m_staging_size, 0, &data);


	if (chunk == nullptr)
		chunk = std::make_unique<vk_chunk>(dev, size, memoryTypeBits);
	if (!chunk->allocate(size, alignment, &blk))
		throw std::runtime_error("CANNOT ALLOCATE TRANSFER BUFFER");
	vkMapMemory(dev, chunk->get_memory(), blk->offset, blk->size, 0, &data);
	blk->ptr = static_cast<byte_*>(data);
}

vk_block* vk_device_allocator::transfer_on_buffer(size_t size, size_t alignment, uint32_t memoryTypeBits)
{
	vk_block* blk = nullptr;
	void* data = nullptr;
	allocate_unique_chunk(m_device, m_on_transfer_chunk, size, alignment, memoryTypeBits, blk, data);
	return blk;
}

vk_block* vk_device_allocator::transfer_off_buffer(size_t size, size_t alignment, uint32_t memoryTypeBits)
{
	vk_block* blk = nullptr;
	void* data = nullptr;
	allocate_unique_chunk(m_device, m_off_transfer_chunk, size, alignment, memoryTypeBits, blk, data);
	return blk;
}



void free_unique_chunk(const std::unique_ptr<vk_chunk>& buff, const vk_block* blk)
{
	if (!buff->deallocate(blk))
		throw std::runtime_error("ISSUE CANNOT DEALLOCATE");
}


void vk_device_allocator::free_transfer_on_buffer(const vk_block* blk) const
{
	free_unique_chunk(m_off_transfer_chunk, blk);
}

void vk_device_allocator::free_transfer_off_buffer(const vk_block* blk) const
{
	free_unique_chunk(m_off_transfer_chunk, blk);
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
