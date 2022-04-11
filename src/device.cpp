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

device::device() : m_num_threads(std::thread::hardware_concurrency()), device_id(devices.size()), global_rank(0),
                   local_rank(0), m_max_memory_size(getTotalSystemMemory()),
                   m_used_memory(0)
{
	m_pool.setThreadCount(m_num_threads);
}

device::device(device& d) : m_num_threads(std::thread::hardware_concurrency()), device_id(d.device_id),
                            global_rank(d.global_rank), local_rank(d.local_rank),
                            m_max_memory_size(d.m_max_memory_size), m_used_memory(d.m_used_memory)
{
	m_pool.setThreadCount(m_num_threads);
}

device::device(const device& d) : m_num_threads(std::thread::hardware_concurrency()), device_id(d.device_id),
                                  global_rank(d.global_rank), local_rank(d.local_rank),
                                  m_max_memory_size(d.m_max_memory_size), m_used_memory(d.m_used_memory)
{
	m_pool.setThreadCount(m_num_threads);
}

device::device(device&& d) noexcept : m_num_threads(std::thread::hardware_concurrency()), device_id(d.device_id),
                                      global_rank(d.global_rank), local_rank(d.local_rank),
                                      m_max_memory_size(d.m_max_memory_size), m_used_memory(d.m_used_memory)
{
	device::m_pool.setThreadCount(m_num_threads);
}

device& device::operator=(const device&) = default;


void device::set_input(const std::shared_ptr<tensor>& input, const std::shared_ptr<compute_job>& cmp)
{
	src_map[cmp].push_back(input);
}

void device::set_output(const std::shared_ptr<tensor>& output, const std::shared_ptr<compute_job>& cmp)
{
	dst_map[output] = cmp;
}

std::shared_ptr<tensor> device::get_input(size_t i, const std::shared_ptr<compute_job>& cmp) const
{
	return src_map.at(cmp)[i];
}

std::shared_ptr<tensor> device::get_output(size_t i, std::shared_ptr<compute_job>& cmp)
{
	const auto result = std::find_if(dst_map.begin(), dst_map.end(),
	                                 [&](const auto& mo) {return mo.second == cmp; });
	if (result != dst_map.end())
		return result->first;
	else
		throw std::runtime_error("cannot find tensor in object queue");
}

device::~device() = default;

block* device::allocate_memory(_int size)
{
	return m_allocator.allocate(size);
}

void device::free_memory(block* data)
{
	m_allocator.deallocate(data);
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

inline int get_heap_index(const VkMemoryPropertyFlags& flags, const VkPhysicalDeviceMemoryProperties& properties)
{
	for (auto i = 0; i < properties.memoryTypeCount; ++i)
	{
		if ((flags & properties.memoryTypes[i].propertyFlags) == flags)
			return properties.memoryTypes[i].heapIndex;
	}
	return -1;
}


vk_device::vk_device(const VkInstance& instance, const VkPhysicalDevice& pDevice) :
	device(), m_instance(instance), m_physical_device(pDevice), m_device_allocator(nullptr)
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
	m_device_allocator = std::make_shared<vk_device_allocator>(m_device, 4096);

	//create_staging_buffer();
}

vk_device::vk_device(const vk_device&) = default;

vk_device::vk_device(vk_device&&) noexcept = default;

vk_device::~vk_device() = default;

block* vk_device::allocate_memory(_int size)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;

	if (vkCreateBuffer(m_device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("cannot create buffer correctly");
	}


	block* blk = m_allocator.allocate(size);
	m_device_map[blk] = allocate_device_memory(&buffer);
	return blk;
}


vk_block* vk_device::allocate_device_memory(const VkBuffer* buffer) const
{
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(get_device(), *buffer, &memoryRequirements);

	const auto size = memoryRequirements.size;
	const auto alignment = memoryRequirements.alignment;
	const auto memoryTypeIndex = findMemoryTypeIndex(memoryRequirements.memoryTypeBits, m_memory_properties, true);

	vk_block* vkblk = m_device_allocator->allocate(size, alignment, memoryTypeIndex);
	return vkblk;
}


void vk_device::free_memory(block* blk)
{
	auto* vkblk = m_device_map[blk];
	if (vkblk != nullptr)
	{
		m_device_allocator->deallocate(vkblk);
		m_device_map[blk] = nullptr;
	}
	m_allocator.deallocate(blk);
}

void vk_device::free_device_memory(vk_block* vkblk) const
{
	m_device_allocator->deallocate(vkblk);
}

void vk_device::copy_memory(block* blk, size_t dst_offset, const byte_* src, size_t n)
{
	device::copy_memory(blk, dst_offset, src, n);
}

void vk_device::copy_memory(block* dst, block* src)
{
	device::copy_memory(dst, src);
}

void vk_device::copy_memory(vk_block* dst, block* src)
{
	if (m_device_map[src] == nullptr)
		m_device_map[src] = dst;
	else
		throw std::runtime_error("Copy to device not implemented");
}

void vk_device::copy_memory(block* dst, vk_block* src)
{
	if (m_device_map[dst] == nullptr)
		m_device_map[dst] = src;
	else
		throw std::runtime_error("Copy to host not implemented");
}

vk_block* vk_device::get_device_memory(block* blk) const
{ return m_device_map.at(blk); }

void vk_device::set_device_memory(vk_block* dblk, block* hblk)
{ m_device_map[hblk] = dblk; }

VkDevice vk_device::get_device() const
{ return m_device; }

VkPhysicalDeviceMemoryProperties vk_device::get_mem_properties() const
{ return m_memory_properties; }

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
	if (vk_devices.empty())
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
