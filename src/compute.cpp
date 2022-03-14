#include "compute.h"

compute_job::compute_job() : state(-1), local_task_id(global_task_id++)
{
	compute_adj_list.emplace_back();
}

compute_job::compute_job(compute_job& cj): parallel_kernels(cj.parallel_kernels), state(cj.state), local_task_id(cj.local_task_id)
{
}

compute_job::compute_job(const compute_job& cj) : parallel_kernels(cj.parallel_kernels), state(cj.state), local_task_id(cj.local_task_id)
{
}


#ifdef VULKAN_ML

#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

void vkml_init();

static VkDevice device;
static VkInstance instance;
static VkQueue queue;
static VkPhysicalDevice physicalDevice;
static uint32_t computeQueueFamilyIndex;
static VkBuffer buffers[2];
static VkDeviceMemory memory;

static void create_instance(){
	VkApplicationInfo appInfo;
	memset(&appInfo, 0, sizeof(appInfo));
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
	VkInstanceCreateInfo instanceCreateInfo;
	const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
	memset(&instanceCreateInfo, 0, sizeof(instanceCreateInfo));
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
	instanceCreateInfo.enabledLayerCount = 1;
	// We add the "VK_KHR_get_physical_device_properties2" only if it is present in the list of available extensions because that's what the spec says
	const char* instanceExtensionName = "VK_KHR_get_physical_device_properties2";
	uint32_t numInstanceExtensions = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &numInstanceExtensions, NULL);
	VkExtensionProperties* instanceExtensions = malloc(sizeof(VkExtensionProperties) * numInstanceExtensions);
	vkEnumerateInstanceExtensionProperties(NULL, &numInstanceExtensions, instanceExtensions);
	for(uint32_t i = 0; i < numInstanceExtensions; i++){
		if(!strcmp(instanceExtensions[i].extensionName, instanceExtensionName)){
			instanceCreateInfo.ppEnabledExtensionNames = &instanceExtensionName;
			instanceCreateInfo.enabledExtensionCount = 1;
		}
	}
	VkResult result = vkCreateInstance(&instanceCreateInfo, NULL, &instance);
	#ifdef DEBUG
	if(result != VK_SUCCESS){
		printf("Failed to create instance with error code: %i\n", result);
	}
	#endif

	free(instanceExtensions);
}

static void pick_physical_device(){
	uint32_t numPhysicalDevices = 0;
	vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, NULL);
	VkPhysicalDevice* physicalDevices = malloc(sizeof(VkPhysicalDevice) * numPhysicalDevices);
	vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, physicalDevices);
	physicalDevice = physicalDevices[0];
	free(physicalDevices);
}

static void find_compute_queue_family_index(){
	uint32_t numQueueFamilyProperties = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilyProperties, NULL);
	VkQueueFamilyProperties* queueFamilyProperties = malloc(sizeof(VkQueueFamilyProperties) * numQueueFamilyProperties);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilyProperties, queueFamilyProperties);
	for(uint32_t i = 0; i < numQueueFamilyProperties; i++){
		if(queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT){
			computeQueueFamilyIndex = i;
			break;
		}
	}
}

static void create_device_and_queue(){
	// Create device and queue
	VkDeviceQueueCreateInfo queueCreateInfo;
	memset(&queueCreateInfo, 0, sizeof(queueCreateInfo));
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = computeQueueFamilyIndex;
	float priority = 1.0f;
	queueCreateInfo.pQueuePriorities = &priority;
	queueCreateInfo.queueCount = 1;
	VkDeviceCreateInfo deviceCreateInfo;
	memset(&deviceCreateInfo, 0, sizeof(deviceCreateInfo));
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	// We add the "VK_KHR_portability_subset" only if it is present in the list of available extensions because that's what the spec says
	const char* deviceExtensionName = "VK_KHR_portability_subset";
	uint32_t numDeviceExtensions = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &numDeviceExtensions, NULL);
	VkExtensionProperties* deviceExtensions = malloc(sizeof(VkExtensionProperties) * numDeviceExtensions);
	vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &numDeviceExtensions, deviceExtensions);
	for(uint32_t i = 0; i < numDeviceExtensions; i++){
		if(!strcmp(deviceExtensions[i].extensionName, deviceExtensionName)){
			deviceCreateInfo.ppEnabledExtensionNames = &deviceExtensionName;
			deviceCreateInfo.enabledExtensionCount = 1;
		}
	}
	VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device);
	#ifdef DEBUG
	if(result != VK_SUCCESS){
		printf("Failed to create device with error code: %i\n", result);
	}
	#endif

	free(deviceExtensions);
}

static void find_device_queue(){
	vkGetDeviceQueue(device, computeQueueFamilyIndex, 0, &queue);
}

static void create_buffers_and_allocate_memory(){
	VkDeviceSize size = pow(2, 24);
	uint32_t memoryTypeIndex;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		const VkMemoryType memoryType = memoryProperties.memoryTypes[i];

		if ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & memoryType.propertyFlags)
			&& (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & memoryType.propertyFlags)
			&& (size < memoryProperties.memoryHeaps[memoryType.heapIndex].size)) {
			memoryTypeIndex = i;
		}
	}

	VkBufferCreateInfo bufferCreateInfo;
	memset(&bufferCreateInfo, 0, sizeof(bufferCreateInfo));
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result;
	result = vkCreateBuffer(device, &bufferCreateInfo, NULL, buffers);
	#ifdef DEBUG
	if(result != VK_SUCCESS){
		printf("Failed to create vulkan ubo buffer\n");
	}
	#endif
	result = vkCreateBuffer(device, &bufferCreateInfo, NULL, buffers + 1);
	#ifdef DEBUG
	if(result != VK_SUCCESS){
		printf("Failed to create vulkan ubo buffer\n");
	}
	#endif

	VkMemoryAllocateInfo memAllocInfo;
	memset(&memAllocInfo, 0, sizeof(memAllocInfo));
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = size * 2;
	memAllocInfo.memoryTypeIndex = memoryTypeIndex;

	result = vkAllocateMemory(device, &memAllocInfo, NULL, &memory);
	#ifdef DEBUG
	if(result != VK_SUCCESS){
		printf("Failed to allocate device memory\n");
	}
	#endif

	vkBindBufferMemory(device, buffers[0], memory, 0);
	vkBindBufferMemory(device, buffers[1], memory, size);
}

void vkml_init(){
	create_instance();
	pick_physical_device();
	find_compute_queue_family_index();
	create_device_and_queue();
	find_device_queue();
	create_buffers_and_allocate_memory();

	void* payload;
	vkMapMemory(device, memory, 0, pow(2, 24), 0, &payload);
	memset(payload, 0, pow(2, 24));
	vkUnmapMemory(device, memory);
}

#endif