#include "device.h"


#ifdef __unix__
#include <unist.h>
_int getTotalSystemMemory()
{
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	return pages * page_size;
}

_int getTotalDiskSpace()
{
	return getTotalSystemMemory();
}

#elif defined(_WIN32) || defined(WIN32)
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

#endif


_int used_memory{0};

device::device() : m_num_threads(std::thread::hardware_concurrency()), device_id(devices.size()), global_rank(0), local_rank(0), m_max_memory_size(getTotalSystemMemory()),
                   m_used_memory(0)
{
	m_pool.setThreadCount(m_num_threads);
}

device::device(device& d) : m_num_threads(std::thread::hardware_concurrency()), device_id(d.device_id), global_rank(d.global_rank), local_rank(d.local_rank),
                            m_max_memory_size(d.m_max_memory_size), m_used_memory(d.m_used_memory)
{
	m_pool.setThreadCount(m_num_threads);
}

device::device(const device& d) : m_num_threads(std::thread::hardware_concurrency()), device_id(d.device_id), global_rank(d.global_rank), local_rank(d.local_rank),
                                  m_max_memory_size(d.m_max_memory_size), m_used_memory(d.m_used_memory)
{
	m_pool.setThreadCount(m_num_threads);
}

device::device(device&& d) noexcept : m_num_threads(std::thread::hardware_concurrency()), device_id(d.device_id), global_rank(d.global_rank), local_rank(d.local_rank),
                                      m_max_memory_size(d.m_max_memory_size), m_used_memory(d.m_used_memory)
{
	m_pool.setThreadCount(m_num_threads);
}

device& device::operator=(const device&) = default;

device::~device() = default;


void device::free_memory(block* data)
{
	device::m_allocator.deallocate(data);
}

block* device::allocate_memory(_int size)
{
	return device::m_allocator.allocate(size);
}

void device::copy_memory(block* blk, size_t dst_offset, const byte_* src, size_t n)
{
	memcpy(blk->ptr + blk->offset + dst_offset, src, n);
}

void device::copy_memory(block* dst, block* src)
{
	memcpy(dst->ptr + dst->offset, src->ptr + src->offset, dst->size);
}

bool device::is_avalible(int d_type) const
{
	if (m_device_type != d_type)
		return false;
	if (m_used_memory >= m_max_memory_size)
		return false;
	return true;
}



#ifdef VULKAN

constexpr float queuePriority = 1.0f;
#include <iostream>
#include "vulkan.hpp"

vk_device::vk_device(const VkInstance& instance, const VkPhysicalDevice& pDevice) :
	device(), m_instance(instance), m_physical_device(pDevice), m_device_allocator(nullptr)
{
	setupDebugMessenger(m_instance, m_debug_messenger);

	m_device_type = 2;
	vkGetPhysicalDeviceProperties(pDevice, &m_device_properties);
	vkGetPhysicalDeviceMemoryProperties(pDevice, &m_memory_properties);

	auto buffer_copy_offset = m_device_properties.limits.optimalBufferCopyOffsetAlignment;
	std::cout << "Using device: " << m_device_properties.deviceName << '\n';

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, queueFamilies.data());
	
	uint32_t i = 0;
	for(auto& queueFamily: queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
			m_compute_queue_index.push_back(i);
		if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			m_transfer_queue_index.push_back(i);
		i++;
	}

	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.queueFamilyIndex = m_compute_queue_index.back();	
	queueCreateInfo.pQueuePriorities = &queuePriority;
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

	if (enableValidationLayers) {
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else 
		deviceCreateInfo.enabledLayerCount = 0;
	
	if (vkCreateDevice(pDevice, &deviceCreateInfo, nullptr, &m_device))
		throw std::runtime_error("failed to create logical device!");
	
	vkGetDeviceQueue(m_device, m_compute_queue_index.back(), 0, &m_cmd_queue);

	m_device_allocator = std::make_shared<m_device_allocator>(m_device, 4096, transfer_buffer);

}

vk_device::~vk_device() = default;

vk_block* vk_device::allocate_memory(const VkBuffer* buffer) const
{
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(get_device(), *buffer, &memoryRequirements);

	const auto size = memoryRequirements.size;
	const auto alignment = memoryRequirements.alignment;
	const auto memoryTypeIndex = findMemoryTypeIndex(memoryRequirements.memoryTypeBits, m_memory_properties, true);

	return m_device_allocator->allocate(size, alignment, memoryTypeIndex);
}

void vk_device::free_memory(vk_block* blk) const
{
	m_device_allocator->deallocate(blk);
}

void emplace_vulkan_devices(std::vector<vk_device>& devices)
{
	VkInstance vk_instance;
	createInstance(vk_instance);

	uint32_t deviceCount = 0;
	if (vkEnumeratePhysicalDevices(vk_instance, &deviceCount, nullptr))
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	if (deviceCount == 0)
		throw std::runtime_error("failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> vkdevices(deviceCount);
	vkEnumeratePhysicalDevices(vk_instance, &deviceCount, vkdevices.data());
	for (VkPhysicalDevice& pDevice : vkdevices)
		devices.emplace_back(vk_instance, pDevice);

}

vk_device& get_vk_device()
{
	if(vk_devices.empty())
		emplace_vulkan_devices(vk_devices);
	return vk_devices[0];
}

#else
void emplace_vulkan_devices(std::vector<vk_device>& devices)
{
	
}

#endif

ThreadPool device::m_pool = ThreadPool();
device_allocator device::m_allocator = device_allocator(4096);
std::vector<node> device::m_object_store = {};
std::vector<std::vector<float>> device::m_transfer_latency = {};
std::vector<std::vector<float>> device::m_compute_latency = {};



device& get_avalible_device(int device_type)
{
	if (devices.empty())
		devices.emplace_back();

	if (vk_devices.empty())
		emplace_vulkan_devices(vk_devices);
	for (auto& dev : vk_devices)
		devices.push_back(dev);

	for (auto& dev : devices)
	{
		if (dev.is_avalible(device_type))
			return dev;
	}

	device::m_transfer_latency = std::vector<std::vector<float>>(devices.size(), std::vector<float>(devices.size(), 0));
	device::m_compute_latency = std::vector<std::vector<float>>(devices.size(), std::vector<float>(devices.size(), 0));

	return devices[0];
}


