#include "Renderer.h"
#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include "FileUtils.h"
#include "TriangleMesh.h"
#include "RenderUtils.h"
#include "BVH.h"
#include <glm/glm.hpp>

#define ENABLE_VALIDATION_LAYERS 1

static float quadVertices[] =
        {
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f
        };

static unsigned int quadIndices[] =
        {
                0, 1, 3,
                1, 2, 3
        };
static float gMouseScrollWheel;
static float gMousePosition[2];
static float gMouseLastPosition[2];
static bool gMouseButtonHeld[3];
static bool gMouseButtonDown[3];
static bool gMouseButtonUp[3];
Camera* gCamera = nullptr;
static void resizeCallback(GLFWwindow *window, int width, int height);
static void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods);
static void mouseScrollCallback(GLFWwindow * window, double xOffset, double yOffset);
static void cursorPosCallback(GLFWwindow* window, double xPos, double yPos);

VkBool32 debugMsgCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
                          size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData)
{
    std::stringstream  message;

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        message << "ERROR";

    if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        message << "WARNING";

    if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        message << "PERFORMANCE";

    if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        message << "INFO";

    if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
        message << "DEBUG";

    message << ": [" << pLayerPrefix << "] Code " << msgCode << ": " << pMsg << std::endl;

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        printf("%s\n",message.str().c_str());
    else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT || flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        printf("%s\n", message.str().c_str());
    else
        printf("%s\n", message.str().c_str());

    return VK_FALSE;
}

struct GPUSceneSeting
{
    unsigned int frameCounr = 0;
    glm::vec3 cameraPosition;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    glm::vec4 cameraParams;
};

struct GPUBVHNode
{
    glm::vec3 bboxMin;
    glm::vec3 bboxMax;
    int numPrimitive;
    int axis;
    int primitiveOffset;
    int secondChildOffset;
};

struct GPUVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 texcoord;
};

struct GPUTriangle
{
    GPUVertex v0;
    GPUVertex v1;
    GPUVertex v2;
    int mat;
};

struct GPUMaterial
{
    glm::vec3 baseColor;
    glm::vec3 emissive;
    float metallic;
    float roughness;
};

Renderer::Renderer(int width, int height)
    :mWidth(width), mHeight(height)
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Star";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
    appInfo.pEngineName = "Star";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

#if ENABLE_VALIDATION_LAYERS
    const char* layers[] ={"VK_LAYER_LUNARG_standard_validation", "VK_LAYER_KHRONOS_validation"};

	const char* extensions[] = {
	        VK_KHR_SURFACE_EXTENSION_NAME,
	        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	        VK_EXT_DEBUG_REPORT_EXTENSION_NAME};

	uint32_t numLayers = sizeof(layers) / sizeof(layers[0]);
#else
    const char** layers = nullptr;
    const char* extensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

    uint32_t numLayers = 0;
#endif
    uint32_t numExtensions = sizeof(extensions) / sizeof(extensions[0]);

    VkInstanceCreateInfo instanceInfo;
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = nullptr;
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledLayerCount = numLayers;
    instanceInfo.ppEnabledLayerNames = layers;
    instanceInfo.enabledExtensionCount = numExtensions;
    instanceInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&instanceInfo, nullptr, &mInstance) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create instance!");
    }

    // setup debug callback
#if ENABLE_VALIDATION_LAYERS
    auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(mInstance, "vkCreateDebugReportCallbackEXT");

    VkDebugReportFlagsEXT debugFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                       VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

    VkDebugReportCallbackCreateInfoEXT debugInfo;
    debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    debugInfo.pNext = nullptr;
    debugInfo.flags = 0;
    debugInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugMsgCallback;
    debugInfo.flags = debugFlags;

    if (vkCreateDebugReportCallbackEXT(mInstance, &debugInfo, nullptr, &mDebugCallback) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug callback!");
    }
#endif

    uint32_t mGpuCount = 0;
    if (vkEnumeratePhysicalDevices(mInstance, &mGpuCount, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to find mGpu!");
    }

    if (mGpuCount == 0)
    {
        throw std::runtime_error("failed to find mGpu!");
    }

    std::vector<VkPhysicalDevice> gpus(mGpuCount);
    if (vkEnumeratePhysicalDevices(mInstance, &mGpuCount, gpus.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to find mGpu!");
        return ;
    }

    for (auto &g : gpus)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(g, &props);
        printf("Found Vulkan GPU: %s\n", props.deviceName);
        printf("    API: %u.%u.%u\n",
             VK_VERSION_MAJOR(props.apiVersion),
             VK_VERSION_MINOR(props.apiVersion),
             VK_VERSION_PATCH(props.apiVersion));
        printf("    Driver: %u.%u.%u\n",
             VK_VERSION_MAJOR(props.driverVersion),
             VK_VERSION_MINOR(props.driverVersion),
             VK_VERSION_PATCH(props.driverVersion));
    }
    mGPU = gpus.front();

    vkGetPhysicalDeviceProperties(mGPU, &mDeviceProperties);
    vkGetPhysicalDeviceFeatures(mGPU, &mDeviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(mGPU, &mMemoryProperties);

    uint32_t numQueueFamilies;
    vkGetPhysicalDeviceQueueFamilyProperties(mGPU, &numQueueFamilies, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(numQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(mGPU, &numQueueFamilies, queueFamilyProperties.data());

    uint32_t graphicsFamily = -1;
    uint32_t computeFamily = -1;
    uint32_t transferFamily = -1;

    for (uint32_t i = 0; i < (uint32_t)queueFamilyProperties.size(); i++)
    {
        if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
        {
            computeFamily = i;
            break;
        }
    }

    for (uint32_t i = 0; i < (uint32_t)queueFamilyProperties.size(); i++)
    {
        if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
            ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
        {
            transferFamily = i;
            break;
        }
    }

    for (uint32_t i = 0; i < (uint32_t)queueFamilyProperties.size(); i++)
    {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsFamily = i;
            break;
        }
    }

    const float graphicsQueuePrio = 0.0f;
    const float computeQueuePrio = 0.1f;
    const float transferQueuePrio = 0.2f;

    std::vector<VkDeviceQueueCreateInfo> queueInfo{};
    queueInfo.resize(3);

    queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo[0].queueFamilyIndex = graphicsFamily;
    queueInfo[0].queueCount = 1;// queueFamilyProperties[graphicsFamily].queueCount;
    queueInfo[0].pQueuePriorities = &graphicsQueuePrio;

    queueInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo[1].queueFamilyIndex = computeFamily;
    queueInfo[1].queueCount = 1;// queueFamilyProperties[computeFamily].queueCount;
    queueInfo[1].pQueuePriorities = &computeQueuePrio;

    queueInfo[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo[2].queueFamilyIndex = transferFamily;
    queueInfo[2].queueCount = 1;// queueFamilyProperties[transferFamily].queueCount;
    queueInfo[2].pQueuePriorities = &transferQueuePrio;

    const char* deviceExtensions[8];
    uint32_t numDeviceExtensions = 0;

    deviceExtensions[numDeviceExtensions++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    deviceExtensions[numDeviceExtensions++] = VK_KHR_MAINTENANCE1_EXTENSION_NAME;
    deviceExtensions[numDeviceExtensions++] = VK_KHR_MAINTENANCE2_EXTENSION_NAME;
    //deviceExtensions[numDeviceExtensions++] = VK_KHR_RAY_TRACING_EXTENSION_NAME;

    bool dedicatedAllocExt = false;
    bool getMemReqExt = false;
    uint32_t numAvailableExtensions = 0;
    vkEnumerateDeviceExtensionProperties(mGPU, nullptr, &numAvailableExtensions, nullptr);
    if (numAvailableExtensions > 0)
    {
        std::vector<VkExtensionProperties> availableExtensions(numAvailableExtensions);
        if (vkEnumerateDeviceExtensionProperties(mGPU, nullptr, &numAvailableExtensions, availableExtensions.data()) == VK_SUCCESS)
        {
            for (auto entry : deviceExtensions)
            {
                if (entry == VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME)
                {
                    deviceExtensions[numDeviceExtensions++] = VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME;
                    dedicatedAllocExt = true;
                }
                else if (entry == VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME)
                {
                    deviceExtensions[numDeviceExtensions++] = VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME;
                    getMemReqExt = true;
                }
            }
        }
    }

    VkDeviceCreateInfo deviceInfo ;
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = nullptr;
    deviceInfo.flags = 0;
    deviceInfo.queueCreateInfoCount = queueInfo.size();
    deviceInfo.pQueueCreateInfos = queueInfo.data();
    deviceInfo.pEnabledFeatures = &mDeviceFeatures;
    deviceInfo.enabledExtensionCount = numDeviceExtensions;
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceInfo.enabledLayerCount = 0;
    deviceInfo.ppEnabledLayerNames = nullptr;

    if (vkCreateDevice(mGPU, &deviceInfo, nullptr, &mDevice) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(mDevice, graphicsFamily, 0, &mGraphicsQueue);
    vkGetDeviceQueue(mDevice, computeFamily, 0, &mComputeQueue);
    vkGetDeviceQueue(mDevice, transferFamily, 0, &mTransferQueue);

    // create cmdbuf pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics command pool!");
    }

    // create descriptor pool
    uint32_t setCount                  = 65535;
    uint32_t sampledImageCount         = 32 * 65536;
    uint32_t storageImageCount         = 1  * 65536;
    uint32_t uniformBufferCount        = 1  * 65536;
    uint32_t uniformBufferDynamicCount = 4  * 65536;
    uint32_t storageBufferCount        = 1  * 65536;
    uint32_t uniformTexelBufferCount   = 8192;
    uint32_t storageTexelBufferCount   = 8192;
    uint32_t samplerCount              = 2  * 65536;

    VkDescriptorPoolSize poolSizes[8];

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    poolSizes[0].descriptorCount = sampledImageCount;

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[1].descriptorCount = storageImageCount;

    poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[2].descriptorCount = uniformBufferCount;

    poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[3].descriptorCount = uniformBufferDynamicCount;

    poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[4].descriptorCount = storageBufferCount;

    poolSizes[5].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    poolSizes[5].descriptorCount = samplerCount;

    poolSizes[6].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    poolSizes[6].descriptorCount = uniformTexelBufferCount;

    poolSizes[7].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    poolSizes[7].descriptorCount = storageTexelBufferCount;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pNext = nullptr;
    descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // Allocated descriptor sets will release their allocations back to the pool
    descriptorPoolCreateInfo.maxSets = setCount;
    descriptorPoolCreateInfo.poolSizeCount = 8;
    descriptorPoolCreateInfo.pPoolSizes = poolSizes;

    vkCreateDescriptorPool(mDevice, &descriptorPoolCreateInfo, nullptr, &mDescriptorPool);

    // init window
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    mWindow = glfwCreateWindow(mWidth, mHeight, "Star", nullptr, nullptr);

    if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
    glfwSetWindowUserPointer(mWindow, this);
    glfwSetFramebufferSizeCallback(mWindow, resizeCallback);
    glfwSetMouseButtonCallback(mWindow, mouseButtonCallback);
    glfwSetScrollCallback(mWindow, mouseScrollCallback);

    // get swap chain support
    VkBool32 supportsPresent;
    vkGetPhysicalDeviceSurfaceSupportKHR(mGPU, graphicsFamily, mSurface, &supportsPresent);
    if (!supportsPresent)
    {
        throw std::runtime_error("cannot find a graphics queue that also supports present operations.");
    }

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mGPU, mSurface, &mSwapChainSupport.capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(mGPU, mSurface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        mSwapChainSupport.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(mGPU, mSurface, &formatCount, mSwapChainSupport.formats.data());
    }
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(mGPU, mSurface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        mSwapChainSupport.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(mGPU, mSurface, &presentModeCount, mSwapChainSupport.presentModes.data());
    }
    mSwapChainSupport.surfaceFormat = chooseSwapSurfaceFormat(mSwapChainSupport.formats);
    mSwapChainSupport.presentMode = chooseSwapPresentMode(mSwapChainSupport.presentModes);

    // create display render pass
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = mSwapChainSupport.surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mDisplayRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }

    // create swap chain
    recreateSwapchain(mWidth, mHeight);

    // create
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphore) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create semaphores!");
    }
    if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphore) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create semaphores!");
    }
}

Renderer::~Renderer()
{
    vkDestroySemaphore(mDevice, mImageAvailableSemaphore, nullptr);
    vkDestroySemaphore(mDevice, mRenderFinishedSemaphore, nullptr);
    for (auto imageView : mSwapChainImageViews)
    {
        vkDestroyImageView(mDevice, imageView, nullptr);
    }
    mSwapChainImageViews.clear();
    for (auto framebuffer : mSwapChainFramebuffers)
    {
        vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
    }
    mSwapChainFramebuffers.clear();
    vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
    vkDestroyRenderPass(mDevice, mDisplayRenderPass, nullptr);

    vkDestroyPipeline(mDevice, mDisplayPipeline, nullptr);
    vkDestroyPipelineLayout(mDevice, mDisplayPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(mDevice, mDisplayDescSetLayout, nullptr);

    vkDestroyPipeline(mDevice, mAccumulatePipeline, nullptr);
    vkDestroyPipelineLayout(mDevice, mAccumulatePipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(mDevice, mAccumulateDescSetLayout, nullptr);

    vkDestroyPipeline(mDevice, mTracePipeline, nullptr);
    vkDestroyPipelineLayout(mDevice, mTracePipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(mDevice, mTraceDescSetLayout, nullptr);

    destroyBuffer(mQuadVertexBuffer);
    destroyBuffer(mQuadIndexBuffer);
    destroyBuffer(mTargetBuffer);
    destroyBuffer(mSceneSetingBuffer);
    destroyBuffer(mSceneBVHNodeBuffer);
    destroyBuffer(mLightBVHNodeBuffer);
    destroyBuffer(mTriangleBuffer);
    destroyBuffer(mMaterialBuffer);
    destroyTexture(mRenderTargetTexture);
    for (int i = 0; i < mCommandBuffers.size(); ++i)
    {
        destroyCommandBuffer(mCommandBuffers[i]);
    }
    mCommandBuffers.clear();
    vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
    vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
    vkDestroyDevice(mDevice, nullptr);
#if ENABLE_VALIDATION_LAYERS
    auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(mInstance, "vkDestroyDebugReportCallbackEXT");
    vkDestroyDebugReportCallbackEXT(mInstance, mDebugCallback, nullptr);
#endif
    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    vkDestroyInstance(mInstance, nullptr);

    glfwDestroyWindow(mWindow);
    glfwTerminate();

    if(gCamera)
        delete gCamera;
}

void Renderer::resize(int width, int height)
{
    mWidth = width;
    mHeight = height;

    // clear old swap chain
    for (auto imageView : mSwapChainImageViews)
    {
        vkDestroyImageView(mDevice, imageView, nullptr);
    }
    mSwapChainImageViews.clear();
    for (auto framebuffer : mSwapChainFramebuffers)
    {
        vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
    }
    mSwapChainFramebuffers.clear();
    vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);

    recreateSwapchain(mWidth, mHeight);
}

void Renderer::recreateSwapchain(int width, int height)
{
    VkExtent2D swapChainExtent;
    swapChainExtent.width = mWidth;
    swapChainExtent.height = mHeight;
    VkSurfaceFormatKHR surfaceFormat = mSwapChainSupport.surfaceFormat;
    VkPresentModeKHR presentMode = mSwapChainSupport.presentMode;

    uint32_t imageCount = mSwapChainSupport.capabilities.minImageCount + 1;
    if (mSwapChainSupport.capabilities.maxImageCount > 0 && imageCount > mSwapChainSupport.capabilities.maxImageCount)
    {
        imageCount = mSwapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = mSurface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = swapChainExtent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 0;
    swapchainInfo.pQueueFamilyIndices = nullptr;
    swapchainInfo.preTransform = mSwapChainSupport.capabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(mDevice, &swapchainInfo, nullptr, &mSwapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    // get swap chain images
    std::vector<VkImage> swapChainImages;
    vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, swapChainImages.data());

    // create image views
    mSwapChainImageViews.clear();
    for (uint32_t i = 0; i < swapChainImages.size(); i++)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = surfaceFormat.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(mDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image view!");
        }
        mSwapChainImageViews.push_back(imageView);
    }

    // create frame buffer
    mSwapChainFramebuffers.clear();
    for (uint32_t i = 0; i < mSwapChainImageViews.size(); i++)
    {
        VkImageView attachments[] = {
                mSwapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = mDisplayRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        VkFramebuffer framebuffer;
        if (vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
        mSwapChainFramebuffers.push_back(framebuffer);
    }
}

void Renderer::initRenderer(BVH* sceneBVH, std::vector<std::shared_ptr<Material>>& mats)
{
    // create camera
    gCamera = new Camera(glm::vec3(0.0f));

    // load buffer
    mQuadVertexBuffer = createBuffer(20 * sizeof(float),
                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    writeData(mQuadVertexBuffer, 0, 20 * sizeof(float), quadVertices);

    mQuadIndexBuffer = createBuffer(6 * sizeof(unsigned int),
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    writeData(mQuadIndexBuffer, 0, 6 * sizeof(unsigned int), quadIndices);

    mTargetBuffer = createBuffer(mWidth * mHeight * 3 * sizeof(float),
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    mSceneSetingBuffer = createBuffer(sizeof(GPUSceneSeting),
                                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    std::vector<GPUBVHNode> sceneBVHNodes;
    for (int i = 0; i < sceneBVH->getNodeCount(); ++i)
    {
        LinearBVHNode* nodes = sceneBVH->getNodes();
        GPUBVHNode node;
        node.axis = nodes[i].axis;
        node.numPrimitive = nodes[i].numPrimitive;
        node.primitiveOffset = nodes[i].primitiveOffset;
        node.secondChildOffset = nodes[i].secondChildOffset;
        node.bboxMin = glm::vec3(nodes[i].bound.mMin.x, nodes[i].bound.mMin.y, nodes[i].bound.mMin.z);
        node.bboxMax = glm::vec3(nodes[i].bound.mMax.x, nodes[i].bound.mMax.y, nodes[i].bound.mMax.z);
        sceneBVHNodes.push_back(node);
    }
    mSceneBVHNodeBuffer = createBuffer(sceneBVHNodes.size() * sizeof(GPUBVHNode),
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    writeData(mSceneBVHNodeBuffer, 0, sceneBVHNodes.size() * sizeof(GPUBVHNode), sceneBVHNodes.data());

    std::vector<GPUTriangle> triangles;
    std::vector<std::shared_ptr<Triangle>> prims = sceneBVH->getPrims();
    for (int i = 0; i < prims.size(); i++)
    {
        glm::vec3 p0, p1, p2;
        glm::vec3 n0, n1, n2;
        glm::vec2 t0, t1, t2;
        prims[i]->getPositionData(p0, p1, p2);
        prims[i]->getNormalData(n0, n1, n2);
        prims[i]->getUVData(t0, t1, t2);

        GPUVertex v0, v1, v2;
        v0.position = p0;
        v0.normal = n0;
        v0.texcoord = glm::vec3(t0.x, t0.y, 0.0);

        v1.position = p1;
        v1.normal = n1;
        v1.texcoord = glm::vec3(t1.x, t1.y, 0.0);

        v2.position = p2;
        v2.normal = n2;
        v2.texcoord = glm::vec3(t2.x, t2.y, 0.0);

        GPUTriangle tri;
        tri.v0 = v0;
        tri.v1 = v1;
        tri.v2 = v2;
        tri.mat = prims[i]->getMaterialID();
        triangles.push_back(tri);
    }
    mTriangleBuffer = createBuffer(triangles.size() * sizeof(GPUTriangle),
                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    writeData(mTriangleBuffer, 0, triangles.size() * sizeof(GPUTriangle), triangles.data());

    std::vector<GPUMaterial> materials;
    for (int i = 0; i < mats.size(); ++i)
    {
        GPUMaterial mat;
        mat.baseColor = mats[i]->baseColor;
        mat.emissive = mats[i]->emissive;
        mat.metallic = mats[i]->metallic;
        mat.roughness = mats[i]->roughness;
        materials.push_back(mat);
    }
    mMaterialBuffer = createBuffer(materials.size() * sizeof(GPUMaterial),
                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    writeData(mMaterialBuffer, 0, materials.size() * sizeof(GPUMaterial), materials.data());

    // load texture
    mRenderTargetTexture = createTexture(mWidth, mHeight, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // create display pipeline
    {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = 0;
        layoutBinding.descriptorCount = 1;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.pImmutableSamplers = nullptr;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &layoutBinding;

        if (vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr, &mDisplayDescSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        mDisplayDescSets.resize(1);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = mDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &mDisplayDescSetLayout;
        if (vkAllocateDescriptorSets(mDevice, &allocInfo, mDisplayDescSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = mRenderTargetTexture->imageView;
        imageInfo.sampler = mRenderTargetTexture->sampler;

        std::vector<VkWriteDescriptorSet> descriptorWrites;
        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = mDisplayDescSets[0];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.pImageInfo = &imageInfo;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrites.push_back(descriptorWrite);

        vkUpdateDescriptorSets(mDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

        VkShaderModule vertShaderModule = createShader(0, "./Resources/Shaders/display.vert");
        VkShaderModule fragShaderModule = createShader(1, "./Resources/Shaders/display.frag");

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        shaderStages.clear();
        shaderStages.push_back(vertShaderStageInfo);
        shaderStages.push_back(fragShaderStageInfo);

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = 20;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attributeDescriptions[2];
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = 0;

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = 12;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = 2;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)mWidth;
        viewport.height = (float)mHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent.width = mWidth;
        scissor.extent.height = mHeight;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &mDisplayDescSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mDisplayPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = mDisplayPipelineLayout;
        pipelineInfo.renderPass = mDisplayRenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mDisplayPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(mDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(mDevice, vertShaderModule, nullptr);
    }

    // create compute pipeline
    {
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        {
            VkDescriptorSetLayoutBinding layoutBinding = {};
            layoutBinding.binding = 0;
            layoutBinding.descriptorCount = 1;
            layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            layoutBinding.pImmutableSamplers = nullptr;
            layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            layoutBindings.push_back(layoutBinding);
        }
        {
            VkDescriptorSetLayoutBinding layoutBinding = {};
            layoutBinding.binding = 1;
            layoutBinding.descriptorCount = 1;
            layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            layoutBinding.pImmutableSamplers = nullptr;
            layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            layoutBindings.push_back(layoutBinding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = layoutBindings.size();
        layoutInfo.pBindings = layoutBindings.data();

        if (vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr, &mAccumulateDescSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        mAccumulateDescSets.resize(1);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = mDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &mAccumulateDescSetLayout;
        if (vkAllocateDescriptorSets(mDevice, &allocInfo, mAccumulateDescSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = mTargetBuffer->buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = mWidth * mHeight * 3 * sizeof(float);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = mRenderTargetTexture->imageView;
        imageInfo.sampler = mRenderTargetTexture->sampler;

        std::vector<VkWriteDescriptorSet> descriptorWrites;
        {
            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = mAccumulateDescSets[0];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.pBufferInfo  = &bufferInfo;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrites.push_back(descriptorWrite);
        }
        {
            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = mAccumulateDescSets[0];
            descriptorWrite.dstBinding = 1;
            descriptorWrite.pImageInfo = &imageInfo;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            descriptorWrite.descriptorCount = 1;
            descriptorWrites.push_back(descriptorWrite);
        }

        vkUpdateDescriptorSets(mDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &mAccumulateDescSetLayout;
        vkCreatePipelineLayout(mDevice, &pipelineLayoutCreateInfo, nullptr, &mAccumulatePipelineLayout);

        VkShaderModule shaderModule = createShader(2, "./Resources/Shaders/accm.comp");
        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageInfo.module = shaderModule;
        shaderStageInfo.pName = "main";

        VkComputePipelineCreateInfo pipelineCreateInfo {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.layout = mAccumulatePipelineLayout;
        pipelineCreateInfo.flags = 0;
        pipelineCreateInfo.stage = shaderStageInfo;

        vkCreateComputePipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mAccumulatePipeline);
        vkDestroyShaderModule(mDevice, shaderModule, nullptr);
    }
    {
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        {
            VkDescriptorSetLayoutBinding layoutBinding = {};
            layoutBinding.binding = 0;
            layoutBinding.descriptorCount = 1;
            layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            layoutBinding.pImmutableSamplers = nullptr;
            layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            layoutBindings.push_back(layoutBinding);
        }
        {
            VkDescriptorSetLayoutBinding layoutBinding = {};
            layoutBinding.binding = 1;
            layoutBinding.descriptorCount = 1;
            layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            layoutBinding.pImmutableSamplers = nullptr;
            layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            layoutBindings.push_back(layoutBinding);
        }
        {
            VkDescriptorSetLayoutBinding layoutBinding = {};
            layoutBinding.binding = 2;
            layoutBinding.descriptorCount = 1;
            layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            layoutBinding.pImmutableSamplers = nullptr;
            layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            layoutBindings.push_back(layoutBinding);
        }
        {
            VkDescriptorSetLayoutBinding layoutBinding = {};
            layoutBinding.binding = 3;
            layoutBinding.descriptorCount = 1;
            layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            layoutBinding.pImmutableSamplers = nullptr;
            layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            layoutBindings.push_back(layoutBinding);
        }
        {
            VkDescriptorSetLayoutBinding layoutBinding = {};
            layoutBinding.binding = 4;
            layoutBinding.descriptorCount = 1;
            layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            layoutBinding.pImmutableSamplers = nullptr;
            layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            layoutBindings.push_back(layoutBinding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = layoutBindings.size();
        layoutInfo.pBindings = layoutBindings.data();

        if (vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr, &mTraceDescSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        mTraceDescSets.resize(1);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = mDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &mTraceDescSetLayout;
        if (vkAllocateDescriptorSets(mDevice, &allocInfo, mTraceDescSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites;
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = mSceneSetingBuffer->buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(GPUSceneSeting);

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = mTraceDescSets[0];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.pBufferInfo  = &bufferInfo;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrites.push_back(descriptorWrite);
        }
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = mTargetBuffer->buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = mWidth * mHeight * 3 * sizeof(float);

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = mTraceDescSets[0];
            descriptorWrite.dstBinding = 1;
            descriptorWrite.pBufferInfo  = &bufferInfo;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrites.push_back(descriptorWrite);
        }
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = mSceneBVHNodeBuffer->buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sceneBVHNodes.size() * sizeof(GPUBVHNode);

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = mTraceDescSets[0];
            descriptorWrite.dstBinding = 2;
            descriptorWrite.pBufferInfo  = &bufferInfo;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrites.push_back(descriptorWrite);
        }

        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = mTriangleBuffer->buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = triangles.size() * sizeof(GPUTriangle);

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = mTraceDescSets[0];
            descriptorWrite.dstBinding = 3;
            descriptorWrite.pBufferInfo  = &bufferInfo;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrites.push_back(descriptorWrite);
        }

        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = mMaterialBuffer->buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = materials.size() * sizeof(GPUMaterial);

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = mTraceDescSets[0];
            descriptorWrite.dstBinding = 4;
            descriptorWrite.pBufferInfo  = &bufferInfo;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrites.push_back(descriptorWrite);
        }

        vkUpdateDescriptorSets(mDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &mTraceDescSetLayout;
        vkCreatePipelineLayout(mDevice, &pipelineLayoutCreateInfo, nullptr, &mTracePipelineLayout);

        VkShaderModule shaderModule = createShader(2, "./Resources/Shaders/trace.comp");
        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageInfo.module = shaderModule;
        shaderStageInfo.pName = "main";

        VkComputePipelineCreateInfo pipelineCreateInfo {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.layout = mTracePipelineLayout;
        pipelineCreateInfo.flags = 0;
        pipelineCreateInfo.stage = shaderStageInfo;

        vkCreateComputePipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mTracePipeline);
        vkDestroyShaderModule(mDevice, shaderModule, nullptr);
    }
}

void Renderer::run()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        // update
        if (gMouseButtonDown[1])
        {
            gMouseLastPosition[0] = gMousePosition[0];
            gMouseLastPosition[1] = gMousePosition[1];
        }
        if (gMouseButtonHeld[1])
        {
            gCamera->rotate(glm::vec2(gMousePosition[0] - gMouseLastPosition[0], gMousePosition[1] - gMouseLastPosition[1]));
            gMouseLastPosition[0] = gMousePosition[0];
            gMouseLastPosition[1] = gMousePosition[1];
        }
        gCamera->move(gMouseScrollWheel * 5.0);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(mDevice, mSwapChain, UINT64_MAX, mImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        // fill cmd buf
        CommandBuffer* displayCmdBuf = getActiveCommandBuffer();
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            if (vkBeginCommandBuffer(displayCmdBuf->commandBuffer, &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }
            // trace
            vkCmdBindPipeline(displayCmdBuf->commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mTracePipeline);
            vkCmdBindDescriptorSets(displayCmdBuf->commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mTracePipelineLayout, 0, mTraceDescSets.size(), mTraceDescSets.data(), 0, 0);
            vkCmdDispatch(displayCmdBuf->commandBuffer, mWidth / 16, mHeight / 16, 1);

            // accm
            vkCmdBindPipeline(displayCmdBuf->commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mAccumulatePipeline);
            vkCmdBindDescriptorSets(displayCmdBuf->commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mAccumulatePipelineLayout, 0, mAccumulateDescSets.size(), mAccumulateDescSets.data(), 0, 0);
            vkCmdDispatch(displayCmdBuf->commandBuffer, mWidth / 16, mHeight / 16, 1);

            // display
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = mDisplayRenderPass;
            renderPassInfo.framebuffer = mSwapChainFramebuffers[imageIndex];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent.width = mWidth;
            renderPassInfo.renderArea.extent.height = mHeight;

            VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(displayCmdBuf->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(displayCmdBuf->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mDisplayPipeline);

            VkBuffer vertexBuffers[] = {mQuadVertexBuffer->buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(displayCmdBuf->commandBuffer, 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(displayCmdBuf->commandBuffer, mQuadIndexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(displayCmdBuf->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mDisplayPipelineLayout, 0, 1, mDisplayDescSets.data(), 0, nullptr);
            vkCmdDrawIndexed(displayCmdBuf->commandBuffer, 6, 1, 0, 0, 0);

            vkCmdEndRenderPass(displayCmdBuf->commandBuffer);

            if (vkEndCommandBuffer(displayCmdBuf->commandBuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }

        {
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &mImageAvailableSemaphore;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &displayCmdBuf->commandBuffer;
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &mRenderFinishedSemaphore;

            vkResetFences(mDevice, 1, &displayCmdBuf->fence);
            if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, displayCmdBuf->fence) != VK_SUCCESS) {
                throw std::runtime_error("failed to submit draw command buffer!");
            }
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &mRenderFinishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &mSwapChain;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(mGraphicsQueue, &presentInfo);
        vkQueueWaitIdle(mGraphicsQueue);

        memset(gMouseButtonDown, 0, sizeof(gMouseButtonDown));
        memset(gMouseButtonUp, 0, sizeof(gMouseButtonUp));
        gMouseScrollWheel = 0;
        glfwPollEvents();
    }
}

VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkShaderModule Renderer::createShader(int type, const std::string& filePath, const std::string& incDir)
{
    std::string src;
    readFileData(filePath, src);
    ShaderDesc shaderDesc;
    shaderDesc.type = type;
    shaderDesc.source = src;
    shaderDesc.includeDir = incDir;
    std::vector<unsigned int> byteCode = compileShader(shaderDesc);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = byteCode.size() * sizeof(unsigned int);
    createInfo.pCode = reinterpret_cast<const unsigned int*>(byteCode.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(mDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(mGPU, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

Buffer* Renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    Buffer* buffer = new Buffer();
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(mDevice, &bufferInfo, nullptr, &buffer->buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(mDevice, buffer->buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &buffer->memory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(mDevice, buffer->buffer, buffer->memory, 0);
    return buffer;
}

void Renderer::destroyBuffer(Buffer *buffer)
{
    if(buffer)
    {
        vkDestroyBuffer(mDevice, buffer->buffer, nullptr);
        vkFreeMemory(mDevice, buffer->memory, nullptr);
        delete buffer;
    }
}

void Renderer::readData(Buffer* buffer, uint32_t offset, uint32_t size, void * dest)
{
    void* data;
    vkMapMemory(mDevice, buffer->memory, offset, size, 0, &data);
    memcpy(dest, data, static_cast<size_t>(size));
    vkUnmapMemory(mDevice, buffer->memory);
}

void Renderer::writeData(Buffer* buffer, uint32_t offset, uint32_t size, void * source)
{
    void *data;
    vkMapMemory(mDevice, buffer->memory, offset, size, 0, &data);
    memcpy(data, source, static_cast<size_t>(size));
    vkUnmapMemory(mDevice, buffer->memory);
}

Texture* Renderer::createTexture(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
    Texture* texture = new Texture();
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(mDevice, &imageInfo, nullptr, &texture->image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(mDevice, texture->image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &texture->memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(mDevice, texture->image, texture->memory, 0);

    // change image layout
    texture->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    transitionImageLayout(texture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(mDevice, &samplerInfo, nullptr, &texture->sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    // create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(mDevice, &viewInfo, nullptr, &texture->imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return texture;
}

void Renderer::destroyTexture(Texture* texture)
{
    if(texture)
    {
        vkDestroyImage(mDevice, texture->image, nullptr);
        vkDestroyImageView(mDevice, texture->imageView, nullptr);
        vkDestroySampler(mDevice, texture->sampler, nullptr);
        vkFreeMemory(mDevice, texture->memory, nullptr);
        delete texture;
    }
}

void Renderer::transitionImageLayout(Texture* texture, VkImageLayout oldLayout, VkImageLayout newLayout,
                           VkPipelineStageFlags srcStageMask,
                           VkPipelineStageFlags dstStageMask)
{
    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = texture->image;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;

    switch (oldLayout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            imageMemoryBarrier.srcAccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            break;
    }

    switch (newLayout)
    {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            if (imageMemoryBarrier.srcAccessMask == 0)
            {
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            break;
    }
    CommandBuffer* tempCmdBuffer = getActiveCommandBuffer();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(tempCmdBuffer->commandBuffer, &beginInfo);
    vkCmdPipelineBarrier(
            tempCmdBuffer->commandBuffer,
            srcStageMask,
            dstStageMask,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);
    vkEndCommandBuffer(tempCmdBuffer->commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &tempCmdBuffer->commandBuffer;

    vkResetFences(mDevice, 1, &tempCmdBuffer->fence);
    if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, tempCmdBuffer->fence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    vkWaitForFences(mDevice, 1, &tempCmdBuffer->fence, VK_TRUE, UINT64_MAX);
}

CommandBuffer* Renderer::getActiveCommandBuffer()
{
    CommandBuffer* cmdBuffer = nullptr;
    for (int i = 0; i < mCommandBuffers.size(); ++i)
    {
        if(checkCommandBufferState(mCommandBuffers[i]))
        {
            return mCommandBuffers[i];
        }
    }
    cmdBuffer = new CommandBuffer();
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(mDevice, &allocInfo, &cmdBuffer->commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateFence(mDevice, &fenceInfo, nullptr, &cmdBuffer->fence) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create fence!");
    }
    mCommandBuffers.push_back(cmdBuffer);
    return cmdBuffer;
}

bool Renderer::checkCommandBufferState(CommandBuffer* commandBuffer)
{
    if(vkGetFenceStatus(mDevice, commandBuffer->fence) == VK_SUCCESS)
    {
        vkResetCommandBuffer(commandBuffer->commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        return true;
    }
    return false;
}

void Renderer::destroyCommandBuffer(CommandBuffer* commandBuffer)
{
    if(commandBuffer)
    {
        vkDestroyFence(mDevice, commandBuffer->fence, nullptr);
        vkFreeCommandBuffers(mDevice, mCommandPool, 1, &commandBuffer->commandBuffer);
        delete commandBuffer;
    }
}

void resizeCallback(GLFWwindow * window, int width, int height)
{
    Renderer* renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->resize(width, height);
}

void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods)
{
    switch (action)
    {
        case GLFW_PRESS:
            gMouseButtonDown[button] = true;
            gMouseButtonHeld[button] = true;
            break;
        case GLFW_RELEASE:
            gMouseButtonUp[button] = true;
            gMouseButtonHeld[button] = false;
            break;
        default:
            break;
    }
}

void mouseScrollCallback(GLFWwindow * window, double xOffset, double yOffset)
{
    gMouseScrollWheel = yOffset;
}

void cursorPosCallback(GLFWwindow* window, double xPos, double yPos)
{
    gMousePosition[0] = xPos;
    gMousePosition[1] = yPos;
}