#ifndef STAR_RENDERER_H
#define STAR_RENDERER_H
#include <Application/Application.h>
#include "Accelerator/BBox.h"

class RHIDevice;
class RHISwapChain;
class RHISemaphore;
class RHIProgram;
class RHIBuffer;
class RHITexture;
class RHIGraphicsPipeline;
class RHIComputePipeline;
class RHIDescriptorSet;
class RHISampler;

namespace star {
    class Scene;

    struct GlobalSetting
    {
        alignas(16) glm::vec3 cameraPosition;
        alignas(16) glm::vec3 cameraRight;
        alignas(16) glm::vec3 cameraUp;
        alignas(16) glm::vec3 cameraFront;
        alignas(16) glm::vec4 cameraParam;
        alignas(16) glm::vec4 screenParam;
        alignas(4) int sceneBvhRootIndex;
    };

    struct Camera
    {
        glm::vec3 position;
        glm::vec3 front;
        glm::vec3 up;
        glm::vec3 right;
        glm::vec2 lastMousePosition;
        float yaw;
        float pitch;
        float fov;
        float aperture;
        float focalDist;
    };

    class Renderer : public Application
    {
    public:
        Renderer(Scene* scene, uint32_t width, uint32_t height);
        ~Renderer();
        virtual void prepare();
        virtual void run();
        virtual void finish();
    protected:
        void updateGlobalSetting();
        int updateCamera();
    protected:
        uint32_t mWidth;
        uint32_t mHeight;
        Camera mCamera;
        Scene* mScene;
        RHIDevice* mDevice = nullptr;
        RHISwapChain* mSwapChain = nullptr;
        RHISemaphore* mImageAvailableSemaphore = nullptr;
        RHISemaphore* mRenderFinishedSemaphore = nullptr;
        RHIProgram* mDisplayVertexProgram = nullptr;
        RHIProgram* mDisplayFragmentProgram = nullptr;
        RHIProgram* mTraceProgram = nullptr;
        RHIProgram* mAccumProgram = nullptr;
        RHIGraphicsPipeline* mDisplayPipeline = nullptr;
        RHIComputePipeline* mTracePipeline = nullptr;
        RHIComputePipeline* mAccumPipeline = nullptr;
        RHIDescriptorSet* mDisplayDescSet = nullptr;
        RHIDescriptorSet* mTraceDescSet = nullptr;
        RHIDescriptorSet* mAccumDescSet = nullptr;
        RHIBuffer* mQuadVertexBuffer = nullptr;
        RHIBuffer* mQuadIndexBuffer = nullptr;
        RHIBuffer* mSettingBuffer = nullptr;
        RHIBuffer* mSceneBvhNodeBuffer = nullptr;
        RHIBuffer* mSceneTransformBuffer = nullptr;
        RHITexture* mTraceTexture = nullptr;
        RHITexture* mAccumTexture = nullptr;
        RHISampler* mDefaultSampler = nullptr;
    };
}

#endif
