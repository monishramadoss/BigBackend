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

bool chunk::deallocate(block* blk)
{
	const auto shared_block = std::make_shared<block>(*blk);

	for(auto& local_blk: m_blocks)
	{
		if(*blk == *local_blk)
		{
			local_blk->free = true;
			blk = local_blk.get();
			m_dealloc_size++;
			return true;
		}
	}

	return false;
}

bool chunk::allocate(size_t size, size_t alignment, block** blk)
{
	if (size > m_size)
		return false;
	for(size_t i = 0; i < m_blocks.size(); ++i)
	{
		if(m_blocks[i]->free)
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
				if (m_blocks[i]->size == size) {
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
}

block* device_allocator::allocate(size_t size) 
{
	return this->allocate(size, 1);
}

block* device_allocator::allocate(size_t size, size_t alignment)
{
	block* blk = nullptr;
	for(const auto& chunk: m_chunks)
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
	for(const auto& chunk: m_chunks)
	{
		if (chunk->deallocate(blk))
			return;
	}
}



	//Vulkan Allocator
#ifdef VULKAN

vk_chunk::vk_chunk(const VkDevice& dev, size_t size, int memoryTypeIndex): m_device(dev), m_size(size), m_memory_type_index(memoryTypeIndex)
{
	VkMemoryAllocateInfo alloc_info {};
	alloc_info.allocationSize = size;
	alloc_info.memoryTypeIndex = memoryTypeIndex;

	if (VK_SUCCESS != vkAllocateMemory(dev, &alloc_info, nullptr, &m_memory))
		throw std::runtime_error("CANNOT ALLOCATE VULKAN OBJECT");

	vk_block blk;
	blk.free = true;
	blk.offset = 0;
	blk.size = size;
	blk.memory = m_memory;

	m_blocks.push_back(std::make_shared<vk_block>(blk));	
}


bool vk_chunk::allocate(size_t size, size_t alignment, const vk_block* blk)
{
	if (size > m_size)
		return false;
	for (const auto& m_block : m_blocks)
	{
		if (m_block->free)
		{
			size_t new_size = m_block->size;
			if (m_block->offset % alignment != 0)
				new_size -= alignment - m_block->offset % alignment;
			if (new_size >= size)
			{
				m_block->size = new_size;
				if (m_block->offset % alignment != 0)
					m_block->offset += alignment - m_block->offset % alignment;
				if (m_ptr != nullptr)
					m_block->ptr = static_cast<char*>(m_ptr) + m_block->offset;
				if (m_block->size == size) {
					m_block->free = false;
					blk = m_block.get();
					return true;
				}

				vk_block nextBlock;
				nextBlock.free = true;
				nextBlock.offset = m_block->offset + size;
				nextBlock.size = m_block->size - size;
				nextBlock.memory = m_memory;

				m_blocks.push_back(std::make_shared<vk_block>(nextBlock));

				m_block->size = size;
				m_block->free = false;

				blk = m_block.get();
				return true;
			}
		}
	}
	return false;
}

bool vk_chunk::deallocate(vk_block* blk)
{
	const auto shard_blk = std::make_shared<vk_block>(*blk);
	const auto blockIt(std::find(m_blocks.begin(), m_blocks.end(), shard_blk));
	if (blockIt == m_blocks.end())
		return false;
	blockIt->get()->free = true;
	blk->free = true;
	m_dealloc_size++;
	return true;
}


vk_device_allocator::vk_device_allocator(const VkDevice& dev, size_t size, int memoryTypeIndex) : m_size(size), m_device(dev)
{
	if (!isPowerOfTwo(size))
		throw std::runtime_error("Size must be in allocation of power 2");
	m_chunks.push_back(std::make_shared<vk_chunk>(dev, size, memoryTypeIndex));
}


vk_block* vk_device_allocator::allocate(size_t size, size_t alignment, int memoryTypeIndex)
{
	vk_block* blk = nullptr;
	for (const auto& chunk : m_chunks)
	{
		if (chunk->allocate(size, alignment, blk))
			return blk;
	}

	m_size = size > m_size ? nextPowerOfTwo(size) : m_size;
	m_chunks.push_back(std::make_shared<vk_chunk>(m_device, m_size, memoryTypeIndex));
	if (!m_chunks.back()->allocate(size, alignment, blk))
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


#endif

















struct Chunk
{
	Chunk* next;
};

class PoolAllocator
{
public:
	PoolAllocator(size_t chunksPerBlock): mChunksPerBlock(chunksPerBlock) {}
	void* allocate(size_t size)
	{
		if (ptr == nullptr)
		{
			ptr = allocateBlock(size);
		}
		Chunk* freeChunk = ptr;
		ptr = ptr->next;
		return freeChunk;
	}
	void deallocate(void* ptr, size_t)
	{
		static_cast<Chunk*>(ptr)->next = static_cast<Chunk*>(ptr);
	}
private:
	size_t mChunksPerBlock;
	Chunk* ptr = nullptr;

	[[nodiscard]] Chunk* allocateBlock(size_t chunkSize) const
	{
		const size_t blockSize = mChunksPerBlock * chunkSize;
		Chunk* blockBegin = reinterpret_cast<Chunk*>(malloc(blockSize));
		Chunk* chunk = blockBegin;
		for(size_t i = 0; i < mChunksPerBlock-1; ++i)
		{
			chunk->next = reinterpret_cast<Chunk*>(reinterpret_cast<char*>(chunk) + chunkSize);
			chunk = chunk->next;
		}
		chunk = chunk->next = nullptr;
		return blockBegin;
	}
};