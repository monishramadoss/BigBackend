#pragma once

#define VULKAN

#include <stdexcept>
#include <vector>
#include <map>
#include <memory>
#include <functional>

#include "types.h"
#include "thread_pool.h"
#include "device_memory.h"

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

	//virtual char* down_set(_int begin_offset = 0, _int end_offset = -1);
	//virtual void up_set(char* data, _int begin_offset = 0, _int end_offset = -1);

	virtual block* allocate_memory(_int size);
	virtual void free_memory(block*);

	virtual void copy_memory(block* blk, size_t dst_offset, const byte_* src, size_t n);
	virtual void copy_memory(block* dst, block* src);

	[[nodiscard]] bool is_avalible(int d_type = -1) const;

	void add_job(std::function<void()> job) = delete;

	static std::vector<std::vector<float>> m_transfer_latency;
	static std::vector<std::vector<float>> m_compute_latency;


	std::map<std::shared_ptr<compute_job>, std::vector<std::shared_ptr<tensor>>> src_map;
	std::map<std::shared_ptr<tensor>, std::shared_ptr<compute_job>> dst_map;

	void set_input(const std::shared_ptr<tensor>& input, const std::shared_ptr<compute_job>& cmp);
	void set_output(const std::shared_ptr<tensor>& output, const std::shared_ptr<compute_job>& cmp);

	std::shared_ptr<tensor> get_input(size_t i, const std::shared_ptr<compute_job>& cmp) const;
	std::shared_ptr<tensor> get_output(size_t i, std::shared_ptr<compute_job>& cmp);

	virtual ~device();
	int m_device_type = -1;
	size_t m_num_threads;

protected:
	_int device_id;
	_int global_rank;
	_int local_rank;
	_int m_max_memory_size;
	_int m_used_memory;
	static device_allocator m_allocator;
	static ThreadPool m_pool;
};

static std::vector<device> devices{};

#ifdef VULKAN
#include <vulkan/vulkan.h>

class vk_device final : public device
{
public:
	vk_device(const VkInstance& instance, const VkPhysicalDevice& pDevice);
	vk_device(const vk_device&);
	vk_device(vk_device&&) noexcept;
	~vk_device() override;

	block* allocate_memory(_int size) override;
	vk_block* allocate_device_memory(const VkBuffer* buffer) const;

	void free_memory(block* blk) override;
	void free_device_memory(vk_block*) const;

	void copy_memory(block* blk, size_t dst_offset, const byte_* src, size_t n) override;
	void copy_memory(block* dst, block* src) override;
	void copy_memory(vk_block* dst, block* src);
	void copy_memory(block* dst, vk_block* src);

	vk_block* get_device_memory(block* blk) const;
	void set_device_memory(vk_block* dblk, block* hblk);
	[[nodiscard]] VkDevice get_device() const;
	[[nodiscard]] VkPhysicalDeviceMemoryProperties get_mem_properties() const;
	int m_max_work_group_size[3];
	int m_max_work_groups[3];


private:
	void create_staging_buffer();

	VkInstance m_instance;
	VkPhysicalDevice m_physical_device;
	VkDebugUtilsMessengerEXT m_debug_messenger{};

	std::vector<uint32_t> m_compute_queue_index;
	std::map<block*, vk_block*> m_device_map;

	VkPhysicalDeviceProperties m_device_properties{};
	VkPhysicalDeviceMemoryProperties m_memory_properties{};

	VkDevice m_device{};

	VkQueue m_cmd_queue{};

	VkCommandPool m_cmd_pool{};

	std::shared_ptr<vk_device_allocator> m_device_allocator;
	_int m_max_device_memory_size{0};

	std::vector<uint32_t> m_staging_queue_index;
	VkQueue m_staging_queue{};


	VkBuffer m_staging_buffer;
	VkDeviceMemory m_staging_memory;
	size_t m_staging_size = 4096;
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
