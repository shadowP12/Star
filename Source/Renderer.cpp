#include "Renderer.h"
#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include "FileUtils.h"
#include "RenderUtils.h"

#define ENABLE_VALIDATION_LAYERS 1

static void resizeCallback(GLFWwindow *window, int width, int height);
static void cursorPosCallback(GLFWwindow * window, double xPos, double yPos);
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods);
static void mouseScrollCallback(GLFWwindow * window, double xOffset, double yOffset);

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
    glfwSetKeyCallback(mWindow, keyCallback);
    glfwSetMouseButtonCallback(mWindow, mouseButtonCallback);
    glfwSetCursorPosCallback(mWindow, cursorPosCallback);
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
}

Renderer::~Renderer()
{
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
    vkDestroyDevice(mDevice, nullptr);
#if ENABLE_VALIDATION_LAYERS
    auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(mInstance, "vkDestroyDebugReportCallbackEXT");
    vkDestroyDebugReportCallbackEXT(mInstance, mDebugCallback, nullptr);
#endif
    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    vkDestroyInstance(mInstance, nullptr);

    glfwDestroyWindow(mWindow);
    glfwTerminate();
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

void Renderer::initRenderer()
{
    // create display pipeline
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

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

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
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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
    pipelineLayoutInfo.setLayoutCount = 0;
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

void Renderer::run()
{
    while (!glfwWindowShouldClose(mWindow))
    {
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

void resizeCallback(GLFWwindow * window, int width, int height)
{
    Renderer* renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->resize(width, height);
}

void cursorPosCallback(GLFWwindow * window, double xPos, double yPos)
{
}

void keyCallback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    switch (action)
    {
        case GLFW_PRESS:
            break;
        case GLFW_RELEASE:
            break;
        default:
            break;
    }
}

void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods)
{
    switch (action)
    {
        case GLFW_PRESS:
            break;
        case GLFW_RELEASE:
            break;
        default:
            break;
    }
}

void mouseScrollCallback(GLFWwindow * window, double xOffset, double yOffset)
{
}