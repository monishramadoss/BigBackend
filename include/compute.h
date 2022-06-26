#pragma once

#include "device.h"
#include "tensor.h"


#include <vector>
#include <numeric>
#include <memory>
#include <functional>


// TODO construct compute flow graph
// TODO construct data flow graph from compute flow
// TODO update compute flow graph with updated data flow with sync and async pipelines


class compute_job : public std::enable_shared_from_this<compute_job>
{
private:
	void split_kernel();

protected:
	std::vector<compute_job> parallel_kernels;
	device dev = get_avalible_device();
	int state{}; // -1 = error 0 = running 1 = transit 2 = complete
public:
	compute_job(std::string);
	
	compute_job(compute_job&);
	compute_job(const compute_job&);
	compute_job(compute_job&&) noexcept;

	compute_job& operator=(const compute_job&);

	size_t local_task_id;
	std::string m_type = "empty";

	dim_vec similar;
	dim_vec differ;

	void set_input(tensor& input);
	void set_output(tensor& output);
	
	std::shared_ptr<compute_job> getptr() { return shared_from_this(); }
	// ReSharper disable once CppSmartPointerVsMakeFunction
	[[nodiscard]] static std::shared_ptr<compute_job> create(const std::string& compute_type)
	{
		return std::shared_ptr<compute_job>(new compute_job(compute_type));
	}

	virtual void run();
	virtual ~compute_job();
};


#ifdef VULKAN
#include <vulkan/vulkan.h>
#include "vulkan.hpp"

static std::vector<uint32_t> compileSource(const std::string& source)
{
	if (system(
		std::string("glslangValidator --stdin -S comp -V -o tmp_kp_shader.comp.spv << END\n" + source + "\nEND").
		c_str()))
		throw std::runtime_error("Error running glslangValidator command");
	std::ifstream fileStream("./tmp_kp_shader.comp.spv", std::ios::binary);
	std::vector<unsigned char> buffer;
	buffer.insert(buffer.begin(), std::istreambuf_iterator<char>(fileStream), {});
	return {reinterpret_cast<uint32_t*>(buffer.data()), reinterpret_cast<uint32_t*>(buffer.data() + buffer.size())};
}


void bindTensor(const VkDescriptorSet& descriptor_set, const VkBuffer& buffer, uint32_t binding);


class vulkan_compute_object : public compute_job, public std::enable_shared_from_this<vulkan_compute_object>
{
private:
	vk_device device;
	VkShaderModule m_shader_module{};
	VkPipelineLayout m_pipeline_layout{};
	VkCommandBuffer m_command_buffer{};

	VkDescriptorPool m_descriptor_pool{};
	VkDescriptorSet m_descriptor_set{};
	VkDescriptorSetLayout m_descriptor_set_layout{};


public:
	vulkan_compute_object(int num_buffers, const std::string& compute_type="empty vulkan");

	vulkan_compute_object(vulkan_compute_object&);
	vulkan_compute_object(const vulkan_compute_object&);
	vulkan_compute_object(vulkan_compute_object&&) noexcept;

	~vulkan_compute_object() override;
	void run() override = 0;
};

#endif
