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



struct node
{
	node(void* p, char t): ptr(p), type(t) {}
	bool operator== (const node& n) const { return ptr == n.ptr && type == n.type; }
	bool operator== (void* p) const { return ptr == p; }
	void* ptr;
	char type;
};


//std::vector<std::vector<size_t>> compute_flow = {};
//std::vector<std::vector<size_t>> data_flow = {};

static size_t global_tensor_id{ 0 };
static size_t global_task_id{ 0 };

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

	virtual void free_memory(block*);
	virtual block* allocate_memory(_int size);
	virtual void copy_memory(block* blk, size_t dst_offset, const byte_* src, size_t n);
	virtual void copy_memory(block* dst, block* src);

	bool is_avalible(int d_type = -1) const;

	void add_job(std::function<void()> job) = delete;

	static std::vector<node> m_object_store;
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
	_int m_used_memory;
	static device_allocator m_allocator;
	static ThreadPool m_pool;

};

static std::vector<device> devices {};



#ifdef VULKAN
#include <vulkan/vulkan.h>

class vk_device final : public device {
public:
	vk_device(const VkInstance& instance, const VkPhysicalDevice& pDevice);

	vk_device(const vk_device&) = default;
	vk_device(vk_device&&) = default;
	~vk_device() override;
	
	void free_memory(vk_block*) const;
	vk_block* allocate_memory(const VkBuffer* buffer) const;
	[[nodiscard]] VkDevice get_device() const { return m_device; }
	[[nodiscard]] VkPhysicalDeviceMemoryProperties get_mem_properties() const { return m_memory_properties;  }
	/*

	void free_memory(block*) override;
	block* allocate_memory(_int size) override;
	bool is_avalible(int d_type) override;

	*/

private:
	VkInstance m_instance;
	VkPhysicalDevice m_physical_device;
	VkDebugUtilsMessengerEXT m_debug_messenger{};

	std::vector<uint32_t> m_compute_queue_index;
	std::vector<uint32_t> m_transfer_queue_index;

	VkPhysicalDeviceProperties m_device_properties{};
	VkPhysicalDeviceMemoryProperties m_memory_properties{};

	VkDevice m_device{};
	
	VkQueue m_cmd_queue{};
	VkQueue m_transfer_queue{};

	VkCommandPool m_cmd_pool{};
	VkBuffer transfer_buffer{}

	;
	std::shared_ptr<vk_device_allocator> m_device_allocator;
	int m_memory_type_idx{};
};

static std::vector<vk_device> vk_devices {};

vk_device& get_vk_device();

#endif


#ifdef CUDA
class cudaDevice : public device {
public: 
	cudaDevice();
	~cudaDevice();
	
private:
};
#endif

device& get_avalible_device(int device_type=-1);