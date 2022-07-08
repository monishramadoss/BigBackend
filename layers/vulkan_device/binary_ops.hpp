#pragma once

#include "compute.h"
#include "init.h"
#include "types.h"
#include "tensor.h"

#include <chrono>
#include <stdexcept>
#include <string>


#ifdef VULKAN
#include <vulkan.hpp>

struct binary_op_param
{
	uint32_t total;
};


std::string binary_op_template = R"(
#version 460
layout(push_constant) uniform pushblock {
    uint total;
};
layout(binding=0) readonly buffer input_1 { float A[]; };
layout(binding=1) readonly buffer input_2 { float B[]; };
layout(binding=2) writeonly buffer output_1 { float C[]; };
layout(local_size_x = 1024, local_size_y = 1, local_size_z=1) in;

void main() {
	for(uint i = gl_GlobalInvocationID.x; i < total; i += gl_NumWorkGroups.x * gl_WorkGroupSize.x){
        C[i] = A[i] + B[i];        
    }
};

)";

namespace vk
{
	class binary_op : public vulkan_compute_job // operate on n dim
	{
	public:
		binary_op();
		tensor operator()(tensor&, tensor&);
		virtual void kernel(tensor&, tensor&, tensor&);
		void run() override { execute_command_buffer(); }

		void compute_group_size() override {
			m_group[0] = static_cast<uint32_t>(alignSize(m_param.total, 1024)) / 1024;
			if (m_group[0] > m_device.m_max_work_group_count[0])
				m_group[0] = m_device.m_max_work_group_count[0] - 1;
		}


		std::string m_type = "naive_cpu_empty";
		binary_op_param m_param{};
	};

	class add : public binary_op
	{
	public:
		add() : binary_op()
		{
			m_type = "vulkan_cpu_add";
			_source = binary_op_template;
		}
	};


	class sub : public binary_op
	{
	public:
		sub() : binary_op()
		{
			m_type = "vulkan_gpu_sub";
			_source = binary_op_template;
		}
	};


	class mul : public binary_op
	{
	public:
		mul() : binary_op()
		{
			m_type = "vulkan_gpu_mul";
			_source = binary_op_template;
		}
	};


	class true_div : public binary_op
	{
	public:
		true_div() : binary_op()
		{
			m_type = "vulkan_gpu_div";
			_source = binary_op_template;
		}
	};
}


#endif


// IMPLMENTATION CODE


namespace vk
{
	inline binary_op::binary_op() : vulkan_compute_job(3, *(vk_device*)device::device_lst[1])
	{
		
	}

	inline void binary_op::kernel(tensor& x, tensor& y, tensor& z)
	{
		auto* p1 = x.get_data();
		auto* p2 = y.get_data();
		auto* p3 = z.get_data();

		bind_tensor(x, 0);
		bind_tensor(y, 1);
		bind_tensor(z, 2);
		m_param.total = static_cast<uint32_t>(x.size());
		record_command_buffer((void*)&m_param, sizeof(m_param));
	}
}
