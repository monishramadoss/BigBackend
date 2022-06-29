#include "compute.h"
#include "compute_manager.h"

compute_job::compute_job(std::string compute_type) : local_task_id(global_task_id++), m_type(std::move(compute_type))
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
	std::cout << "setting input: " << this << std::endl;
	global_compute_manager.set_input(this, input);
}

void compute_job::set_output(tensor& output)
{
	std::cout << "setting output: " << this << std::endl;
	global_compute_manager.set_output(this, output);
}


void compute_job::run() { state = 2; }


vulkan_compute_object::vulkan_compute_object(int num_buffers, const std::string& compute_type) :
	compute_job(compute_type), m_device(get_vk_device())
{
	if (num_buffers <= 0)
		return;
	std::vector<VkDescriptorSetLayoutBinding> bindings(num_buffers);
	for (int i = 0; i < num_buffers; i++)
	{
		bindings[i].binding = i;
		bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[i].descriptorCount = 1;
		bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	}
	VkDescriptorSetLayoutCreateInfo descriptor_set_info = {};
	descriptor_set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_info.bindingCount = num_buffers;
	descriptor_set_info.pBindings = &bindings[0];
	if (vkCreateDescriptorSetLayout(m_device.get_device(), &descriptor_set_info, nullptr, &m_descriptor_set_layout) !=
		VK_SUCCESS)
		throw std::runtime_error("CANNOT CREATE APPROPRIATE NUMBER OF DESCRIPTOR SET LAYOUTS");

	VkDescriptorPoolSize pool_size = {};
	pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_size.descriptorCount = num_buffers;

	VkDescriptorPoolCreateInfo descriptor_pool_info = {};
	descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_info.maxSets = 1;
	descriptor_pool_info.poolSizeCount = 1;
	descriptor_pool_info.pPoolSizes = &pool_size;
	if (vkCreateDescriptorPool(m_device.get_device(), &descriptor_pool_info, nullptr, &m_descriptor_pool) != VK_SUCCESS)
		throw std::runtime_error("CANNOT CREATE DESCRIPTOR POOL");

	VkDescriptorSetAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.descriptorPool = m_descriptor_pool;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts = &m_descriptor_set_layout;
	if (vkAllocateDescriptorSets(m_device.get_device(), &allocate_info, &m_descriptor_set) != VK_SUCCESS)
		throw std::runtime_error("CANNOT CREATE DESCRIPTOR SET");

	VkCommandBufferAllocateInfo command_buffer_alloc_info = {};
	command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_alloc_info.commandPool = m_device.get_cmd_pool();
	command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_alloc_info.commandBufferCount = 1;
	if (vkAllocateCommandBuffers(m_device.get_device(), &command_buffer_alloc_info, &m_cmd_buffer) != VK_SUCCESS)
		throw std::runtime_error("CANNOT ALLOCATE COMMAND BUFFER");
}

vulkan_compute_object::vulkan_compute_object(vulkan_compute_object&) = default;
vulkan_compute_object::vulkan_compute_object(const vulkan_compute_object&) = default;
vulkan_compute_object::vulkan_compute_object(vulkan_compute_object&&) noexcept = default;

vulkan_compute_object::~vulkan_compute_object() = default;

void bindTensor(const VkDevice& device, const VkDescriptorSet& descriptor_set, const VkBuffer& storage,
                size_t size_in_bytes, uint32_t binding)
{
	VkDescriptorBufferInfo desc_buffer_info = {};
	desc_buffer_info.buffer = storage;
	desc_buffer_info.offset = 0;
	desc_buffer_info.range = size_in_bytes;

	VkWriteDescriptorSet write_descriptor_set = {};
	write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set.dstSet = descriptor_set;
	write_descriptor_set.dstBinding = binding;
	write_descriptor_set.descriptorCount = 1;
	write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write_descriptor_set.pBufferInfo = &desc_buffer_info;

	vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);
}


void createShaderModule(VkDevice& device, VkShaderModule* shader_module, const uint32_t* spv, size_t size,
                        const std::string& source)
{
	VkShaderModuleCreateInfo create_info = {};
	if (spv)
	{
		create_info.pCode = spv;
		create_info.codeSize = size;
	}
	else
	{
		std::vector<uint32_t> code = compileSource(source);
		create_info.pCode = code.data();
		create_info.codeSize = sizeof(uint32_t) * code.size();
	}

	if (vkCreateShaderModule(device, &create_info, nullptr, shader_module) != VK_SUCCESS)
		throw std::runtime_error("Not able to create shader code");
}


void vulkan_compute_object::record_command_buffer(void* push_constants, uint32_t push_constants_size,
                                                  const VkSpecializationInfo* specialization_info)
{
	if (m_pipeline == nullptr)
	{
		/*
		*  createShaderModule(_shader, _shader_size);
		   createPipeline(sizeof(param));
		*/

		createShaderModule(m_device.get_device(), &m_shader_module, nullptr, 0, _source);

		VkPipelineShaderStageCreateInfo stage_create_info = {};
		stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stage_create_info.module = m_shader_module;
		stage_create_info.pName = "main";
		stage_create_info.pSpecializationInfo = specialization_info;

		VkPushConstantRange push_constant_ranges[1] = {};
		push_constant_ranges[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		push_constant_ranges[0].offset = 0;
		push_constant_ranges[0].size = push_constants_size;

		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		if (push_constants_size)
		{
			pipeline_layout_create_info.pushConstantRangeCount = 1;
			pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges;
		}

		pipeline_layout_create_info.setLayoutCount = 1;
		pipeline_layout_create_info.pSetLayouts = &m_descriptor_set_layout;
		if (vkCreatePipelineLayout(m_device.get_device(), &pipeline_layout_create_info, nullptr, &m_pipeline_layout) !=
			VK_SUCCESS)
			throw std::runtime_error("cannot create compute pipeline layout");
		VkComputePipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeline_create_info.stage = stage_create_info;
		pipeline_create_info.layout = m_pipeline_layout;
		if (vkCreateComputePipelines(m_device.get_device(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr,
		                             &m_pipeline) != VK_SUCCESS)
			throw std::runtime_error("cannot create compute pipeline");
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	//VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(m_cmd_buffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("cannot begin record command buffer");

	if (push_constants)
		vkCmdPushConstants(m_cmd_buffer, m_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, push_constants_size,
		                   push_constants);

	vkCmdBindPipeline(m_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
	vkCmdBindDescriptorSets(m_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, 0, 1, &m_descriptor_set, 0,
	                        nullptr);
	vkCmdDispatch(m_cmd_buffer, m_group[0], m_group[1], m_group[2]);
	if (vkEndCommandBuffer(m_cmd_buffer))
		throw std::runtime_error("cannot end recording of command buffer");
}


void vulkan_compute_object::execute_command_buffer()
{
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_cmd_buffer;

	VkFenceCreateInfo fence_create_info = {};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.flags = 0;

	vkCreateFence(m_device.get_device(), &fence_create_info, nullptr, &m_fence);
	{
		m_device.lock();
		vkQueueSubmit(m_device.get_queue(), 1, &submit_info, m_fence);
		m_device.unlock();
	}
	vkWaitForFences(m_device.get_device(), 1, &m_fence, VK_TRUE, 10000000);
}
