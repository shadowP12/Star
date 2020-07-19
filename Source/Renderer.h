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

struct Buffer
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct Texture
{
    VkImage image = VK_NULL_HANDLE;
    VkImage imageView = VK_NULL_HANDLE;
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct CommandBuffer
{
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
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
    // texture
    Texture* createTexture(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    void destroyTexture(Texture* texture);
    void transitionImageLayout(Texture* texture, VkImageLayout oldLayout, VkImageLayout newLayout,
            VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    // command buffer
    CommandBuffer* getActiveCommandBuffer();
    void destroyCommandBuffer(CommandBuffer* commandBuffer);
    bool checkCommandBufferState(CommandBuffer* commandBuffer);
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
    VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
    VkCommandPool mCommandPool = VK_NULL_HANDLE;
    std::vector<CommandBuffer*> mCommandBuffers;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;
    std::vector<VkImageView> mSwapChainImageViews;
    std::vector<VkFramebuffer> mSwapChainFramebuffers;
    SwapChainSupportDetails mSwapChainSupport;
    VkRenderPass mDisplayRenderPass = VK_NULL_HANDLE;
    /*
     * display pipeline params
     */
    VkPipeline mDisplayPipeline = VK_NULL_HANDLE;
    VkPipelineLayout mDisplayPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout mDisplayDescSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> mDisplayDescSets;
    /*
     * accumulate pipeline params
     */
    VkPipeline mAccumulatePipeline = VK_NULL_HANDLE;
    VkPipelineLayout mAccumulatePipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout mAccumulateDescSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> mAccumulateDescSets;
    /*
     * trace pipeline params
     */
    VkPipeline mTracePipeline = VK_NULL_HANDLE;
    VkPipelineLayout mTracePipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout mTraceDescSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> mTraceDescSets;
    VkSemaphore mImageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore mRenderFinishedSemaphore = VK_NULL_HANDLE;
    Texture* mRenderTargetTexture = nullptr;
    Buffer* mQuadVertexBuffer = nullptr;
    Buffer* mQuadIndexBuffer = nullptr;
    Buffer* mTargetBuffer = nullptr;
    GLFWwindow* mWindow;
    int mWidth;
    int mHeight;
    int mSamplerCount;
};