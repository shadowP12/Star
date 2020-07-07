#pragma once
//#define VK_ENABLE_BETA_EXTENSIONS
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vk_sdk_platform.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
};

class Buffer
{
public:
    Buffer(){}
    ~Buffer(){}
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

class Renderer
{
public:
    Renderer(int width, int height);
    ~Renderer();
    void initRenderer();
    void run();
    void resize(int width, int height);
private:
    void recreateSwapchain(int width, int height);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkShaderModule createShader(int type, const std::string& filePath, const std::string& incDir = "");
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    // buffer
    Buffer* createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void destroyBuffer(Buffer* buffer);
    void readData(Buffer* buffer, uint32_t offset, uint32_t size, void * dest);
    void writeData(Buffer* buffer, uint32_t offset, uint32_t size, void * source);

private:
    VkInstance mInstance;
    VkDebugReportCallbackEXT mDebugCallback = VK_NULL_HANDLE;
    VkPhysicalDevice mGPU;
    VkPhysicalDeviceProperties mDeviceProperties;
    VkPhysicalDeviceFeatures mDeviceFeatures;
    VkPhysicalDeviceMemoryProperties mMemoryProperties;
    VkDevice mDevice;
    VkQueue mGraphicsQueue;
    VkQueue mComputeQueue;
    VkQueue mTransferQueue;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;
    std::vector<VkImageView> mSwapChainImageViews;
    std::vector<VkFramebuffer> mSwapChainFramebuffers;
    SwapChainSupportDetails mSwapChainSupport;
    VkRenderPass mDisplayRenderPass = VK_NULL_HANDLE;
    VkPipeline mDisplayPipeline = VK_NULL_HANDLE;
    VkPipelineLayout mDisplayPipelineLayout = VK_NULL_HANDLE;
    Buffer* mQuadVertexBuffer = nullptr;
    Buffer* mQuadIndexBuffer = nullptr;
    GLFWwindow* mWindow;
    int mWidth;
    int mHeight;
};