#pragma once

#define VULKAN


#include "types.h"
#include "thread_pool.h"
#include "device_memory.h"

#include <mutex>
#include <stdexcept>
#include <vector>
#include <memory>
#include <functional>
#include <map>

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
	size_t m_num_threads;

	virtual void* malloc(size_t size);
	virtual void free(void* ptr);
	virtual void memcpy(void* dst, void* src, size_t size=0);

	virtual bool offload(void* ptr) { return true; }
	virtual bool upload(void* ptr) { return true; }

	virtual void* get_buffer(void* ptr) { return nullptr; }
	block* get_block(void* ptr);

	virtual void transfer(device& dev);

	static std::vector<device*> device_lst;

	_int type() const { return device_id; }

protected:
	_int device_id;
	_int global_rank;
	_int local_rank;
	_int m_max_memory_size;
	static host_allocator m_allocator;
	static ThreadPool m_pool;
	int m_device_type = -1;

	std::map<void*, block*> memory_map;
};

std::vector<device*>& init_devices();
static std::vector<device*> devices; // = init_devices();

#ifdef VULKAN
#include <vulkan/vulkan.h>


class vk_device : public device
{
public:
	vk_device(const VkInstance& instance, const VkPhysicalDevice& pDevice);
	vk_device(const vk_device&);
	vk_device(vk_device&&) noexcept;
	vk_device& operator=(const vk_device&);
	~vk_device() override;

	VkDevice& get_device() { return m_device; }
	[[nodiscard]] VkPhysicalDeviceMemoryProperties get_mem_properties() const { return m_memory_properties; }
	uint32_t m_max_work_group_count[3];
	uint32_t m_max_work_group_size[3];

	VkCommandPool& get_cmd_pool() { return m_cmd_pool; }
	VkQueue& get_queue() { return m_cmd_queue; }

	void lock() { this->device_lock.lock(); }
	void unlock() { this->device_lock.unlock(); }


	void* malloc(size_t size) override;
	void free(void* ptr) override;
	void memcpy(void* src, void* dst, size_t size=0) override;

	bool upload(void* ptr) override;
	bool offload(void* ptr) override;

	void* get_buffer(void* ptr) override;
	
private:
	VkInstance m_instance{};
	VkPhysicalDevice m_physical_device{};
	VkPhysicalDeviceProperties m_device_properties{};
	VkPhysicalDeviceMemoryProperties m_memory_properties{};
	VkDebugUtilsMessengerEXT m_debug_messenger{};

	std::vector<uint32_t> m_compute_queue_index;
	std::vector<uint32_t> m_staging_queue_index;

	VkDevice m_device{};
	VkQueue m_cmd_queue{};
	VkCommandPool m_cmd_pool{};
	VkCommandPool m_transfer_pool{};

	_int m_max_device_memory_size{0};
	VkQueue m_staging_queue{};
	VkCommandBuffer m_staging_cmd_buffer;

	vk_allocator m_allocator;

	std::mutex device_lock;
	std::map<block*, vk_block*> device_memory_map;

	bool is_on_device(block* blk);
};


static std::vector<vk_device> vk_devices{};
inline std::vector<vk_device>& get_vk_devices() { return vk_devices; }

#endif


#ifdef CUDA
class cudaDevice final : public device {
public: 
	cudaDevice();
	~cudaDevice();
	
private:
};
#endif

//device& get_avalible_device(int device_type = -1);
