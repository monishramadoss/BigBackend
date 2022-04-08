#include "compute.h"

compute_job::compute_job() : local_task_id(global_task_id++)
{
}

compute_job::compute_job(device& dev) : local_task_id(global_task_id++)
{
}

compute_job::compute_job(compute_job& cj) : local_task_id(cj.local_task_id), m_type(cj.m_type)
{
}


compute_job::compute_job(const compute_job& cj) : local_task_id(cj.local_task_id), m_type(cj.m_type)
{
}

compute_job::compute_job(compute_job&&) noexcept = default;

compute_job& compute_job::operator=(const compute_job& cj)
{
	local_task_id = cj.local_task_id;
	m_type = cj.m_type;
	return *this;
}


compute_job::~compute_job() = default;


void compute_job::split_kernel()
{
	std::sort(similar.begin(), similar.end());
	std::sort(differ.begin(), differ.end());
}


void compute_job::set_input(tensor& input)
{
	dev.set_input(input.getptr(), this->getptr());
}

void compute_job::set_output(tensor& output)
{
	dev.set_output(output.getptr(), this->getptr());
}


std::shared_ptr<tensor> compute_job::get_input(size_t i)
{
	return dev.get_input(i, this->getptr());
}

std::shared_ptr<tensor> compute_job::get_output(size_t i)
{
	return dev.get_output(i, this->getptr());
}

void compute_job::run() { state = 2; }


vulkan_compute_object::vulkan_compute_object() : compute_job()
{
}

vulkan_compute_object::vulkan_compute_object(vulkan_compute_object&)
{
}

vulkan_compute_object::vulkan_compute_object(const vulkan_compute_object&) = default;


vulkan_compute_object::vulkan_compute_object(vulkan_compute_object&&) noexcept = default;

vulkan_compute_object::~vulkan_compute_object() = default;
