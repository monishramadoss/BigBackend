#pragma once
#include <map>
#include <vector>
#include "device.h"
#include "device_memory.h"


//TODO TRIGGER when to fire off to device data

class compute_manager;

class storage_manager
{
public:
	storage_manager();

	byte_* allocate_block(size_t size);
	void free_block(byte_* src);

	void copy_block(byte_* dst, byte_* src, size_t size);

private:
	std::map<byte_*, block*> host_mapping;
	host_allocator host_memory_allocator;
	std::vector<off_device_cache> sideline_data;
	device& (*host)(int device_type);
	size_t m_used_memory{0};

#ifdef VULKAN
	std::map<block*, vk_block*> vk_memory_mapping;
	std::map<vk_block*, size_t> vk_device_memory_mapping;
	std::vector<vk_device_allocator> vk_memory_allocators;
	std::vector<vk_device> vkdev;
	std::vector<size_t> m_vk_used_memory;

	void allocate_vk_block(byte_* ptr, size_t device_id = 0);
	void free_vk_block(byte_* ptr);
#endif

	friend compute_manager;
};

static storage_manager global_store_manager = storage_manager();


//
//void vk_device::copy_memory(vk_block* dst, vk_block* src) const
//{
//	if (dst == nullptr || src == nullptr)
//		return;
//
//	VkCommandBufferBeginInfo command_buffer_begin_info;
//	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//	command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
//
//	vkBeginCommandBuffer(m_transfer_cmd_buffer, &command_buffer_begin_info);
//	VkBufferCopy  buffer_copy_info;
//	buffer_copy_info.dstOffset = dst->offset;
//	buffer_copy_info.srcOffset = src->offset;
//	buffer_copy_info.size = src->size;
//
////	vkCmdCopyBuffer(m_cmd_buffer, NULL, NULL, 1, &buffer_copy_info);
//	VkBufferMemoryBarrier buffer_memory_barrier;
//	buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
//	buffer_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
//	buffer_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
//	buffer_memory_barrier.offset = src->offset;
//	buffer_memory_barrier.size = src->size;
//
//	vkCmdPipelineBarrier(m_transfer_cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
//		0, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
//
//	vkEndCommandBuffer(m_transfer_cmd_buffer);
//
//
//	VkSubmitInfo submit_info;
//	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//	submit_info.waitSemaphoreCount = 0;
//	submit_info.pCommandBuffers = &m_transfer_cmd_buffer;
//	submit_info.signalSemaphoreCount = 0;
//	submit_info.pSignalSemaphores = nullptr;
//
//	if (vkQueueSubmit(m_cmd_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
//		throw std::runtime_error("QUEUE SUBMISSION FAILED");
//	vkDeviceWaitIdle(m_device);
//}
