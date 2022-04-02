#include "compute.h"

compute_job::compute_job() : state(-1), local_task_id(global_task_id++) { }

compute_job::compute_job(compute_job& cj): parallel_kernels(cj.parallel_kernels), state(cj.state), local_task_id(cj.local_task_id) { }

compute_job::compute_job(const compute_job& cj) : parallel_kernels(cj.parallel_kernels), state(cj.state), local_task_id(cj.local_task_id) { }

compute_job::compute_job(compute_job&&) noexcept = default;

compute_job::~compute_job() = default;



void compute_job::split_kernel() 
{
	std::sort(similar.begin(), similar.end());
	std::sort(differ.begin(), differ.end());
}


void compute_job::push_input(tensor& input)
{
	input.set_dst_kernel(this);
	m_inputs.push_back(input);
}

void compute_job::push_output(tensor& output)
{
	output.set_src_kernel(this);
	m_outputs.push_back(output);
}