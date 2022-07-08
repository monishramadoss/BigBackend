#pragma once

/// TODO REPLACE INDEXING WITH STRINGS;

#include "device.h"
#include "device_memory.h"

#include <map>
#include <vector>
#include <memory.h>

//TODO TRIGGER when to fire off to device data

class compute_manager;

class storage_manager
{
public:
	storage_manager();

	void* allocate_block(size_t size);
	void free_block(void* ptr);
	void copy_block(void* dst, void* src, size_t size = 0);
	
private:
	std::vector<off_device_cache> sideline_data;

#ifdef VULKAN
	
public:
	VkBuffer& get_vk_buffer(void* ptr);
#endif
	std::map<void*, int> device_map;
	int current_device = 1;
	
	friend compute_manager;
};

static storage_manager global_store_manager = storage_manager();
