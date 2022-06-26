#pragma once

#define VULKAN


#include "types.h"
#include "thread_pool.h"
#include "device_memory.h"


#include <stdexcept>
#include <vector>
#include <memory>
#include <functional>


class tensor;
class compute_job;


//std::vector<std::vector<size_t>> compute_flow = {};
//std::vector<std::vector<size_t>> data_flow = {};

static size_t global_tensor_id{0};
static size_t global_task_id{0};

class device
{
public:
	device();
	device(device&);
	device(const device&);
	device(device&&) noexcept;
	device& operator=(const device&);

	[[nodiscard]] bool is_avalible(int d_type = -1) const;

	void add_job(std::function<void()> job) = delete;

	static std::vector<std::vector<float>> m_transfer_latency;
	static std::vector<std::vector<float>> m_compute_latency;

	virtual ~device();
	int m_device_type = -1;
	size_t m_num_threads;

protected:
	_int device_id;
	_int global_rank;
	_int local_rank;
	_int m_max_memory_size;
	static host_allocator m_allocator;
	static ThreadPool m_pool;
};

static std::vector<device> devices{};

#ifdef VULKAN
#include <vulkan/vulkan.h>

class vk_device : public device
{
public:
	vk_device(const VkInstance& instance, const VkPhysicalDevice& pDevice);
	vk_device(const vk_device&);
	vk_device(vk_device&&) noexcept;
	vk_device& operator=(vk_device&);
	~vk_device() override;

	[[nodiscard]] VkDevice get_device() const;
	[[nodiscard]] VkPhysicalDeviceMemoryProperties get_mem_properties() const;
	int m_max_work_group_size[3];
	int m_max_work_groups[3];


private:
	VkInstance m_instance {};
	VkPhysicalDevice m_physical_device {};
	VkPhysicalDeviceProperties m_device_properties {};
	VkPhysicalDeviceMemoryProperties m_memory_properties {};
	VkDebugUtilsMessengerEXT m_debug_messenger {};

	std::vector<uint32_t> m_compute_queue_index;
	std::vector<uint32_t> m_staging_queue_index;

	VkDevice m_device{};

	VkQueue m_cmd_queue{};
	VkCommandPool m_cmd_pool{};

	_int m_max_device_memory_size{0};
	VkQueue m_staging_queue{};
	VkCommandBuffer m_transfer_cmd_buffer;
};

static std::vector<vk_device> vk_devices{};
vk_device& get_vk_device();

#endif


#ifdef CUDA
class cudaDevice final : public device {
public: 
	cudaDevice();
	~cudaDevice();
	
private:
};
#endif

device& get_avalible_device(int device_type = -1);
