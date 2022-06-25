#pragma once
#include <chrono>
#include <stdexcept>
#include <string>

#include "compute.h"
#include "init.h"
#include "types.h"
#include "tensor.h"


#ifdef VULKAN
#include <vulkan.hpp>


std::string binary_op_template = "";

namespace vk
{
	class binary_op : public vulkan_compute_object // operate on n dim
	{
	public:
		binary_op();
		tensor operator()(tensor&, tensor&);
		virtual void kernel(tensor&, tensor&, tensor&) = 0;
		void run() override {};
		std::string m_type = "naive_cpu_empty";
	};

	class add : public binary_op
	{
	public:
		add() : binary_op() { m_type = "vulkan_cpu_add"; }
		void kernel(tensor&, tensor&, tensor&) override;
	};


	class sub : public binary_op
	{
	public:
		sub() : binary_op() { m_type = "vulkan_gpu_sub"; }
		void kernel(tensor&, tensor&, tensor&) override;
	};


	class mul : public binary_op
	{
	public:
		mul() : binary_op() { m_type = "vulkan_gpu_mul"; }
		void kernel(tensor&, tensor&, tensor&) override;
	};


	class true_div : public binary_op
	{
	public:
		true_div() : binary_op() { m_type = "vulkan_gpu_div"; }
		void kernel(tensor&, tensor&, tensor&) override;
	};
}


#endif


// IMPLMENTATION CODE


namespace vk
{
	inline binary_op::binary_op() : vulkan_compute_object(3)
	{
	}

	inline void add::kernel(tensor& x, tensor& w, tensor& y)
	{
	}

	inline void sub::kernel(tensor& x, tensor& w, tensor& y)
	{
	}

	inline void mul::kernel(tensor& x, tensor& w, tensor& y)
	{
	}

	inline void true_div::kernel(tensor& x, tensor& w, tensor& y)
	{
	}
}
