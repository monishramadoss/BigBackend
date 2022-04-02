#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include <vulkan/vulkan.h>

//  void setup_vulkan(VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger, VkPhysicalDevice& physicalDevice,
//       uint32_t& queueFamilyIndex, VkDevice& device, VkQueue& queue);

bool checkValidationLayerSupport();
void createInstance(VkInstance& instance);
void setupDebugMessenger(const VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger);




void createBuffer(const VkDevice& device, uint32_t queueFamilyIndex, VkBuffer& buffer,
    uint32_t n, uint32_t m, uint64_t elem_size = sizeof(float));

void allocateAndBindBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const std::vector<VkBuffer*>& buffers,
    VkDeviceMemory& memory, std::vector<uint64_t>& offsets);

void createPipelineLayout(const VkDevice& device, uint32_t bindingsCount, VkDescriptorSetLayout& setLayout,
    VkPipelineLayout& pipelineLayout, uint32_t push_constant_size);

void createComputePipeline(const VkDevice& device, const std::string& shaderName, const VkPipelineLayout& pipelineLayout,
    VkPipeline& pipeline, const std::string& entry_point = "main");

void allocateDescriptorSet(const VkDevice& device, std::vector<VkBuffer*>& buffers,
    VkDescriptorPool& descriptorPool, const VkDescriptorSetLayout& setLayout, VkDescriptorSet& descriptorSet);

void allocateDescriptorSet(const VkDevice& device, const std::vector<std::vector<VkBuffer*>>& buffers,
    VkDescriptorPool& descriptorPool, const std::vector<VkDescriptorSetLayout>& setLayout, std::vector<VkDescriptorSet>& descriptorSet);

void createCommandPoolAndBuffer(const VkDevice& device, uint32_t queueFamilyIndex,
    VkCommandPool& commandPool, VkCommandBuffer& commandBuffer, VkCommandPoolCreateFlags flags = 0);

void createCommandPoolAndBuffer(const VkDevice& device, uint32_t queueFamilyIndex,
    VkCommandPool& commandPool, std::vector<VkCommandBuffer>& commandBuffer, VkCommandPoolCreateFlags flags = 0);

void recordComputePipeline(const VkCommandBuffer& commandBuffer, const VkPipelineLayout& pipelineLayout,
                           uint32_t push_constant_size, const void* push_constant_vals, const VkPipeline& pipeline,
                           const VkDescriptorSet& descriptorSet, uint32_t x_group, uint32_t y_group, uint32_t z_group,
                           VkCommandBufferUsageFlags flags = 0);

void submitTask(const VkQueue& queue, const VkCommandBuffer* pCommandBuffer, bool wait_for_queue = true);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);


const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
};


VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

inline bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);

    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool found = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }
    return true;
}

inline std::vector<const char*> getRequiredExtensions() {

    std::vector<const char*> extensions;

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

inline void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
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

inline void createInstance(VkInstance& instance) {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "BigBackend";
    appInfo.engineVersion = VK_API_VERSION_1_2;
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_API_VERSION_1_2;
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    const auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

inline VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugUtilsMessengerEXT* pDebugMessenger) {
    const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,
                                                                                                 "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

inline void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    if constexpr (!enableValidationLayers) return;

    const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

inline void setupDebugMessenger(const VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger) {
    if constexpr (!enableValidationLayers) return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

inline bool isDeviceSuitable(VkPhysicalDevice device, uint32_t& queueFamilyIndex) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queueFamilyIndex = i;
            return true;
        }
        i++;
    }

    return false;
}

inline void pickPhysicalDevice(const VkInstance& instance, VkPhysicalDevice& physicalDevice, uint32_t& queueFamilyIndex) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    physicalDevice = VK_NULL_HANDLE;

    for (const auto& device : devices) {
        if (isDeviceSuitable(device, queueFamilyIndex)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    VkPhysicalDeviceProperties gpuProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &gpuProperties);

    std::cout << "Using device: " << gpuProperties.deviceName << '\n';
}

inline void createLogicalDeviceAndQueue(const VkPhysicalDevice& physicalDevice,
                                        const uint32_t& queueFamilyIndex, VkDevice& device, VkQueue& queue) {

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

    if (enableValidationLayers) {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
}

inline void setup_vulkan(VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger, VkPhysicalDevice& physicalDevice,
                         uint32_t& queueFamilyIndex, VkDevice& device, VkQueue& queue) {

    createInstance(instance);
    setupDebugMessenger(instance, debugMessenger);
    pickPhysicalDevice(instance, physicalDevice, queueFamilyIndex);
    createLogicalDeviceAndQueue(physicalDevice, queueFamilyIndex, device, queue);
}

inline void createBuffer(const VkDevice& device, uint32_t queueFamilyIndex, VkBuffer& buffer,
                         uint32_t n, uint32_t m, uint64_t elem_size) {
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = n * m * elem_size;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 1;
    bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;

    if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer");
    }
}

inline uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

inline void allocateAndBindBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const std::vector<VkBuffer*>& buffers,
                                   VkDeviceMemory& memory, std::vector<uint64_t>& offsets) {

    VkDeviceSize requiredMemorySize = 0;
    uint32_t typeFilter = 0;

    for (const VkBuffer* buff : buffers) {
        VkMemoryRequirements bufferMemoryRequirements;

        vkGetBufferMemoryRequirements(device, *buff, &bufferMemoryRequirements);
        requiredMemorySize += bufferMemoryRequirements.size;

        if (bufferMemoryRequirements.size % bufferMemoryRequirements.alignment != 0) {
            requiredMemorySize += bufferMemoryRequirements.alignment - bufferMemoryRequirements.size % bufferMemoryRequirements.alignment;
        }
        typeFilter |= bufferMemoryRequirements.memoryTypeBits;
    }

    const uint32_t memoryTypeIndex = findMemoryType(physicalDevice, typeFilter, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = requiredMemorySize;
    allocateInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(device, &allocateInfo, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory");
    }

    VkDeviceSize offset = 0;

    for (const VkBuffer* buff : buffers) {
        offsets.push_back(static_cast<uint32_t>(offset));

        VkMemoryRequirements bufferMemoryRequirements;
        vkGetBufferMemoryRequirements(device, *buff, &bufferMemoryRequirements);

        if (vkBindBufferMemory(device, *buff, memory, offset) != VK_SUCCESS) {
            throw std::runtime_error("failed to bind buffer memory");
        }

        offset += bufferMemoryRequirements.size;
        if (bufferMemoryRequirements.size % bufferMemoryRequirements.alignment != 0) {
            offset += bufferMemoryRequirements.alignment - bufferMemoryRequirements.size % bufferMemoryRequirements.alignment;
        }
    }
}

inline void createPipelineLayout(const VkDevice& device, uint32_t bindingsCount, VkDescriptorSetLayout& setLayout,
                                 VkPipelineLayout& pipelineLayout, uint32_t push_constant_size) {
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

    for (uint32_t i = 0; i < bindingsCount; i++) {
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

    if (vkCreateDescriptorSetLayout(device, &setLayoutCreateInfo, nullptr, &setLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

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

    if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout)) {
        throw std::runtime_error("failed to create pipeline layout");
    }
}

inline std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    const size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

inline void createComputePipeline(const VkDevice& device, const std::string& shaderName, const VkPipelineLayout& pipelineLayout,
                                  VkPipeline& pipeline, const std::string& entry_point) {
    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    auto shaderCode = readFile(shaderName);
    shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(shaderCode.data());
    shaderModuleCreateInfo.codeSize = shaderCode.size();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module");
    }

    VkComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineCreateInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineCreateInfo.stage.module = shaderModule;

    pipelineCreateInfo.stage.pName = entry_point.c_str();
    pipelineCreateInfo.layout = pipelineLayout;

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline");
    }
    vkDestroyShaderModule(device, shaderModule, nullptr);
}

inline void allocateDescriptorSet(const VkDevice& device, std::vector<VkBuffer*>& buffers,
                                  VkDescriptorPool& descriptorPool, const VkDescriptorSetLayout& setLayout, VkDescriptorSet& descriptorSet) {
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.maxSets = 1;

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(buffers.size());

    descriptorPoolCreateInfo.poolSizeCount = 1;
    descriptorPoolCreateInfo.pPoolSizes = &poolSize;
    if (vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool");
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &setLayout;

    if (vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets");
    }


    std::vector<VkWriteDescriptorSet> descriptorSetWrites(buffers.size());
    std::vector<VkDescriptorBufferInfo> bufferInfos(buffers.size());

    uint32_t i = 0;
    for (const VkBuffer* buff : buffers) {
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

    vkUpdateDescriptorSets(device, descriptorSetWrites.size(), descriptorSetWrites.data(), 0, nullptr);
}

inline void allocateDescriptorSet(const VkDevice& device, const std::vector<std::vector<VkBuffer*>>& buffers,
                                  VkDescriptorPool& descriptorPool, const std::vector<VkDescriptorSetLayout>& setLayout, std::vector<VkDescriptorSet>& descriptorSet) {
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(buffers.size());

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    uint32_t count = 0;

    for (const std::vector<VkBuffer*>& buffer : buffers) {
        count += buffer.size();
    }

    poolSize.descriptorCount = count;

    descriptorPoolCreateInfo.poolSizeCount = 1;
    descriptorPoolCreateInfo.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool");
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(setLayout.size());
    descriptorSetAllocateInfo.pSetLayouts = setLayout.data();

    if (vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, descriptorSet.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets");
    }

    uint32_t j = 0;
    for (const std::vector<VkBuffer*>& buffer : buffers) {

        std::vector<VkWriteDescriptorSet> descriptorSetWrites(buffer.size());
        std::vector<VkDescriptorBufferInfo> bufferInfos(buffer.size());

        uint32_t i = 0;
        for (const VkBuffer* buff : buffer) {
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

        vkUpdateDescriptorSets(device, descriptorSetWrites.size(), descriptorSetWrites.data(), 0, nullptr);
        j++;
    }
}

inline void createCommandPoolAndBuffer(const VkDevice& device, uint32_t queueFamilyIndex,
                                       VkCommandPool& commandPool, VkCommandBuffer& commandBuffer, VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = flags;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;

    if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool");
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer");
    }
}

inline void createCommandPoolAndBuffer(const VkDevice& device, uint32_t queueFamilyIndex,
                                       VkCommandPool& commandPool, std::vector<VkCommandBuffer>& commandBuffer, VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = flags;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;

    if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool");
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffer.size());

    if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffer.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer");
    }
}

inline void recordComputePipeline(const VkCommandBuffer& commandBuffer, const VkPipelineLayout& pipelineLayout,
                                  uint32_t push_constant_size, const void* push_constant_vals, const VkPipeline& pipeline,
                                  const VkDescriptorSet& descriptorSet, uint32_t x_group, uint32_t y_group, uint32_t z_group,
                                  VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.flags = flags;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin command buffer");
    }

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, push_constant_size, push_constant_vals);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDispatch(commandBuffer, x_group, y_group, z_group);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to end command buffer");
    }
}

inline void submitTask(const VkQueue& queue, const VkCommandBuffer* pCommandBuffer, bool wait_for_queue) {
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = pCommandBuffer;
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    // TODO: use fence?
    if (wait_for_queue) vkQueueWaitIdle(queue);
}












	
class Counter
{
public:
    Counter() = default;
    Counter(Counter const &counter);
    Counter(Counter &&counter) = default;
    Counter &operator=(Counter counter);
 
    uint32_t getCount() const;
 
    virtual ~Counter();
protected:
    std::shared_ptr<uint32_t> mCount = std::make_shared<uint32_t>(1);
};
 
Counter::Counter(const Counter &counter) :
    mCount(counter.mCount) {
    ++(*mCount);
}
 
Counter &Counter::operator =(Counter counter) {
    using std::swap;
    swap(mCount, counter.mCount);
    return *this;
}
 
uint32_t Counter::getCount() const {
    return *mCount;
}
 
Counter::~Counter() {
 
}

	
class VkResource : public Counter
{
public:
    VkResource() = default;
    VkResource(Device const &device);
    VkResource(VkResource &&vkResource) = default;
    VkResource(VkResource const &vkResource) = default;
    VkResource &operator=(VkResource &&vkResource) = default;
    VkResource &operator=(VkResource const &vkResource) = default;
 
    vk::Device getDevice() const;
 
protected:
    std::shared_ptr<Device> mDevice;
};
 
 
VkResource::VkResource(const Device &device) :
    mDevice(std::make_shared<Device>(device)) {
 
}
 
vk::Device VkResource::getDevice() const {
    return *mDevice;
}

class Buffer : public VkResource, public vk::Buffer
{
public:
    Buffer() = default;

    Buffer(Device &device, vk::BufferUsageFlags usage, vk::DeviceSize size, std::shared_ptr<AbstractAllocator> allocator, bool shouldBeDeviceLocal) 
    {
                        
        int memoryTypeIndex = findMemoryType(mRequirements->memoryTypeBits, *mProperties, shouldBeDeviceLocal);

        *mBlock = mAllocator->allocate(mRequirements->size, mRequirements->alignment, memoryTypeIndex);
        mDevice->bindBufferMemory(m_buffer, mBlock->memory, mBlock->offset);

        // if host_visible, we can map it
        if(!shouldBeDeviceLocal)
            *mPtr = mDevice->mapMemory(mBlock->memory, mBlock->offset, *mSize, vk::MemoryMapFlags());

        vk::BufferCreateInfo createInfo(vk::BufferCreateFlags(), *mSize, *mUsage, vk::SharingMode::eExclusive);

        m_buffer = mDevice->createBuffer(createInfo);
        *mRequirements = mDevice->getBufferMemoryRequirements(m_buffer);

    }

    Buffer(Buffer &&buffer) = default;
    Buffer(Buffer const &buffer) = default;
    Buffer &operator=(Buffer const &buffer);

    vk::DeviceSize getSize() const;
    vk::BufferUsageFlags getUsage() const;
    bool isDeviceLocal() const;
    void *getPtr();
    std::shared_ptr<AbstractAllocator> getAllocator();

    ~Buffer();

private:
    std::shared_ptr<AbstractAllocator> mAllocator;
    std::shared_ptr<vk::DeviceSize> mSize = std::make_shared<vk::DeviceSize>();
    std::shared_ptr<vk::BufferUsageFlags> mUsage = std::make_shared<vk::BufferUsageFlags>();
    std::shared_ptr<vk::MemoryRequirements> mRequirements = std::make_shared<vk::MemoryRequirements>();
    std::shared_ptr<vk::PhysicalDeviceMemoryProperties> mProperties = std::make_shared<vk::PhysicalDeviceMemoryProperties>();
    std::shared_ptr<Block> mBlock = std::make_shared<Block>();
    std::shared_ptr<bool> mIsDeviceLocal;
    std::shared_ptr<void *> mPtr = std::make_shared<void *>(nullptr);

    void createBuffer();
    void allocate(bool shouldBeDeviceLocal);
};

int findMemoryType(uint32_t memoryTypeBits,   vk::PhysicalDeviceMemoryProperties const &properties,  bool shouldBeDeviceLocal) {
 
    auto lambdaGetMemoryType = [&](vk::MemoryPropertyFlags propertyFlags) -> int {
        for(uint32_t i = 0; i < properties.memoryTypeCount; ++i)
            if((memoryTypeBits & (1 << i)) &&
            ((properties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags))
                return i;
        return -1;
    };
 
    if(!shouldBeDeviceLocal) {
        vk::MemoryPropertyFlags optimal = vk::MemoryPropertyFlagBits::eHostCached |
                vk::MemoryPropertyFlagBits::eHostCoherent |
                vk::MemoryPropertyFlagBits::eHostVisible;
 
        vk::MemoryPropertyFlags required = vk::MemoryPropertyFlagBits::eHostCoherent |
                vk::MemoryPropertyFlagBits::eHostVisible;
 
        int type = lambdaGetMemoryType(optimal);
        if(type == -1) {
            int result = lambdaGetMemoryType(required);
            if(result == -1)
                assert(!"Memory type does not find");
            return result;
        }
        return type;
    }
 
    else
        return lambdaGetMemoryType(vk::MemoryPropertyFlagBits::eDeviceLocal);
}


class ObserverCommandBufferSubmitter {
public:
    virtual void notify() = 0;
};
 
class CommandBufferSubmitter
{
public:
    CommandBufferSubmitter(Device &device, uint32_t numberCommandBuffers);
 
    void addObserver(ObserverCommandBufferSubmitter *observer);
 
    vk::CommandBuffer createCommandBuffer();
 
    void submit();
    void wait();
 
protected:
    std::shared_ptr<Device> mDevice;
    std::shared_ptr<vk::Queue> mQueue;
    std::shared_ptr<CommandPool> mCommandPool;
    std::shared_ptr<std::vector<vk::CommandBuffer>> mCommandBuffers = std::make_shared<std::vector<vk::CommandBuffer>>();
    std::shared_ptr<Fence> mFence;
    std::shared_ptr<uint32_t> mIndex = std::make_shared<uint32_t>(0);
    std::shared_ptr<std::vector<ObserverCommandBufferSubmitter*>> mObservers = std::make_shared<std::vector<ObserverCommandBufferSubmitter*>>();
};
 
CommandBufferSubmitter::CommandBufferSubmitter(Device &device, uint32_t numberCommandBuffers) :
    mDevice(std::make_shared<Device>(device)),
    mQueue(std::make_shared<vk::Queue>(device.getTransferQueue())),
    mCommandPool(std::make_shared<CommandPool>(device, true, true, device.getIndexTransferQueue())),
    mFence(std::make_shared<Fence>(device, false)) {
    *mCommandBuffers = mCommandPool->allocate(vk::CommandBufferLevel::ePrimary, numberCommandBuffers);
}
 
void CommandBufferSubmitter::addObserver(ObserverCommandBufferSubmitter *observer) {
    mObservers->emplace_back(observer);
}
 
vk::CommandBuffer CommandBufferSubmitter::createCommandBuffer() {
    if(*mIndex >= mCommandBuffers->size()) {
        auto buffers = mCommandPool->allocate(vk::CommandBufferLevel::ePrimary, 10);
 
        for(auto &b : buffers)
            mCommandBuffers->emplace_back(b);
    }
 
    return (*mCommandBuffers)[(*mIndex)++];
}
 
void CommandBufferSubmitter::submit() {
    vk::SubmitInfo info;
    info.setCommandBufferCount(*mIndex).setPCommandBuffers(mCommandBuffers->data());
    mFence->reset();
    mQueue->submit(info, *mFence);
}
 
void CommandBufferSubmitter::wait() {
    *mIndex = 0;
    mFence->wait();
    mFence->reset();
    for(auto &observer : *mObservers)
        observer->notify();
}


	
class BufferTransferer : public ObserverCommandBufferSubmitter
{
public:
    BufferTransferer(Device &device, uint32_t numberBuffers, vk::DeviceSize sizeTransfererBuffers,
                     std::shared_ptr<AbstractAllocator> allocator, CommandBufferSubmitter &commandBufferSubmitter);
 
    void transfer(const Buffer &src, Buffer &dst,
                  vk::DeviceSize offsetSrc,
                  vk::DeviceSize offsetDst,
                  vk::DeviceSize size);
 
    void transfer(Buffer &buffer, vk::DeviceSize offset, vk::DeviceSize size, void *data);
 
    void notify();
 
private:
    std::shared_ptr<CommandBufferSubmitter> mCommandBufferSubmitter;
    std::shared_ptr<std::vector<Buffer>> mTransfererBuffers = std::make_shared<std::vector<Buffer>>();
    std::shared_ptr<uint32_t> mSizeTransfererBuffers;
    std::shared_ptr<uint32_t> mIndex = std::make_shared<uint32_t>(0);
};

void BufferTransferer::notify() {
    *mIndex = 0;
}
 
void BufferTransferer::transfer(Buffer const &src, Buffer &dst,
                                vk::DeviceSize offsetSrc, vk::DeviceSize offsetDst,
                                vk::DeviceSize size) {
    // Check if size and usage are legals
    assert((src.getUsage() & vk::BufferUsageFlagBits::eTransferSrc) ==
                vk::BufferUsageFlagBits::eTransferSrc);
    assert((dst.getUsage() & vk::BufferUsageFlagBits::eTransferDst) ==
                vk::BufferUsageFlagBits::eTransferDst);
 
    assert(src.getSize() >= (offsetSrc + size));
    assert(dst.getSize() >= (offsetDst + size));
 
    // Prepare the region copied
    vk::BufferCopy region(offsetSrc, offsetDst, size);
 
    vk::CommandBufferBeginInfo begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
 
    vk::CommandBuffer cmd = mCommandBufferSubmitter->createCommandBuffer();
 
    cmd.begin(begin);
    cmd.copyBuffer(src, dst, {region});
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eAllCommands,
                        vk::DependencyFlags(),
                        nullptr,
                        vk::BufferMemoryBarrier(vk::AccessFlagBits::eTransferWrite,
                                                vk::AccessFlagBits::eMemoryRead,
                                                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                                                dst, offsetSrc, size),
                        nullptr);
    cmd.end();
 
}
 
void BufferTransferer::transfer(Buffer &buffer, vk::DeviceSize offset, vk::DeviceSize size, void *data) {
    if(*mIndex == mTransfererBuffers->size()) {
        mCommandBufferSubmitter->submit();
        mCommandBufferSubmitter->wait();
    }
    assert(size <= *mSizeTransfererBuffers);
    memcpy((*mTransfererBuffers)[*mIndex].getPtr(), data, size);
    transfer((*mTransfererBuffers)[*mIndex], buffer, 0, offset, size);
    (*mIndex)++;
}



CommandBufferSubmitter commandBufferSubmitter(device, 1);
BufferTransferer bufferTransferer(device, 1, 1 << 20, deviceAllocator, commandBufferSubmitter);
glm::vec2 quad[] = {glm::vec2(-1, -1), glm::vec2(1, -1), glm::vec2(-1, 1), glm::vec2(1, 1)};
Buffer vbo(device, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, sizeof quad, deviceAllocator, true);
bufferTransferer.transfer(vbo, 0, sizeof quad, quad);
commandBufferSubmitter.submit();
commandBufferSubmitter.wait();













	
struct Block {
    vk::DeviceMemory memory;
    vk::DeviceSize offset;
    vk::DeviceSize size;
    bool free;
    void *ptr = nullptr; // Useless if it is a GPU allocation
 
    bool operator==(Block const &block);
};
bool Block::operator==(Block const &block) {
    if(memory == block.memory &&
       offset == block.offset &&
       size == block.size &&
       free == block.free &&
       ptr == block.ptr)
        return true;
    return false;
}


class Chunk : private NotCopyable {
public:
    Chunk(Device &device, vk::DeviceSize size, int memoryTypeIndex);
 
    bool allocate(vk::DeviceSize size, Block &block);
    bool isIn(Block const &block) const;
    void deallocate(Block const &block);
    int memoryTypeIndex() const;
 
    ~Chunk();
 
protected:
    Device mDevice;
    vk::DeviceMemory mMemory = VK_NULL_HANDLE;
    vk::DeviceSize mSize;
    int mMemoryTypeIndex;
    std::vector<Block> mBlocks;
    void *mPtr = nullptr;
};


Chunk::Chunk(Device &device, vk::DeviceSize size, int memoryTypeIndex) :
    mDevice(device),
    mSize(size),
    mMemoryTypeIndex(memoryTypeIndex) {
    vk::MemoryAllocateInfo allocateInfo(size, memoryTypeIndex);
 
    Block block;
    block.free = true;
    block.offset = 0;
    block.size = size;
    mMemory = block.memory = device.allocateMemory(allocateInfo);
 
    if((device.getPhysicalDevice().getMemoryProperties().memoryTypes[memoryTypeIndex].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) == vk::MemoryPropertyFlagBits::eHostVisible)
        mPtr = device.mapMemory(mMemory, 0, VK_WHOLE_SIZE);
 
    mBlocks.emplace_back(block);
}


void Chunk::deallocate(const Block &block) {
    auto blockIt(std::find(mBlocks.begin(), mBlocks.end(), block));
    assert(blockIt != mBlocks.end());
    // Just put the block to free
    blockIt->free = true;
}
 
bool Chunk::allocate(vk::DeviceSize size, vk::DeviceSize alignment, Block &block) {
    // if chunk is too small
    if(size > mSize)
        return false;
 
    for(uint32_t i = 0; i < mBlocks.size(); ++i) {
        if(mBlocks[i].free) {
            // Compute virtual size after taking care about offsetAlignment
            uint32_t newSize = mBlocks[i].size;
 
            if(mBlocks[i].offset % alignment != 0)
                newSize -= alignment - mBlocks[i].offset % alignment;
 
            // If match
            if(newSize >= size) {
 
                // We compute offset and size that care about alignment (for this Block)
                mBlocks[i].size = newSize;
                if(mBlocks[i].offset % alignment != 0)
                    mBlocks[i].offset += alignment - mBlocks[i].offset % alignment;
 
                // Compute the ptr address
                if(mPtr != nullptr)
                    mBlocks[i].ptr = (char*)mPtr + mBlocks[i].offset;
 
                // if perfect match
                if(mBlocks[i].size == size) {
                    mBlocks[i].free = false;
                    block = mBlocks[i];
                    return true;
                }
 
                Block nextBlock;
                nextBlock.free = true;
                nextBlock.offset = mBlocks[i].offset + size;
                nextBlock.memory = mMemory;
                nextBlock.size = mBlocks[i].size - size;
                mBlocks.emplace_back(nextBlock); // We add the newBlock
 
                mBlocks[i].size = size;
                mBlocks[i].free = false;
 
                block = mBlocks[i];
                return true;
            }
        }
    }
 
    return false;
}


	
class ChunkAllocator 
{
public:
    ChunkAllocator(Device &device, vk::DeviceSize size);
 
    // if size > mSize, allocate to the next power of 2
    std::unique_ptr<Chunk> allocate(vk::DeviceSize size, int memoryTypeIndex);
 
private:
    Device mDevice;
    vk::DeviceSize mSize;
};
 
vk::DeviceSize nextPowerOfTwo(vk::DeviceSize size) {
    vk::DeviceSize power = (vk::DeviceSize)std::log2l(size) + 1;
    return (vk::DeviceSize)1 << power;
}
 
bool isPowerOfTwo(vk::DeviceSize size) {
    vk::DeviceSize mask = 0;
    vk::DeviceSize power = (vk::DeviceSize)std::log2l(size);
 
    for(vk::DeviceSize i = 0; i < power; ++i)
        mask += (vk::DeviceSize)1 << i;
 
    return !(size & mask);
}
 
ChunkAllocator::ChunkAllocator(Device &device, vk::DeviceSize size) :
    mDevice(device),
    mSize(size) 
{
    assert(isPowerOfTwo(size));
}
 
std::unique_ptr<Chunk> ChunkAllocator::allocate(vk::DeviceSize size, int memoryTypeIndex)
 {
    size = (size > mSize) ? nextPowerOfTwo(size) : mSize; 
    return std::make_unique<Chunk>(mDevice, size, memoryTypeIndex);
}



	
class DeviceAllocator : public AbstractAllocator
{
public:
    DeviceAllocator(Device device, vk::DeviceSize size);
 
    Block allocate(vk::DeviceSize size, vk::DeviceSize alignment, int memoryTypeIndex);
    void deallocate(Block &block);
 
 
private:
    ChunkAllocator mChunkAllocator;
    std::vector<std::shared_ptr<Chunk>> mChunks;
};

	
DeviceAllocator::DeviceAllocator(Device device, vk::DeviceSize size) :
    AbstractAllocator(device),
    mChunkAllocator(device, size) {
 
}
 
Block DeviceAllocator::allocate(vk::DeviceSize size, vk::DeviceSize alignment, int memoryTypeIndex) {
    Block block;
    // We search a "good" chunk
    for(auto &chunk : mChunks)
        if(chunk->memoryTypeIndex() == memoryTypeIndex)
            if(chunk->allocate(size, alignment, block))
                return block;
 
    mChunks.emplace_back(mChunkAllocator.allocate(size, memoryTypeIndex));
    assert(mChunks.back()->allocate(size, alignment, block));
    return block;
}
 
void DeviceAllocator::deallocate(Block &block) {
    for(auto &chunk : mChunks) {
        if(chunk->isIn(block)) {
            chunk->deallocate(block);
            return ;
        }
    }
    assert(!"unable to deallocate the block");
}