#pragma once
#include <stdexcept>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include "types.h"
#include "thread_pool.h"


class device
{
public:
	device();
	device(device&);
	device(const device&);
	device(device&&) noexcept;
	device& operator=(const device&) = default;

	virtual void transfer_data(byte_* t, device& dev);
	//virtual char* down_set(_int begin_offset = 0, _int end_offset = -1);
	//virtual void up_set(char* data, _int begin_offset = 0, _int end_offset = -1);

	virtual void free_memory(byte_*);
	virtual byte_* allocate_memory(_int size, _int& offset);
	virtual bool is_avalible(int d_type = -1);
	void addJob(std::function<void()> job);

	static std::vector<device> devices;
	static std::vector<std::vector<float>> transfer_latency;
	static std::vector<std::vector<float>> compute_latency;
	~device();
	int device_type = -1;
	size_t num_threads;

protected:

	_int device_id;
	_int global_rank;
	_int local_rank;
	_int max_memory_size;
	_int used_memory;

	static ThreadPool pool;
	static std::shared_ptr<byte_> host_memory;
	static std::map<byte_*, _int> memory_sector;

};




#ifdef VULKAN
#include <vulkan/vulkan.h>

class vkDevice : public device {
public:
	vkDevice();
	~vkDevice();
	void copy_data(const char* data);
	int device_type=1;
	VkInstance m_instance;
private:
	VkDevice device;
	VkPhysicalDevice physical_device;
	std::vector<VkQueue> queue;
	VkCommandPool cmdpool;
	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceMemoryProperties memory_properties;
};
#endif


#ifdef CUDA
class cudaDevice : public device {
public: 
	cudaDevice();
	~cudaDevice();
	int device_type=2;

private:
};
#endif

device& get_avalible_device(int device_type=-1);