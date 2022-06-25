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


vulkan_compute_object::vulkan_compute_object(int num_buffers, const std::string& compute_type) : compute_job(compute_type), device(get_vk_device())
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
	if (vkCreateDescriptorSetLayout(device.get_device(), &descriptor_set_info, nullptr, &m_descriptor_set_layout) !=
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
	if (vkCreateDescriptorPool(device.get_device(), &descriptor_pool_info, nullptr, &m_descriptor_pool) != VK_SUCCESS)
		throw std::runtime_error("CANNOT CREATE DESCRIPTOR POOL");

	VkDescriptorSetAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.descriptorPool = m_descriptor_pool;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts = &m_descriptor_set_layout;
	if (vkAllocateDescriptorSets(device.get_device(), &allocate_info, &m_descriptor_set) != VK_SUCCESS)
		throw std::runtime_error("CANNOT CREATE DESCRIPTOR SET");

	//VkCommandBufferAllocateInfo command_buffer_alloc_info = {};
	//command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	////command_buffer_alloc_info.commandPool = device.get_cmd_pool();
	//command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	//command_buffer_alloc_info.commandBufferCount = 1;
	//if (vkAllocateCommandBuffers(device.get_device(), &command_buffer_alloc_info, &m_command_buffer) != VK_SUCCESS)
	//    throw std::runtime_error("CANNOT ALLOCATE COMMAND BUFFER");
}

vulkan_compute_object::vulkan_compute_object(vulkan_compute_object&) = default;
vulkan_compute_object::vulkan_compute_object(const vulkan_compute_object&) = default;
vulkan_compute_object::vulkan_compute_object(vulkan_compute_object&&) noexcept = default;

vulkan_compute_object::~vulkan_compute_object() = default;

void bindTensor(const VkDescriptorSet& descriptor_set, const VkBuffer& storage, size_t size_in_bytes, uint32_t binding)
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
}
