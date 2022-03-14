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
	return 0;	
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
	return 0;
}

#endif


_int used_memory{0};

device::device() : num_threads(std::thread::hardware_concurrency()), device_id(devices.size()), global_rank(0), local_rank(0), max_memory_size(getTotalSystemMemory()),
                   used_memory(0)
{
	pool.setThreadCount(num_threads);
}

device::device(device& d) : num_threads(std::thread::hardware_concurrency()), device_id(d.device_id), global_rank(d.global_rank), local_rank(d.local_rank),
                            max_memory_size(d.max_memory_size), used_memory(d.used_memory)
{
	pool.setThreadCount(num_threads);
}

device::device(const device& d) : num_threads(std::thread::hardware_concurrency()), device_id(d.device_id), global_rank(d.global_rank), local_rank(d.local_rank),
                                  max_memory_size(d.max_memory_size), used_memory(d.used_memory)
{
	pool.setThreadCount(num_threads);
}

device::device(device&& d) noexcept : num_threads(std::thread::hardware_concurrency()), device_id(d.device_id), global_rank(d.global_rank), local_rank(d.local_rank),
                                      max_memory_size(d.max_memory_size), used_memory(d.used_memory)
{
	pool.setThreadCount(num_threads);
}


void device::transfer_data(byte_* t, device& dev)
{
	_int size = memory_sector[t];
	_int offset;
	byte_* ptr = dev.allocate_memory(size, offset);
	memcpy(ptr, t, size);
}


void device::free_memory(byte_* data)
{
	memory_sector.erase(data);
	free(data);
}

byte_* device::allocate_memory(_int size, _int& offset)
{
	if (max_memory_size == offset)
		throw std::bad_alloc();
	used_memory += size;
	auto* ptr = static_cast<byte_*>(malloc(size));
	//	auto* ptr = static_cast<byte_*>((!used_memory ? malloc(size) : std::realloc(host_memory.get(), used_memory)));

	if (ptr == nullptr)
		throw std::bad_alloc();
	/*
	if (host_memory == nullptr)
		host_memory = std::shared_ptr<byte_>(ptr);
	*/

	memory_sector[ptr] = size;
	return ptr;
}

bool device::is_avalible(int d_type)
{
	if (device_type != d_type)
		return false;
	if (used_memory >= max_memory_size)
		return false;
	return true;
}

void device::addJob(std::function<void()> job)
{
	
}

device::~device()
{
	//for (const auto& [fst, snd] : memory_sector)
	//{
	//	if (fst != nullptr);
	//		//free(fst);
	//}
};

ThreadPool device::pool = ThreadPool();
std::map<byte_*, _int> device::memory_sector = {};
std::shared_ptr<byte_> device::host_memory = nullptr;
std::vector<device> device::devices = {};
std::vector<std::vector<float>> device::transfer_latency = {};
std::vector<std::vector<float>> device::compute_latency = {};

device& get_avalible_device(int device_type)
{
	if (device::devices.empty())
		device::devices.emplace_back();


	for (auto& dev : device::devices)
	{
		if (dev.is_avalible(device_type))
			return dev;
	}			
	device::transfer_latency = std::vector<std::vector<float>>(device::devices.size(), std::vector<float>(device::devices.size(), 0));
	device::compute_latency = std::vector<std::vector<float>>(device::devices.size(), std::vector<float>(device::devices.size(), 0));

	
	return device::devices[0];
}
