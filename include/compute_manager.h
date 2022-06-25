#pragma once
#include <map>
#include <vector>

#include "compute.h"
#include "tensor.h"
#include "storage_manager.h"

class compute_manager {
public:
	compute_manager() = default;

	void set_input(compute_job* cj, tensor& input)
	{
		input_map[cj].push_back(global_store_manager.host_mapping[input.get_data()]);
	}

	void set_output(compute_job* cj, tensor& output)
	{
		output_map[cj].push_back(global_store_manager.host_mapping[output.get_data()]);
	}

	block* get_input(compute_job* cj, size_t i = 0)
	{
		return input_map[cj].at(i);
	}

	block* get_output(compute_job* cj, size_t i = 0)
	{
		return output_map[cj].at(i);
	}
	
	void run_kernels()
	{
		if (!pipeline_setup || starter == nullptr)
		{

		}
		for(const auto kernel: run_queue)
		{
			kernel->run();
		}
	}

	void emplace_kernel(compute_job* cj)
	{
		input_map[cj] = {};
		output_map[cj] = {};
	}

private:
	std::map<compute_job*, std::vector<block*>> input_map;
	std::map<compute_job*, std::vector<block*>> output_map;

	std::vector<compute_job*> run_queue;
	compute_job* starter = nullptr;
	bool pipeline_setup = false;
};

static compute_manager global_compute_manager = compute_manager();
