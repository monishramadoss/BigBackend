#pragma once

#include "compute.h"
#include "tensor.h"
#include "storage_manager.h"


#include <map>
#include <vector>


class compute_manager
{
public:
	compute_manager() = default;

	void set_input(compute_job* cj, tensor& input)
	{
		input_map[cj].push_back(input.get_data());
	}

	void set_output(compute_job* cj, tensor& output)
	{
		output_map[cj].push_back(output.get_data());
	}

	block* get_input(compute_job* cj, size_t i = 0)
	{
		void* ptr = input_map[cj].at(i);
		return devices[cj->device_id]->get_block(ptr);
	}

	block* get_output(compute_job* cj, size_t i = 0)
	{
		void* ptr = output_map[cj].at(i);
		return devices[cj->device_id]->get_block(ptr);
	}

	void run_kernels() const
	{
		if (!pipeline_setup || starter == nullptr)
		{

		}
		for (const auto kernel : run_queue)
		{
			kernel->run();
		}
	}

	void emplace_kernel(compute_job* cj)
	{
		input_map[cj] = {};
		output_map[cj] = {};
	}


    static std::map<std::string, compute_job*> kernel_map;
#ifdef VULKAN
	static std::map<std::string, vulkan_compute_job*> vk_kernel_map;
#endif


private:
	std::map<compute_job*, std::vector<void*>> input_map;
	std::map<compute_job*, std::vector<void*>> output_map;
	
	std::vector<compute_job*> run_queue;
	compute_job* starter = nullptr;
	bool pipeline_setup = false;
};

static compute_manager global_compute_manager = compute_manager();
