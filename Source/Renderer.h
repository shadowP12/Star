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

    class Renderer : public Application
    {
    public:
        Renderer(Scene* scene, uint32_t width, uint32_t height);
        ~Renderer();
        virtual void prepare();
        virtual void run();
        virtual void finish();
    protected:
        uint32_t mWidth;
        uint32_t mHeight;
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
        RHIBuffer* mTraceBuffer = nullptr;
        RHITexture* mAccumTexture = nullptr;
        RHISampler* mDefaultSampler = nullptr;
    };
}

#endif
