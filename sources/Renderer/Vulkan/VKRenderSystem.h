/*
 * VKRenderSystem.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_VK_RENDER_SYSTEM_H
#define LLGL_VK_RENDER_SYSTEM_H


#include <LLGL/RenderSystem.h>
#include "VKPhysicalDevice.h"
#include "VKDevice.h"
#include "../ContainerTypes.h"
#include "Memory/VKDeviceMemoryManager.h"

#include "VKCommandQueue.h"
#include "VKCommandBuffer.h"
#include "VKSwapChain.h"

#include "Buffer/VKBuffer.h"
#include "Buffer/VKBufferArray.h"

#include "Shader/VKShader.h"

#include "Texture/VKTexture.h"
#include "Texture/VKSampler.h"
#include "Texture/VKRenderTarget.h"

#include "RenderState/VKQueryHeap.h"
#include "RenderState/VKFence.h"
#include "RenderState/VKRenderPass.h"
#include "RenderState/VKPipelineLayout.h"
#include "RenderState/VKGraphicsPSO.h"
#include "RenderState/VKResourceHeap.h"

#include <string>
#include <memory>
#include <vector>
#include <set>
#include <tuple>


namespace LLGL
{


class VKRenderSystem final : public RenderSystem
{

    public:

        /* ----- Common ----- */

        VKRenderSystem(const RenderSystemDescriptor& renderSystemDesc);
        ~VKRenderSystem();

        /* ----- Swap-chain ----- */

        SwapChain* CreateSwapChain(const SwapChainDescriptor& swapChainDesc, const std::shared_ptr<Surface>& surface = nullptr) override;

        void Release(SwapChain& swapChain) override;

        /* ----- Command queues ----- */

        CommandQueue* GetCommandQueue() override;

        /* ----- Command buffers ----- */

        CommandBuffer* CreateCommandBuffer(const CommandBufferDescriptor& commandBufferDesc = {}) override;

        void Release(CommandBuffer& commandBuffer) override;

        /* ----- Buffers ------ */

        Buffer* CreateBuffer(const BufferDescriptor& bufferDesc, const void* initialData = nullptr) override;
        BufferArray* CreateBufferArray(std::uint32_t numBuffers, Buffer* const * bufferArray) override;

        void Release(Buffer& buffer) override;
        void Release(BufferArray& bufferArray) override;

        void WriteBuffer(Buffer& buffer, std::uint64_t offset, const void* data, std::uint64_t dataSize) override;
        void ReadBuffer(Buffer& buffer, std::uint64_t offset, void* data, std::uint64_t dataSize) override;

        void* MapBuffer(Buffer& buffer, const CPUAccess access) override;
        void* MapBuffer(Buffer& buffer, const CPUAccess access, std::uint64_t offset, std::uint64_t length) override;
        void UnmapBuffer(Buffer& buffer) override;

        /* ----- Textures ----- */

        Texture* CreateTexture(const TextureDescriptor& textureDesc, const SrcImageDescriptor* imageDesc = nullptr) override;

        void Release(Texture& texture) override;

        void WriteTexture(Texture& texture, const TextureRegion& textureRegion, const SrcImageDescriptor& imageDesc) override;
        void ReadTexture(Texture& texture, const TextureRegion& textureRegion, const DstImageDescriptor& imageDesc) override;

        /* ----- Sampler States ---- */

        Sampler* CreateSampler(const SamplerDescriptor& samplerDesc) override;

        void Release(Sampler& sampler) override;

        /* ----- Resource Heaps ----- */

        ResourceHeap* CreateResourceHeap(const ResourceHeapDescriptor& resourceHeapDesc, const ArrayView<ResourceViewDescriptor>& initialResourceViews = {}) override;

        void Release(ResourceHeap& resourceHeap) override;

        std::uint32_t WriteResourceHeap(ResourceHeap& resourceHeap, std::uint32_t firstDescriptor, const ArrayView<ResourceViewDescriptor>& resourceViews) override;

        /* ----- Render Passes ----- */

        RenderPass* CreateRenderPass(const RenderPassDescriptor& renderPassDesc) override;

        void Release(RenderPass& renderPass) override;

        /* ----- Render Targets ----- */

        RenderTarget* CreateRenderTarget(const RenderTargetDescriptor& renderTargetDesc) override;

        void Release(RenderTarget& renderTarget) override;

        /* ----- Shader ----- */

        Shader* CreateShader(const ShaderDescriptor& shaderDesc) override;

        void Release(Shader& shader) override;

        /* ----- Pipeline Layouts ----- */

        PipelineLayout* CreatePipelineLayout(const PipelineLayoutDescriptor& pipelineLayoutDesc) override;

        void Release(PipelineLayout& pipelineLayout) override;

        /* ----- Pipeline States ----- */

        PipelineState* CreatePipelineState(const Blob& serializedCache) override;
        PipelineState* CreatePipelineState(const GraphicsPipelineDescriptor& pipelineStateDesc, std::unique_ptr<Blob>* serializedCache = nullptr) override;
        PipelineState* CreatePipelineState(const ComputePipelineDescriptor& pipelineStateDesc, std::unique_ptr<Blob>* serializedCache = nullptr) override;

        void Release(PipelineState& pipelineState) override;

        /* ----- Queries ----- */

        QueryHeap* CreateQueryHeap(const QueryHeapDescriptor& queryHeapDesc) override;

        void Release(QueryHeap& queryHeap) override;

        /* ----- Fences ----- */

        Fence* CreateFence() override;

        void Release(Fence& fence) override;

    private:

        void CreateInstance(const RendererConfigurationVulkan* config);
        void CreateDebugReportCallback();
        void PickPhysicalDevice();
        void CreateLogicalDevice();

        bool IsLayerRequired(const char* name, const RendererConfigurationVulkan* config) const;

        VKDeviceBuffer CreateStagingBuffer(const VkBufferCreateInfo& createInfo);

        VKDeviceBuffer CreateStagingBufferAndInitialize(
            const VkBufferCreateInfo&   createInfo,
            const void*                 data,
            VkDeviceSize                dataSize
        );

    private:

        /* ----- Common objects ----- */

        VKPtr<VkInstance>                       instance_;

        VKPhysicalDevice                        physicalDevice_;
        VKDevice                                device_;

        VKPtr<VkDebugReportCallbackEXT>         debugReportCallback_;

        bool                                    debugLayerEnabled_      = false;

        std::unique_ptr<VKDeviceMemoryManager>  deviceMemoryMngr_;

        VKGraphicsPipelineLimits                gfxPipelineLimits_;

        /* ----- Hardware object containers ----- */

        HWObjectContainer<VKSwapChain>          swapChains_;
        HWObjectInstance<VKCommandQueue>        commandQueue_;
        HWObjectContainer<VKCommandBuffer>      commandBuffers_;
        HWObjectContainer<VKBuffer>             buffers_;
        HWObjectContainer<VKBufferArray>        bufferArrays_;
        HWObjectContainer<VKTexture>            textures_;
        HWObjectContainer<VKSampler>            samplers_;
        HWObjectContainer<VKRenderPass>         renderPasses_;
        HWObjectContainer<VKRenderTarget>       renderTargets_;
        HWObjectContainer<VKShader>             shaders_;
        HWObjectContainer<VKPipelineLayout>     pipelineLayouts_;
        HWObjectContainer<VKPipelineState>      pipelineStates_;
        HWObjectContainer<VKResourceHeap>       resourceHeaps_;
        HWObjectContainer<VKQueryHeap>          queryHeaps_;
        HWObjectContainer<VKFence>              fences_;

};


} // /namespace LLGL


#endif



// ================================================================================
