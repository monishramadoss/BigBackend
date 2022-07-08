#include "device.h"

#if defined(_WIN32) || defined(WIN32)
#include <Windows.h>

_int getTotalSystemMemory()
{
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	return status.ullTotalPhys;
}

_int getTotalDiskSpace()
{
	return getTotalSystemMemory();
}

#else
#include <unistd.h>
_int getTotalSystemMemory(){
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	return pages * page_size;
}

_int getTotalDiskSpace()
{
	return getTotalSystemMemory();
}

#endif


_int used_memory{0};

device::device() : m_num_threads(std::thread::hardware_concurrency()), device_id(devices.size()), global_rank(0),
                   local_rank(0), m_max_memory_size(getTotalSystemMemory())
{
	//device::device_lst.push_back(this);
	//m_pool.setThreadCount(m_num_threads);
}

device::device(device& d) : m_num_threads(std::thread::hardware_concurrency()), device_id(d.device_id),
                            global_rank(d.global_rank), local_rank(d.local_rank),
                            m_max_memory_size(d.m_max_memory_size)
{
	//m_pool.setThreadCount(m_num_threads);
}

device::device(const device& d) : m_num_threads(std::thread::hardware_concurrency()), device_id(d.device_id),
                                  global_rank(d.global_rank), local_rank(d.local_rank),
                                  m_max_memory_size(d.m_max_memory_size)
{
	//m_pool.setThreadCount(m_num_threads);
}

device::device(device&& d) noexcept : m_num_threads(std::thread::hardware_concurrency()), device_id(d.device_id),
                                      global_rank(d.global_rank), local_rank(d.local_rank),
                                      m_max_memory_size(d.m_max_memory_size)
{
	//m_pool.setThreadCount(m_num_threads);
}

device& device::operator=(const device&) = default;

device::~device() = default;

void* device::malloc(size_t size)
{
	block* blk = m_allocator.allocate(size);
	void* ptr = blk->ptr + blk->offset;
	memory_map[ptr] = blk;
	return ptr;
}

void device::free(void* ptr)
{
	block* blk = memory_map[ptr];
	m_allocator.deallocate(blk);
}

void device::memcpy(void* dst, void* src, size_t size)
{
	const auto* src_blk = memory_map[src];
	const auto* dst_blk = memory_map[dst];
	if(size == 0)
		size = min(src_blk->size, dst_blk->size);

	std::memcpy(dst, src, size);
}


bool device::is_avalible(int d_type) const
{
	if (m_device_type != d_type)
		return false;
	return true;
}


block* device::get_block(void* ptr)
{
	const auto blk = memory_map.find(ptr);
	if (blk == memory_map.end())
		return nullptr;
	else
		return blk->second;
}

void device::transfer(device& dev)
{

}


#ifdef VULKAN

constexpr float queuePriority = 1.0f;
#include <iostream>
#include <vulkan/vulkan.h>
#include "vulkan.hpp"

inline uint32_t get_heap_index(const VkMemoryPropertyFlags& flags, const VkPhysicalDeviceMemoryProperties& properties)
{
	for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
	{
		if ((flags & properties.memoryTypes[i].propertyFlags) == flags)
			return properties.memoryTypes[i].heapIndex;
	}
	return -1;
}


/**
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 0x00000001,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 0x00000002,
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = 0x00000004,
    VK_MEMORY_PROPERTY_HOST_CACHED_BIT = 0x00000008,
 */

vk_device::vk_device(const VkInstance& instance, const VkPhysicalDevice& pDevice) :
	device(), m_instance(instance), m_physical_device(pDevice), m_staging_cmd_buffer(nullptr)
{
	m_device_type = 1;
	setupDebugMessenger(m_instance, m_debug_messenger);

	vkGetPhysicalDeviceProperties(pDevice, &m_device_properties);
	vkGetPhysicalDeviceMemoryProperties(pDevice, &m_memory_properties);

	const int heap_idx = get_heap_index(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_memory_properties);

	if (heap_idx == -1)
		throw std::runtime_error("Cannot find device heap");

	m_max_device_memory_size = m_memory_properties.memoryHeaps[heap_idx].size;
	m_max_memory_size = m_max_device_memory_size;
	// auto buffer_copy_offset = m_device_properties.limits.optimalBufferCopyOffsetAlignment;
	m_max_work_group_count[0] = m_device_properties.limits.maxComputeWorkGroupCount[0];
	m_max_work_group_count[1] = m_device_properties.limits.maxComputeWorkGroupCount[1];
	m_max_work_group_count[2] = m_device_properties.limits.maxComputeWorkGroupCount[2];
	m_max_work_group_size[0] = m_device_properties.limits.maxComputeWorkGroupSize[0];
	m_max_work_group_size[1] = m_device_properties.limits.maxComputeWorkGroupSize[1];
	m_max_work_group_size[2] = m_device_properties.limits.maxComputeWorkGroupSize[2];

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, queueFamilies.data());

	uint32_t i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
			m_compute_queue_index.push_back(i);
		if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			m_staging_queue_index.push_back(i);
		i++;
	}

	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.queueFamilyIndex = m_compute_queue_index.back();
	queueCreateInfo.pQueuePriorities = &queuePriority;


	// any specific device features needed
	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	if (vkCreateDevice(pDevice, &deviceCreateInfo, nullptr, &m_device) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device");

	std::cout << "Using device: " << m_device_properties.deviceName << " Maximum Memory: " << m_max_device_memory_size
		<< '\n';

	vkGetDeviceQueue(m_device, m_compute_queue_index.back(), 0, &m_cmd_queue);

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = m_compute_queue_index.back();

	if (vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_cmd_pool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool");

	VkCommandPoolCreateInfo commandPoolCreateInfo_2 = {};
	commandPoolCreateInfo_2.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo_2.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo_2.queueFamilyIndex = m_staging_queue_index.back();

	if (vkCreateCommandPool(m_device, &commandPoolCreateInfo_2, nullptr, &m_transfer_pool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool");

	VkCommandBufferAllocateInfo buffer_allocate_info = {};
	buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	buffer_allocate_info.commandPool = m_transfer_pool;
	buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	buffer_allocate_info.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(m_device, &buffer_allocate_info, &m_staging_cmd_buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create transfer buffer");

	m_allocator = vk_allocator(m_device, m_memory_properties, 16);

}


vk_device::vk_device(const vk_device& vkd) : m_allocator(vkd.m_allocator)
{
	for (char i = 0; i < 3; ++i)
	{
		m_max_work_group_size[i] = vkd.m_max_work_group_size[i];
		m_max_work_group_count[i] = vkd.m_max_work_group_count[i];
	}

	m_instance = vkd.m_instance;
	m_physical_device = vkd.m_physical_device;
	m_device_properties = vkd.m_device_properties;
	m_memory_properties = vkd.m_memory_properties;
	m_debug_messenger = vkd.m_debug_messenger;

	m_compute_queue_index = vkd.m_compute_queue_index;
	m_staging_queue_index = vkd.m_staging_queue_index;

	m_device = vkd.m_device;
	m_cmd_queue = vkd.m_cmd_queue;
	m_cmd_pool = vkd.m_cmd_pool;
	m_transfer_pool = vkd.m_transfer_pool;

	m_max_device_memory_size = vkd.m_max_device_memory_size;

	m_staging_queue = vkd.m_staging_queue;
	m_staging_cmd_buffer = vkd.m_staging_cmd_buffer;

	memory_map = vkd.memory_map;
	device_memory_map = vkd.device_memory_map;
}


vk_device::vk_device(vk_device&& vkd) noexcept : m_allocator(std::move(vkd.m_allocator))
{
	for (char i = 0; i < 3; ++i)
	{
		m_max_work_group_size[i] = vkd.m_max_work_group_size[i];
		m_max_work_group_count[i] = vkd.m_max_work_group_count[i];
	}

	m_instance = vkd.m_instance;
	m_physical_device = vkd.m_physical_device;
	m_device_properties = vkd.m_device_properties;
	m_memory_properties = vkd.m_memory_properties;
	m_debug_messenger = vkd.m_debug_messenger;

	m_compute_queue_index = vkd.m_compute_queue_index;
	m_staging_queue_index = vkd.m_staging_queue_index;

	m_device = vkd.m_device;
	m_cmd_queue = vkd.m_cmd_queue;
	m_cmd_pool = vkd.m_cmd_pool;
	m_transfer_pool = vkd.m_transfer_pool;

	m_max_device_memory_size = vkd.m_max_device_memory_size;

	m_staging_queue = vkd.m_staging_queue;
	m_staging_cmd_buffer = vkd.m_staging_cmd_buffer;

	memory_map = std::move(vkd.memory_map);
	device_memory_map = std::move(vkd.device_memory_map);
};

vk_device::~vk_device() = default;

void* vk_device::malloc(size_t size)
{
	void* ptr = device::malloc(size);
	auto* blk = memory_map[ptr];
	auto* vk_blk = m_allocator.allocate(size, true);
	device_memory_map[blk] = vk_blk;
	return ptr;
}

void vk_device::free(void* ptr)
{
	offload(ptr);
	device::free(ptr);

}

void vk_device::memcpy(void* src, void* dst, size_t size)
{
	auto* src_blk = get_block(src);
	auto* dst_blk = get_block(dst);


	auto* src_vk_blk = device_memory_map[src_blk];
	auto* dst_vk_blk = device_memory_map[dst_blk];

	/*
	 *VkBuffer src_buf = m_allocator.get_buffer(src_vk_blk);
	VkBuffer dst_buf = m_allocator.get_buffer(dst_vk_blk);
	*/

	device::memcpy(src, dst, size);
}

bool vk_device::upload(void* ptr)
{
	auto* blk = get_block(ptr);
	const auto vk_blk = device_memory_map.find(blk);
	if (vk_blk == device_memory_map.end() || vk_blk->second == nullptr)
	{
		ptr = this->malloc(blk->size);
		return false;
	}

	//copy block on to device
	
	return true;

}

bool vk_device::offload(void* ptr)
{
	// copy block off to data

	auto* blk = get_block(ptr);
	const auto h_d_blks = device_memory_map.find(blk);
	if (h_d_blks == device_memory_map.end())
	{
		auto* vk_blk = h_d_blks->second;
		m_allocator.deallocate(vk_blk);
	}
	device_memory_map[blk] = nullptr;
	return device::offload(ptr);
}

void* vk_device::get_buffer(void* ptr)
{
	auto* blk = get_block(ptr);
	return (void*)m_allocator.get_buffer(device_memory_map[blk]);
}


vk_device& vk_device::operator=(const vk_device& vkd)
{
	for (char i = 0; i < 3; ++i)
	{
		m_max_work_group_size[i] = vkd.m_max_work_group_size[i];
		m_max_work_group_count[i] = vkd.m_max_work_group_count[i];
	}

	m_instance = vkd.m_instance;
	m_physical_device = vkd.m_physical_device;
	m_device_properties = vkd.m_device_properties;
	m_memory_properties = vkd.m_memory_properties;
	m_debug_messenger = vkd.m_debug_messenger;

	m_compute_queue_index = vkd.m_compute_queue_index;
	m_staging_queue_index = vkd.m_staging_queue_index;

	m_device = vkd.m_device;
	m_cmd_queue = vkd.m_cmd_queue;
	m_cmd_pool = vkd.m_cmd_pool;
	m_transfer_pool = vkd.m_transfer_pool;

	m_max_device_memory_size = vkd.m_max_device_memory_size;

	m_staging_queue = vkd.m_staging_queue;
	m_staging_cmd_buffer = vkd.m_staging_cmd_buffer;

	m_allocator = vkd.m_allocator;

	return *this;
}


void emplace_vulkan_devices(std::vector<vk_device*>& devices)
{
	VkInstance vk_instance = VK_NULL_HANDLE;
	createInstance(vk_instance);
	if (vk_instance == nullptr)
		return;

	uint32_t deviceCount = 0;
	if (vkEnumeratePhysicalDevices(vk_instance, &deviceCount, nullptr))
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	if (deviceCount == 0)
		throw std::runtime_error("failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> vkdevices(deviceCount);
	vkEnumeratePhysicalDevices(vk_instance, &deviceCount, vkdevices.data());
	for (VkPhysicalDevice& pDevice : vkdevices)
		devices.push_back(new vk_device(vk_instance, pDevice));
}



bool vk_device::is_on_device(block* blk)
{
	const auto vk_blk = device_memory_map.find(blk);
	bool ret = vk_blk == device_memory_map.end() || vk_blk->second == nullptr;
	return !ret;
}


#else
void emplace_vulkan_devices(std::vector<vk_device>& devices)
{
	
}

#endif

host_allocator device::m_allocator = host_allocator(1048576); // 4096);
ThreadPool device::m_pool = ThreadPool();
std::vector<std::vector<float>> device::m_transfer_latency = {};
std::vector<std::vector<float>> device::m_compute_latency = {};
std::vector<device*> device::device_lst = init_devices();

std::vector<device*>& init_devices()
{
	//std::vector<device*> devices;
	if (devices.empty())
	{
		devices.push_back(new device());

#ifdef VULKAN
		std::vector<vk_device*> vk_devices;
		emplace_vulkan_devices(vk_devices);
	
		for (auto i = 0; i < vk_devices.size(); ++i)
		{
			devices.push_back((device*)vk_devices[i]);
		}
#endif
		device::m_transfer_latency = std::vector<std::vector<float>>(devices.size(), std::vector<float>(devices.size(), 0.0));
		device::m_compute_latency = std::vector<std::vector<float>>(devices.size(), std::vector<float>(devices.size(), 0.0));
	}
	return devices;
}


device& get_avalible_device(int device_type)
{

	for (auto& dev : devices)
	{
		if (dev->is_avalible(device_type))
			return *dev;
	}

	return *devices[0];
}
