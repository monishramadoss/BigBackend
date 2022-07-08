#pragma once

#include <vector>
#include <cstring>
#include <iostream>
#include <fstream>

#ifdef VULKAN
#include <vulkan/vulkan.h>

//  void setup_vulkan(VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger, VkPhysicalDevice& physicalDevice,
//       uint32_t& queueFamilyIndex, VkDevice& device, VkQueue& queue);

//#ifdef NDEBUG
//constexpr bool enableValidationLayers = false;
//#else
//constexpr bool enableValidationLayers = true;
//#endif


bool checkValidationLayerSupport();
void createInstance(VkInstance& instance);
void setupDebugMessenger(const VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger);


void createBuffer(const VkDevice& device, uint32_t queueFamilyIndex, VkBuffer& buffer,
                  uint32_t size, uint64_t elem_size = sizeof(float));

void allocateAndBindBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice,
                            const std::vector<VkBuffer*>& buffers,
                            VkDeviceMemory& memory, std::vector<uint64_t>& offsets);

void createPipelineLayout(const VkDevice& device, uint32_t bindingsCount, VkDescriptorSetLayout& setLayout,
                          VkPipelineLayout& pipelineLayout, uint32_t push_constant_size);

void createComputePipeline(const VkDevice& device, const std::string& shaderName,
                           const VkPipelineLayout& pipelineLayout,
                           VkPipeline& pipeline, const std::string& entry_point = "main");

void allocateDescriptorSet(const VkDevice& device, std::vector<VkBuffer*>& buffers,
                           VkDescriptorPool& descriptorPool, const VkDescriptorSetLayout& setLayout,
                           VkDescriptorSet& descriptorSet);

void allocateDescriptorSet(const VkDevice& device, const std::vector<std::vector<VkBuffer*>>& buffers,
                           VkDescriptorPool& descriptorPool, const std::vector<VkDescriptorSetLayout>& setLayout,
                           std::vector<VkDescriptorSet>& descriptorSet);

void createCommandPoolAndBuffer(const VkDevice& device, uint32_t queueFamilyIndex,
                                VkCommandPool& commandPool, VkCommandBuffer& commandBuffer,
                                VkCommandPoolCreateFlags flags = 0);

void createCommandPoolAndBuffer(const VkDevice& device, uint32_t queueFamilyIndex,
                                VkCommandPool& commandPool, std::vector<VkCommandBuffer>& commandBuffer,
                                VkCommandPoolCreateFlags flags = 0);

void recordComputePipeline(const VkCommandBuffer& commandBuffer, const VkPipelineLayout& pipelineLayout,
                           uint32_t push_constant_size, const void* push_constant_vals, const VkPipeline& pipeline,
                           const VkDescriptorSet& descriptorSet, uint32_t x_group, uint32_t y_group, uint32_t z_group,
                           VkCommandBufferUsageFlags flags = 0);

void submitTask(const VkQueue& queue, const VkCommandBuffer* pCommandBuffer, bool wait_for_queue = true);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator);


const std::vector<const char*> validationLayers = {
#ifndef NDEBUG
	"VK_LAYER_KHRONOS_validation"
#endif
};


VKAPI_ATTR inline VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	//std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

inline bool checkValidationLayerSupport()
{
	uint32_t layerCount;
	auto li = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);

	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool found = false;
		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			return false;
		}
	}
	return true;
}

inline std::vector<const char*> getRequiredExtensions()
{
	std::vector<const char*> extensions;
#ifndef NDEBUG
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	return extensions;
}

inline void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

inline void createInstance(VkInstance& instance)
{
#ifndef NDEBUG
	if (!checkValidationLayerSupport())
		throw std::runtime_error("validation layers requested, but not available!");
#endif

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "BigBackend";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.pEngineName = "backend_engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	const auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
#ifndef NDEBUG
	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();
	populateDebugMessengerCreateInfo(debugCreateInfo);
	createInfo.pNext = &debugCreateInfo;
#else
	createInfo.enabledLayerCount = 0;
	createInfo.pNext = nullptr;
#endif

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		//		throw std::runtime_error("failed to create instance!");
	}
}

inline VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,
		"vkCreateDebugUtilsMessengerEXT"));
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

inline void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks* pAllocator)
{
#ifdef NDEBUG
	return;
#else
	const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
#endif
}

inline void setupDebugMessenger(const VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger)
{
#ifdef NDEBUG
	return;
#else
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}
#endif
}

inline bool isDeviceSuitable(VkPhysicalDevice device, uint32_t& queueFamilyIndex)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			queueFamilyIndex = i;
			return true;
		}
		i++;
	}

	return false;
}

inline void pickPhysicalDevice(const VkInstance& instance, VkPhysicalDevice& physicalDevice, uint32_t& queueFamilyIndex)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	physicalDevice = VK_NULL_HANDLE;

	for (const auto& device : devices)
	{
		if (isDeviceSuitable(device, queueFamilyIndex))
		{
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
		throw std::runtime_error("failed to find a suitable GPU!");

	VkPhysicalDeviceProperties gpuProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &gpuProperties);
}

inline void createLogicalDeviceAndQueue(const VkPhysicalDevice& physicalDevice,
                                        const uint32_t& queueFamilyIndex, VkDevice& device, VkQueue& queue)
{
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
	constexpr float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

#ifndef NDEBUG
	deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
#else
	deviceCreateInfo.enabledLayerCount = 0;
#endif
	if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device!");

	vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
}

inline void setup_vulkan(VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger,
                         VkPhysicalDevice& physicalDevice,
                         uint32_t& queueFamilyIndex, VkDevice& device, VkQueue& queue)
{
	createInstance(instance);
	setupDebugMessenger(instance, debugMessenger);
	pickPhysicalDevice(instance, physicalDevice, queueFamilyIndex);
	createLogicalDeviceAndQueue(physicalDevice, queueFamilyIndex, device, queue);
}


inline void createBuffer(const VkDevice& device, uint32_t queueFamilyIndex, VkBuffer& buffer,
                         uint32_t size, uint64_t elem_size)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size * elem_size;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 1;
	bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;

	if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create buffer");
}


inline uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

inline int findMemoryTypeIndex(uint32_t memoryTypeBits, const VkPhysicalDeviceMemoryProperties& properties,
                               bool shouldBeDeviceLocal)
{
	auto lambdaGetMemoryType = [&](VkMemoryPropertyFlags propertyFlags) -> int
	{
		for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
		{
			if ((memoryTypeBits & (1 << i)) && ((properties.memoryTypes[i].propertyFlags & propertyFlags) ==
				propertyFlags))
				return i;
		}
		return -1;
	};


	if (!shouldBeDeviceLocal)
	{
		constexpr VkMemoryPropertyFlags optimal = VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		constexpr VkMemoryPropertyFlags required = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		const int type = lambdaGetMemoryType(optimal);
		if (type == -1)
		{
			const int result = lambdaGetMemoryType(required);
			if (result == -1)
				throw std::runtime_error("Memory type does not find");
			return result;
		}
		return type;
	}
	return lambdaGetMemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}


inline void allocateAndBindBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice,
                                   const std::vector<VkBuffer*>& buffers,
                                   VkDeviceMemory& memory, std::vector<uint64_t>& offsets)
{
	VkDeviceSize requiredMemorySize = 0;
	uint32_t typeFilter = 0;

	for (const VkBuffer* buff : buffers)
	{
		VkMemoryRequirements bufferMemoryRequirements;

		vkGetBufferMemoryRequirements(device, *buff, &bufferMemoryRequirements);
		requiredMemorySize += bufferMemoryRequirements.size;

		if (bufferMemoryRequirements.size % bufferMemoryRequirements.alignment != 0)
		{
			requiredMemorySize += bufferMemoryRequirements.alignment - bufferMemoryRequirements.size %
				bufferMemoryRequirements.alignment;
		}
		typeFilter |= bufferMemoryRequirements.memoryTypeBits;
	}

	const uint32_t memoryTypeIndex = findMemoryType(physicalDevice, typeFilter, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	                                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VkMemoryAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = requiredMemorySize;
	allocateInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(device, &allocateInfo, nullptr, &memory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate buffer memory");

	VkDeviceSize offset = 0;

	for (const VkBuffer* buff : buffers)
	{
		offsets.push_back(static_cast<uint32_t>(offset));

		VkMemoryRequirements bufferMemoryRequirements;
		vkGetBufferMemoryRequirements(device, *buff, &bufferMemoryRequirements);

		if (vkBindBufferMemory(device, *buff, memory, offset) != VK_SUCCESS)
			throw std::runtime_error("failed to bind buffer memory");

		offset += bufferMemoryRequirements.size;
		if (bufferMemoryRequirements.size % bufferMemoryRequirements.alignment != 0)
			offset += bufferMemoryRequirements.alignment - bufferMemoryRequirements.size % bufferMemoryRequirements.
				alignment;
	}
}

inline void createPipelineLayout(const VkDevice& device, uint32_t bindingsCount, VkDescriptorSetLayout& setLayout,
                                 VkPipelineLayout& pipelineLayout, uint32_t push_constant_size)
{
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

	for (uint32_t i = 0; i < bindingsCount; i++)
	{
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = i;
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		layoutBindings.push_back(layoutBinding);
	}

	VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
	setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setLayoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	setLayoutCreateInfo.pBindings = layoutBindings.data();

	if (vkCreateDescriptorSetLayout(device, &setLayoutCreateInfo, nullptr, &setLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set layout!");

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &setLayout;

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.size = push_constant_size;
	pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pushConstantRange.offset = 0;

	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout))
		throw std::runtime_error("failed to create pipeline layout");
}

inline std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	const size_t fileSize = file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

inline void createComputePipeline(const VkDevice& device, const std::string& shaderName,
                                  const VkPipelineLayout& pipelineLayout,
                                  VkPipeline& pipeline, const std::string& entry_point)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo{};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	auto shaderCode = readFile(shaderName);
	shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(shaderCode.data());
	shaderModuleCreateInfo.codeSize = shaderCode.size();

	VkShaderModule shaderModule = VK_NULL_HANDLE;
	if (vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module");
	}

	VkComputePipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pipelineCreateInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineCreateInfo.stage.module = shaderModule;

	pipelineCreateInfo.stage.pName = entry_point.c_str();
	pipelineCreateInfo.layout = pipelineLayout;

	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create compute pipeline");
	}
	vkDestroyShaderModule(device, shaderModule, nullptr);
}

inline void allocateDescriptorSet(const VkDevice& device, std::vector<VkBuffer*>& buffers,
                                  VkDescriptorPool& descriptorPool, const VkDescriptorSetLayout& setLayout,
                                  VkDescriptorSet& descriptorSet)
{
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.maxSets = 1;

	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(buffers.size());

	descriptorPoolCreateInfo.poolSizeCount = 1;
	descriptorPoolCreateInfo.pPoolSizes = &poolSize;
	if (vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool");

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &setLayout;

	if (vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate descriptor sets");


	std::vector<VkWriteDescriptorSet> descriptorSetWrites(buffers.size());
	std::vector<VkDescriptorBufferInfo> bufferInfos(buffers.size());

	uint32_t i = 0;
	for (const VkBuffer* buff : buffers)
	{
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = descriptorSet;
		writeDescriptorSet.dstBinding = i;
		writeDescriptorSet.dstArrayElement = 0;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		VkDescriptorBufferInfo buffInfo{};
		buffInfo.buffer = *buff;
		buffInfo.offset = 0;
		buffInfo.range = VK_WHOLE_SIZE;
		bufferInfos[i] = buffInfo;

		writeDescriptorSet.pBufferInfo = &bufferInfos[i];
		descriptorSetWrites[i] = writeDescriptorSet;
		i++;
	}

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorSetWrites.size()),
	                       descriptorSetWrites.data(), 0, nullptr);
}

inline void allocateDescriptorSet(const VkDevice& device, const std::vector<std::vector<VkBuffer*>>& buffers,
                                  VkDescriptorPool& descriptorPool, const std::vector<VkDescriptorSetLayout>& setLayout,
                                  std::vector<VkDescriptorSet>& descriptorSet)
{
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(buffers.size());

	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	size_t count = 0;

	for (const std::vector<VkBuffer*>& buffer : buffers)
		count += buffer.size();

	poolSize.descriptorCount = static_cast<uint32_t>(count);

	descriptorPoolCreateInfo.poolSizeCount = 1;
	descriptorPoolCreateInfo.pPoolSizes = &poolSize;

	if (vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool");
	}

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(setLayout.size());
	descriptorSetAllocateInfo.pSetLayouts = setLayout.data();

	if (vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, descriptorSet.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets");
	}

	uint32_t j = 0;
	for (const std::vector<VkBuffer*>& buffer : buffers)
	{
		std::vector<VkWriteDescriptorSet> descriptorSetWrites(buffer.size());
		std::vector<VkDescriptorBufferInfo> bufferInfos(buffer.size());

		uint32_t i = 0;
		for (const VkBuffer* buff : buffer)
		{
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = descriptorSet[j];
			writeDescriptorSet.dstBinding = i;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

			VkDescriptorBufferInfo buffInfo{};
			buffInfo.buffer = *buff;
			buffInfo.offset = 0;
			buffInfo.range = VK_WHOLE_SIZE;
			bufferInfos[i] = buffInfo;

			writeDescriptorSet.pBufferInfo = &bufferInfos[i];
			descriptorSetWrites[i] = writeDescriptorSet;
			i++;
		}

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorSetWrites.size()), descriptorSetWrites.data(), 0,
		                       nullptr);
		j++;
	}
}

inline void createCommandPoolAndBuffer(const VkDevice& device, uint32_t queueFamilyIndex,
                                       VkCommandPool& commandPool, VkCommandBuffer& commandBuffer,
                                       VkCommandPoolCreateFlags flags)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = flags;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;

	if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool");
	}

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType =
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffer");
	}
}

inline void createCommandPoolAndBuffer(const VkDevice& device, uint32_t queueFamilyIndex,
                                       VkCommandPool& commandPool, std::vector<VkCommandBuffer>& commandBuffer,
                                       VkCommandPoolCreateFlags flags)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = flags;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;

	if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool");
	}

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType =
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffer.size());

	if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffer.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffer");
	}
}

inline void recordComputePipeline(const VkCommandBuffer& commandBuffer, const VkPipelineLayout& pipelineLayout,
                                  uint32_t push_constant_size, const void* push_constant_vals,
                                  const VkPipeline& pipeline,
                                  const VkDescriptorSet& descriptorSet, uint32_t x_group, uint32_t y_group,
                                  uint32_t z_group,
                                  VkCommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.flags = flags;
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin command buffer");
	}

	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, push_constant_size,
	                   push_constant_vals);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0,
	                        nullptr);
	vkCmdDispatch(commandBuffer, x_group, y_group, z_group);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to end command buffer");
	}
}

inline void submitTask(const VkQueue& queue, const VkCommandBuffer* pCommandBuffer, bool wait_for_queue)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = pCommandBuffer;
	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	// TODO: use fence?
	if (wait_for_queue) vkQueueWaitIdle(queue);
}

#endif
