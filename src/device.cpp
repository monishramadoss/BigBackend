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
	m_pool.setThreadCount(m_num_threads);
}

device::device(device& d) : m_num_threads(std::thread::hardware_concurrency()), device_id(d.device_id),
                            global_rank(d.global_rank), local_rank(d.local_rank),
                            m_max_memory_size(d.m_max_memory_size)
{
	m_pool.setThreadCount(m_num_threads);
}

device::device(const device& d) : m_num_threads(std::thread::hardware_concurrency()), device_id(d.device_id),
                                  global_rank(d.global_rank), local_rank(d.local_rank),
                                  m_max_memory_size(d.m_max_memory_size)
{
	m_pool.setThreadCount(m_num_threads);
}

device::device(device&& d) noexcept : m_num_threads(std::thread::hardware_concurrency()), device_id(d.device_id),
                                      global_rank(d.global_rank), local_rank(d.local_rank),
                                      m_max_memory_size(d.m_max_memory_size)
{
	m_pool.setThreadCount(m_num_threads);
}

device& device::operator=(const device&) = default;


device::~device() = default;


bool device::is_avalible(int d_type) const
{
	if (m_device_type != d_type)
		return false;
	return true;
}


#ifdef VULKAN

constexpr float queuePriority = 1.0f;
#include <iostream>
#include "vulkan.hpp"

inline uint32_t get_heap_index(const VkMemoryPropertyFlags& flags, const VkPhysicalDeviceMemoryProperties& properties)
{
	for (auto i = 0; i < properties.memoryTypeCount; ++i)
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
	device(), m_instance(instance), m_physical_device(pDevice)
{
	setupDebugMessenger(m_instance, m_debug_messenger);

	m_device_type = 2;
	vkGetPhysicalDeviceProperties(pDevice, &m_device_properties);
	vkGetPhysicalDeviceMemoryProperties(pDevice, &m_memory_properties);

	const int heap_idx = get_heap_index(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_memory_properties);

	if (heap_idx == -1)
		throw std::runtime_error("Cannot find device heap");

	m_max_device_memory_size = m_memory_properties.memoryHeaps[heap_idx].size;

	// auto buffer_copy_offset = m_device_properties.limits.optimalBufferCopyOffsetAlignment;
	m_max_work_group_size[0] = m_device_properties.limits.maxComputeWorkGroupCount[0];
	m_max_work_group_size[1] = m_device_properties.limits.maxComputeWorkGroupCount[1];
	m_max_work_group_size[2] = m_device_properties.limits.maxComputeWorkGroupCount[2];
	m_max_work_groups[0] = m_device_properties.limits.maxComputeWorkGroupSize[0];
	m_max_work_groups[1] = m_device_properties.limits.maxComputeWorkGroupSize[1];
	m_max_work_groups[2] = m_device_properties.limits.maxComputeWorkGroupSize[2];

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, queueFamilies.data());

	uint32_t i = 0;
	for (auto& queueFamily : queueFamilies)
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

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

	if (enableValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
		deviceCreateInfo.enabledLayerCount = 0;

	if (vkCreateDevice(pDevice, &deviceCreateInfo, nullptr, &m_device))
		throw std::runtime_error("failed to create logical device!");

	std::cout << "Using device: " << m_device_properties.deviceName << " Maximum Memory: " << m_max_device_memory_size
		<< '\n';

	vkGetDeviceQueue(m_device, m_compute_queue_index.back(), 0, &m_cmd_queue);

	/*VkCommandBufferAllocateInfo buffer_allocate_info;
	buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	buffer_allocate_info.commandBufferCount = 1;
	buffer_allocate_info.commandPool = m_cmd_pool;
	buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	vkAllocateCommandBuffers(m_device, &buffer_allocate_info, &m_transfer_cmd_buffer);*/
}

vk_device::vk_device(const vk_device&) = default;

vk_device::vk_device(vk_device&&) noexcept = default;

vk_device::~vk_device() = default;

vk_device& vk_device::operator=(vk_device&) = default;


VkDevice vk_device::get_device() const
{
	return m_device;
}

VkPhysicalDeviceMemoryProperties vk_device::get_mem_properties() const
{
	return m_memory_properties;
}

void emplace_vulkan_devices(std::vector<vk_device>& devices)
{
	VkInstance vk_instance = VK_NULL_HANDLE;
	createInstance(vk_instance);
	if(vk_instance == nullptr)
		return;

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
	if (vk_devices.empty())
		emplace_vulkan_devices(vk_devices);
	
	if (vk_devices.empty())
		return;

	return vk_devices[0];
}

#else
void emplace_vulkan_devices(std::vector<vk_device>& devices)
{
	
}

#endif


ThreadPool device::m_pool = ThreadPool();
std::vector<std::vector<float>> device::m_transfer_latency = {};
std::vector<std::vector<float>> device::m_compute_latency = {};


device& get_avalible_device(int device_type)
{
	if (devices.empty())
		devices.emplace_back();

#ifdef VULKAN
	if (vk_devices.empty())
	{
		emplace_vulkan_devices(vk_devices);
		for (auto& dev : vk_devices)
			devices.push_back(dev);
	}
#endif

	for (auto& dev : devices)
	{
		if (dev.is_avalible(device_type))
			return dev;
	}

	device::m_transfer_latency = std::vector<std::vector<float>>(devices.size(), std::vector<float>(devices.size(), 0));
	device::m_compute_latency = std::vector<std::vector<float>>(devices.size(), std::vector<float>(devices.size(), 0));

	return devices[0];
}
