#include "Renderer.h"
#include "Scene.h"
#include <RHI/RHIDevice.h>
#include <RHI/RHISwapChain.h>
#include <RHI/RHICommandBuffer.h>
#include <RHI/RHISynchronization.h>
#include <RHI/RHIPipeline.h>
#include <RHI/RHIBuffer.h>
#include <RHI/RHIQueue.h>
#include <RHI/RHITexture.h>
#include <RHI/RHIProgram.h>
#include <RHI/RHIFramebuffer.h>
#include <RHI/RHIDescriptorSet.h>
#include <Utility/FileSystem.h>
#include <RHI/Managers/SpirvManager.h>

struct QuadVertex {
    glm::vec3 pos;
    glm::vec2 uv;
};

struct TempBvhNode {
    alignas(16) glm::vec3 test1;
    alignas(16) glm::vec3 test2;
};

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

namespace star {
    Renderer::Renderer(Scene* scene, uint32_t width, uint32_t height)
            :Application(width, height)
    {
        mScene = scene;
        mWidth = width;
        mHeight = height;
        SpirvManager::startUp();
    }

    Renderer::~Renderer()
    {
        SpirvManager::shutDown();
    }

    void Renderer::prepare()
    {
        mCamera.position = glm::vec3(0.0f, 0.0f, 0.0f);
        mCamera.right = glm::vec3(1.0f, 0.0f, 0.0f);
        mCamera.up = glm::vec3(0.0f, 1.0f, 0.0f);
        mCamera.front = glm::vec3(0.0f, 0.0f, -1.0f);
        mCamera.yaw = -90.0f;
        mCamera.pitch = 0.0f;
        mCamera.fov = 60.0f;
        mCamera.focalDist = 0.1f;
        mCamera.aperture = 0.0f;

        mDevice = new RHIDevice();

        RHISwapChainInfo swapChainInfo;
        swapChainInfo.width = mWindow->getWidth();
        swapChainInfo.height = mWindow->getHeight();
        swapChainInfo.windowHandle = mWindow->getWindowPtr();
        mSwapChain = new RHISwapChain(mDevice, swapChainInfo);

        mImageAvailableSemaphore = new RHISemaphore(mDevice);
        mRenderFinishedSemaphore = new RHISemaphore(mDevice);

        RHICommandBuffer* cmdBuf = mDevice->getGraphicsCommandPool()->getActiveCmdBuffer();

        RHIProgramInfo programInfo;
        SpirvCompileResult compileResult;
        SpirvCompileInfo spirvCompileInfo;
        spirvCompileInfo.stageType = STAGE_VERTEX;
        spirvCompileInfo.entryPoint = "main";
        FileSystem::readFile("./Resources/Shaders/display.vert", spirvCompileInfo.source);
        compileResult = SpirvManager::instance().compile(spirvCompileInfo);
        programInfo.type = PROGRAM_VERTEX;
        programInfo.bytes = compileResult.bytes;
        mDisplayVertexProgram = new RHIProgram(mDevice, programInfo);

        spirvCompileInfo.stageType = STAGE_FRAGMENT;
        spirvCompileInfo.entryPoint = "main";
        FileSystem::readFile("./Resources/Shaders/display.frag", spirvCompileInfo.source);
        compileResult = SpirvManager::instance().compile(spirvCompileInfo);
        programInfo.type = PROGRAM_FRAGMENT;
        programInfo.bytes = compileResult.bytes;
        mDisplayFragmentProgram = new RHIProgram(mDevice, programInfo);

        spirvCompileInfo.stageType = STAGE_COMPUTE;
        spirvCompileInfo.entryPoint = "main";
        FileSystem::readFile("./Resources/Shaders/accum.comp", spirvCompileInfo.source);
        compileResult = SpirvManager::instance().compile(spirvCompileInfo);
        programInfo.type = PROGRAM_COMPUTE;
        programInfo.bytes = compileResult.bytes;
        mAccumProgram = new RHIProgram(mDevice, programInfo);

        spirvCompileInfo.stageType = STAGE_COMPUTE;
        spirvCompileInfo.entryPoint = "main";
        FileSystem::readFile("./Resources/Shaders/trace.comp", spirvCompileInfo.source);
        compileResult = SpirvManager::instance().compile(spirvCompileInfo);
        programInfo.type = PROGRAM_COMPUTE;
        programInfo.bytes = compileResult.bytes;
        mTraceProgram = new RHIProgram(mDevice, programInfo);

        RHIBufferInfo bufferInfo;
        bufferInfo.size = sizeof(Vertex) * 4;
        bufferInfo.descriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        bufferInfo.memoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        mQuadVertexBuffer = new RHIBuffer(mDevice, bufferInfo);
        mQuadVertexBuffer->writeData(0,sizeof(quadVertices), quadVertices);

        bufferInfo.size = sizeof(unsigned int) * 6;
        bufferInfo.descriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
        bufferInfo.memoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        mQuadIndexBuffer = new RHIBuffer(mDevice, bufferInfo);
        mQuadIndexBuffer->writeData(0, sizeof(quadIndices), quadIndices);

        bufferInfo.size = sizeof(GlobalSetting);
        bufferInfo.descriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bufferInfo.memoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        mSettingBuffer = new RHIBuffer(mDevice, bufferInfo);

        bufferInfo.size = sizeof(TempBvhNode) * 2;
        bufferInfo.descriptors = DESCRIPTOR_TYPE_RW_BUFFER;
        bufferInfo.memoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        mSceneBvhNodeBuffer = new RHIBuffer(mDevice, bufferInfo);
        std::vector<TempBvhNode> tempNodes;
        tempNodes.push_back({glm::vec3(1, 0, 0), glm::vec3(0, 1, 0)});
        tempNodes.push_back({glm::vec3(0, 1, 0), glm::vec3(1, 1, 1)});
        mSceneBvhNodeBuffer->writeData(0, sizeof(TempBvhNode) * 2, tempNodes.data());

        RHITextureInfo textureInfo;
        textureInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        textureInfo.descriptors = DESCRIPTOR_TYPE_TEXTURE | DESCRIPTOR_TYPE_RW_TEXTURE;
        textureInfo.width = mWidth;
        textureInfo.height = mHeight;
        textureInfo.depth = 1;
        textureInfo.mipLevels = 1;
        textureInfo.arrayLayers = 1;
        mAccumTexture = new RHITexture(mDevice, textureInfo);

        textureInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        textureInfo.descriptors = DESCRIPTOR_TYPE_RW_TEXTURE;
        textureInfo.width = mWidth;
        textureInfo.height = mHeight;
        textureInfo.depth = 1;
        textureInfo.mipLevels = 1;
        textureInfo.arrayLayers = 1;
        mTraceTexture = new RHITexture(mDevice, textureInfo);

        cmdBuf->begin();
        RHITextureBarrier barriers[] = { { mAccumTexture, RESOURCE_STATE_COMMON },
                                       { mTraceTexture, RESOURCE_STATE_COMMON } };
        cmdBuf->setResourceBarrier(0, nullptr, 2, barriers);
        cmdBuf->end();
        RHIQueueSubmitInfo submitInfo;
        submitInfo.cmdBuf = cmdBuf;
        mDevice->getGraphicsQueue()->submit(submitInfo);
        mDevice->getGraphicsQueue()->waitIdle();

        RHISamplerInfo samplerInfo;
        mDefaultSampler = new RHISampler(mDevice, samplerInfo);

        RHIDescriptorSetInfo descriptorSetInfo;
        descriptorSetInfo.set = 0;
        descriptorSetInfo.bindingCount = 1;
        descriptorSetInfo.bindings[0].binding = 0;
        descriptorSetInfo.bindings[0].descriptorCount = 1;
        descriptorSetInfo.bindings[0].type = DESCRIPTOR_TYPE_TEXTURE;
        descriptorSetInfo.bindings[0].stage = PROGRAM_FRAGMENT;
        mDisplayDescSet = new RHIDescriptorSet(mDevice, descriptorSetInfo);
        mDisplayDescSet->updateTexture(0, DESCRIPTOR_TYPE_TEXTURE, mAccumTexture, mDefaultSampler);

        descriptorSetInfo.set = 0;
        descriptorSetInfo.bindingCount = 2;
        descriptorSetInfo.bindings[0].binding = 0;
        descriptorSetInfo.bindings[0].descriptorCount = 1;
        descriptorSetInfo.bindings[0].type = DESCRIPTOR_TYPE_RW_TEXTURE;
        descriptorSetInfo.bindings[0].stage = PROGRAM_COMPUTE;
        descriptorSetInfo.bindings[1].binding = 1;
        descriptorSetInfo.bindings[1].descriptorCount = 1;
        descriptorSetInfo.bindings[1].type = DESCRIPTOR_TYPE_RW_TEXTURE;
        descriptorSetInfo.bindings[1].stage = PROGRAM_COMPUTE;
        mAccumDescSet = new RHIDescriptorSet(mDevice, descriptorSetInfo);
        mAccumDescSet->updateTexture(0, DESCRIPTOR_TYPE_RW_TEXTURE, mTraceTexture);
        mAccumDescSet->updateTexture(1, DESCRIPTOR_TYPE_RW_TEXTURE, mAccumTexture);

        descriptorSetInfo.set = 0;
        descriptorSetInfo.bindingCount = 3;
        descriptorSetInfo.bindings[0].binding = 0;
        descriptorSetInfo.bindings[0].descriptorCount = 1;
        descriptorSetInfo.bindings[0].type = DESCRIPTOR_TYPE_RW_TEXTURE;
        descriptorSetInfo.bindings[0].stage = PROGRAM_COMPUTE;
        descriptorSetInfo.bindings[1].binding = 1;
        descriptorSetInfo.bindings[1].descriptorCount = 1;
        descriptorSetInfo.bindings[1].type = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorSetInfo.bindings[1].stage = PROGRAM_COMPUTE;
        descriptorSetInfo.bindings[2].binding = 2;
        descriptorSetInfo.bindings[2].descriptorCount = 1;
        descriptorSetInfo.bindings[2].type = DESCRIPTOR_TYPE_RW_BUFFER;
        descriptorSetInfo.bindings[2].stage = PROGRAM_COMPUTE;
        mTraceDescSet = new RHIDescriptorSet(mDevice, descriptorSetInfo);
        mTraceDescSet->updateTexture(0, DESCRIPTOR_TYPE_RW_TEXTURE, mTraceTexture);
        mTraceDescSet->updateBuffer(1, DESCRIPTOR_TYPE_UNIFORM_BUFFER, mSettingBuffer, sizeof(GlobalSetting), 0);
        mTraceDescSet->updateBuffer(2, DESCRIPTOR_TYPE_RW_BUFFER, mSceneBvhNodeBuffer, sizeof(TempBvhNode) * 2, 0);

        VertexLayout vertexLayout;
        vertexLayout.attribCount = 2;
        vertexLayout.attribs[0].location = 0;
        vertexLayout.attribs[0].binding = 0;
        vertexLayout.attribs[0].offset = offsetof(QuadVertex, pos);
        vertexLayout.attribs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexLayout.attribs[0].rate = VERTEX_ATTRIB_RATE_VERTEX;
        vertexLayout.attribs[0].semantic = SEMANTIC_POSITION;

        vertexLayout.attribs[1].location = 1;
        vertexLayout.attribs[1].binding = 0;
        vertexLayout.attribs[1].offset = offsetof(QuadVertex, uv);
        vertexLayout.attribs[1].format = VK_FORMAT_R32G32_SFLOAT;
        vertexLayout.attribs[1].rate = VERTEX_ATTRIB_RATE_VERTEX;
        vertexLayout.attribs[1].semantic = SEMANTIC_TEXCOORD0;

        RHIGraphicsPipelineInfo pipelineInfo;
        pipelineInfo.renderPass = mSwapChain->getRenderPass();
        pipelineInfo.renderTargetCount = 1;
        pipelineInfo.descriptorSetCount = 1;
        pipelineInfo.descriptorSets = &mDisplayDescSet;
        pipelineInfo.vertexProgram = mDisplayVertexProgram;
        pipelineInfo.fragmentProgram = mDisplayFragmentProgram;
        pipelineInfo.vertexLayout = vertexLayout;
        mDisplayPipeline = new RHIGraphicsPipeline(mDevice, pipelineInfo);

        RHIComputePipelineInfo computePipelineInfo;
        computePipelineInfo.computeProgram = mAccumProgram;
        computePipelineInfo.descriptorSetCount = 1;
        computePipelineInfo.descriptorSets = &mAccumDescSet;
        mAccumPipeline = new RHIComputePipeline(mDevice, computePipelineInfo);

        computePipelineInfo.computeProgram = mTraceProgram;
        computePipelineInfo.descriptorSetCount = 1;
        computePipelineInfo.descriptorSets = &mTraceDescSet;
        mTracePipeline = new RHIComputePipeline(mDevice, computePipelineInfo);
    }

    void Renderer::run()
    {
        uint32_t imageIndex;
        mSwapChain->acquireNextImage(mImageAvailableSemaphore, nullptr, imageIndex);
        // update date
        int cameraDirtyFlag = updateCamera();
        updateGlobalSetting();

        RHICommandBuffer* cmdBuf = mDevice->getGraphicsCommandPool()->getActiveCmdBuffer();
        cmdBuf->begin();
        RHITexture* colorTarget = mSwapChain->getColorTexture(imageIndex);
        RHITexture* depthTarget = mSwapChain->getDepthStencilTexture(imageIndex);
        {
            RHITextureBarrier barriers[] = { { colorTarget, RESOURCE_STATE_RENDER_TARGET },
                                             { depthTarget, RESOURCE_STATE_DEPTH_WRITE } };
            cmdBuf->setResourceBarrier(0, nullptr, 2, barriers);
        }
        cmdBuf->bindComputePipeline(mTracePipeline, &mTraceDescSet, 1);
        cmdBuf->dispatch(mWidth / 16, mHeight / 16, 1);
        {
            RHITextureBarrier barriers[] = { { mTraceTexture, RESOURCE_STATE_UNORDERED_ACCESS } };
            cmdBuf->setResourceBarrier(0, nullptr, 1, barriers);
        }
        cmdBuf->bindComputePipeline(mAccumPipeline, &mAccumDescSet, 1);
        cmdBuf->dispatch(mWidth / 16, mHeight / 16, 1);
        {
            RHITextureBarrier barriers[] = { { mAccumTexture, RESOURCE_STATE_UNORDERED_ACCESS } };
            cmdBuf->setResourceBarrier(0, nullptr, 1, barriers);
        }
        cmdBuf->bindFramebuffer(mSwapChain->getFramebuffer(imageIndex));
        cmdBuf->bindGraphicsPipeline(mDisplayPipeline, &mDisplayDescSet, 1);
        cmdBuf->setViewport(0, 0, mWidth, mHeight);
        cmdBuf->setScissor(0, 0, mWidth, mHeight);
        cmdBuf->bindIndexBuffer(mQuadIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        cmdBuf->bindVertexBuffer(mQuadVertexBuffer, 0);
        cmdBuf->drawIndexed(6, 1, 0, 0, 0);
        cmdBuf->unbindFramebuffer();
        {
            RHITextureBarrier barriers[] = { { colorTarget, RESOURCE_STATE_PRESENT },
                                                  { depthTarget, RESOURCE_STATE_DEPTH_WRITE } };
            cmdBuf->setResourceBarrier(0, nullptr, 2, barriers);
        }
        cmdBuf->end();
        RHIQueueSubmitInfo submitInfo;
        submitInfo.cmdBuf = cmdBuf;
        submitInfo.waitSemaphores = &mImageAvailableSemaphore;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.signalSemaphores = &mRenderFinishedSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        mDevice->getGraphicsQueue()->submit(submitInfo);
        RHIQueuePresentInfo presentInfo;
        presentInfo.index = imageIndex;
        presentInfo.swapChain = mSwapChain;
        presentInfo.waitSemaphores = &mRenderFinishedSemaphore;
        presentInfo.waitSemaphoreCount = 1;
        mDevice->getGraphicsQueue()->Present(presentInfo);
    }

    void Renderer::finish()
    {
        SAFE_DELETE(mTraceTexture);
        SAFE_DELETE(mAccumTexture);
        SAFE_DELETE(mSceneBvhNodeBuffer);
        SAFE_DELETE(mSettingBuffer);
        SAFE_DELETE(mQuadVertexBuffer);
        SAFE_DELETE(mQuadIndexBuffer);
        SAFE_DELETE(mDefaultSampler);
        SAFE_DELETE(mTracePipeline);
        SAFE_DELETE(mAccumPipeline);
        SAFE_DELETE(mDisplayPipeline);
        SAFE_DELETE(mTraceDescSet);
        SAFE_DELETE(mAccumDescSet);
        SAFE_DELETE(mDisplayDescSet);
        SAFE_DELETE(mTraceProgram);
        SAFE_DELETE(mAccumProgram);
        SAFE_DELETE(mDisplayVertexProgram);
        SAFE_DELETE(mDisplayFragmentProgram);
        SAFE_DELETE(mImageAvailableSemaphore);
        SAFE_DELETE(mRenderFinishedSemaphore);
        SAFE_DELETE(mSwapChain);
        SAFE_DELETE(mDevice);
    }

    void Renderer::updateGlobalSetting()
    {
        GlobalSetting globalSetting;
        globalSetting.cameraPosition = mCamera.position;
        globalSetting.cameraRight = mCamera.right;
        globalSetting.cameraUp = mCamera.up;
        globalSetting.cameraFront = mCamera.front;
        globalSetting.cameraParam = glm::vec4(mCamera.fov, mCamera.focalDist, mCamera.aperture, 0.0f);
        globalSetting.screenParam = glm::vec4((float)mWidth, (float)mHeight, 0.0f, 0.0f);
        mSettingBuffer->writeData(0, sizeof(globalSetting), &globalSetting);
    }

    int Renderer::updateCamera()
    {
        int dirtyFlag = 0;
        glm::vec2 offset = Input::instance().getMousePosition() - mCamera.lastMousePosition;
        mCamera.lastMousePosition = Input::instance().getMousePosition();
        if (Input::instance().getMouseButton(MouseButton::MouseRight))
        {
            mCamera.yaw += offset.x * 0.1f;
            mCamera.pitch -= offset.y * 0.1f;
            glm::vec3 front;
            front.x = cos(glm::radians(mCamera.yaw)) * cos(glm::radians(mCamera.pitch));
            front.y = sin(glm::radians(mCamera.pitch));
            front.z = sin(glm::radians(mCamera.yaw)) * cos(glm::radians(mCamera.pitch));
            mCamera.front = glm::normalize(front);
            mCamera.right = glm::normalize(glm::cross(mCamera.front, glm::vec3(0, 1, 0)));
            mCamera.up = glm::normalize(glm::cross(mCamera.right, mCamera.front));
            dirtyFlag = 1;
        }
        if (Input::instance().getMouseScrollWheel() != 0)
        {
            float sw = Input::instance().getMouseScrollWheel();
            mCamera.position += mCamera.front * sw * 0.1f;
            dirtyFlag = 1;
        }

        return dirtyFlag;
    }
}
