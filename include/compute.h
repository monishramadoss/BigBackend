#pragma once

#include "device.h"
#include "tensor.h"


#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>
#include <functional>


// TODO construct compute flow graph
// TODO construct data flow graph from compute flow
// TODO update compute flow graph with updated data flow with sync and async pipelines

inline size_t alignSize(size_t sz, int n) { return (sz + n - 1) & -n; }


class compute_job : public std::enable_shared_from_this<compute_job>
{
private:
	void split_kernel();

protected:
	std::vector<compute_job> parallel_kernels;
	int state{}; // -1 = error 0 = running 1 = transit 2 = complete

public:
	compute_job(std::string);
	compute_job(compute_job&);
	compute_job(const compute_job&);
	compute_job(compute_job&&) noexcept;

	compute_job& operator=(const compute_job&);

	size_t device_id;
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



static std::vector<uint32_t> compile(const std::string& source, char* filename = nullptr)
{
	char tmp_filename_in[L_tmpnam_s];
	char tmp_filename_out[L_tmpnam_s];

	tmpnam_s(tmp_filename_in, L_tmpnam_s);
	tmpnam_s(tmp_filename_out, L_tmpnam_s);

	FILE* tmp_file = nullptr;
	fopen_s(&tmp_file, tmp_filename_in, "wb+");
	fputs(source.c_str(), tmp_file);
	fclose(tmp_file);

	fopen_s(&tmp_file, tmp_filename_out, "wb+");
	fclose(tmp_file);

	const std::string cmd_str = std::string("glslangValidator -V " + std::string(tmp_filename_in) + " -S comp -o " + tmp_filename_out);

	if (auto system_return = system(cmd_str.c_str()))
		throw std::runtime_error("Error running glslangValidator command");
	std::ifstream fileStream(tmp_filename_out, std::ios::binary);
	std::vector<char> buffer;
	buffer.insert(buffer.begin(), std::istreambuf_iterator<char>(fileStream), {});
	return { reinterpret_cast<uint32_t*>(buffer.data()), reinterpret_cast<uint32_t*>(buffer.data() + buffer.size()) };
}

//static std::vector<uint32_t> compileSource(const std::string& source)
//{
//	if (system(std::string("glslangValidator --stdin -S comp -V -o tmp_kp_shader.comp.spv << END\n" + source + "\nEND").c_str()))
//		throw std::runtime_error("Error running glslangValidator command");
//	std::ifstream fileStream("./tmp_kp_shader.comp.spv", std::ios::binary);
//	std::vector<unsigned char> buffer;
//	buffer.insert(buffer.begin(), std::istreambuf_iterator<char>(fileStream), {});
//	return {reinterpret_cast<uint32_t*>(buffer.data()), reinterpret_cast<uint32_t*>(buffer.data() + buffer.size())};
//}


void bindTensor(const VkDescriptorSet& descriptor_set, const VkBuffer& buffer, uint32_t binding);


class vulkan_compute_job : public compute_job, public std::enable_shared_from_this<vulkan_compute_job>
{
private:
	VkShaderModule m_shader_module{};
	VkPipeline m_pipeline{};
	VkPipelineLayout m_pipeline_layout{};
	VkCommandBuffer m_cmd_buffer{};

	VkDescriptorPool m_descriptor_pool{};
	VkDescriptorSet m_descriptor_set{};
	VkDescriptorSetLayout m_descriptor_set_layout{};

	VkFence m_fence{};

protected:
	vk_device m_device;
	void bind_tensor(tensor& t, uint32_t binding);
	void record_command_buffer(const void* push_constants, uint32_t push_constants_size,
	                           const VkSpecializationInfo* specialization_info = nullptr);
	void execute_command_buffer();
	virtual void compute_group_size() = 0;
	uint32_t m_group[3] = { 1, 1, 1 };

public:
	vulkan_compute_job(int num_buffers, vk_device& device, const std::string& compute_type = "empty vulkan");

	vulkan_compute_job(vulkan_compute_job&);
	vulkan_compute_job(const vulkan_compute_job&);
	vulkan_compute_job(vulkan_compute_job&&) noexcept;

	~vulkan_compute_job() override;
	void run() override = 0;
	std::string _source;
};

#endif
