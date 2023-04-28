/*
 * VKCommandBuffer.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_VK_COMMAND_BUFFER_H
#define LLGL_VK_COMMAND_BUFFER_H


#include <LLGL/CommandBuffer.h>
#include "Vulkan.h"
#include "VKPtr.h"
#include "VKCore.h"
#include "RenderState/VKStagingDescriptorSetPool.h"
#include "RenderState/VKDescriptorCache.h"
#include <vector>


namespace LLGL
{


class VKDevice;
class VKPhysicalDevice;
class VKResourceHeap;
class VKRenderPass;
class VKQueryHeap;
class VKPipelineState;

class VKCommandBuffer final : public CommandBuffer
{

    public:

        /* ----- Common ----- */

        VKCommandBuffer(
            const VKPhysicalDevice&         physicalDevice,
            VKDevice&                       device,
            VkQueue                         commandQueue,
            const QueueFamilyIndices&       queueFamilyIndices,
            const CommandBufferDescriptor&  desc
        );
        ~VKCommandBuffer();

        /* ----- Encoding ----- */

        void Begin() override;
        void End() override;

        void Execute(CommandBuffer& deferredCommandBuffer) override;

        /* ----- Blitting ----- */

        void UpdateBuffer(
            Buffer&         dstBuffer,
            std::uint64_t   dstOffset,
            const void*     data,
            std::uint16_t   dataSize
        ) override;

        void CopyBuffer(
            Buffer&         dstBuffer,
            std::uint64_t   dstOffset,
            Buffer&         srcBuffer,
            std::uint64_t   srcOffset,
            std::uint64_t   size
        ) override;

        void CopyBufferFromTexture(
            Buffer&                 dstBuffer,
            std::uint64_t           dstOffset,
            Texture&                srcTexture,
            const TextureRegion&    srcRegion,
            std::uint32_t           rowStride   = 0,
            std::uint32_t           layerStride = 0
        ) override;

        void FillBuffer(
            Buffer&         dstBuffer,
            std::uint64_t   dstOffset,
            std::uint32_t   value,
            std::uint64_t   fillSize    = Constants::wholeSize
        ) override;

        void CopyTexture(
            Texture&                dstTexture,
            const TextureLocation&  dstLocation,
            Texture&                srcTexture,
            const TextureLocation&  srcLocation,
            const Extent3D&         extent
        ) override;

        void CopyTextureFromBuffer(
            Texture&                dstTexture,
            const TextureRegion&    dstRegion,
            Buffer&                 srcBuffer,
            std::uint64_t           srcOffset,
            std::uint32_t           rowStride   = 0,
            std::uint32_t           layerStride = 0
        ) override;

        void GenerateMips(Texture& texture) override;
        void GenerateMips(Texture& texture, const TextureSubresource& subresource) override;

        /* ----- Viewport and Scissor ----- */

        void SetViewport(const Viewport& viewport) override;
        void SetViewports(std::uint32_t numViewports, const Viewport* viewports) override;

        void SetScissor(const Scissor& scissor) override;
        void SetScissors(std::uint32_t numScissors, const Scissor* scissors) override;

        /* ----- Input Assembly ------ */

        void SetVertexBuffer(Buffer& buffer) override;
        void SetVertexBufferArray(BufferArray& bufferArray) override;

        void SetIndexBuffer(Buffer& buffer) override;
        void SetIndexBuffer(Buffer& buffer, const Format format, std::uint64_t offset = 0) override;

        /* ----- Resources ----- */

        void SetResourceHeap(ResourceHeap& resourceHeap, std::uint32_t descriptorSet = 0) override;

        void SetResource(std::uint32_t descriptor, Resource& resource) override;

        void ResetResourceSlots(
            const ResourceType  resourceType,
            std::uint32_t       firstSlot,
            std::uint32_t       numSlots,
            long                bindFlags,
            long                stageFlags      = StageFlags::AllStages
        ) override;

        /* ----- Render Passes ----- */

        void BeginRenderPass(
            RenderTarget&       renderTarget,
            const RenderPass*   renderPass      = nullptr,
            std::uint32_t       numClearValues  = 0,
            const ClearValue*   clearValues     = nullptr
        ) override;

        void EndRenderPass() override;

        void Clear(long flags, const ClearValue& clearValue = {}) override;
        void ClearAttachments(std::uint32_t numAttachments, const AttachmentClear* attachments) override;

        /* ----- Pipeline States ----- */

        void SetPipelineState(PipelineState& pipelineState) override;
        void SetBlendFactor(const float color[4]) override;
        void SetStencilReference(std::uint32_t reference, const StencilFace stencilFace = StencilFace::FrontAndBack) override;
        void SetUniforms(std::uint32_t first, const void* data, std::uint16_t dataSize) override;

        /* ----- Queries ----- */

        void BeginQuery(QueryHeap& queryHeap, std::uint32_t query = 0) override;
        void EndQuery(QueryHeap& queryHeap, std::uint32_t query = 0) override;

        void BeginRenderCondition(QueryHeap& queryHeap, std::uint32_t query = 0, const RenderConditionMode mode = RenderConditionMode::Wait) override;
        void EndRenderCondition() override;

        /* ----- Stream Output ------ */

        void BeginStreamOutput(std::uint32_t numBuffers, Buffer* const * buffers) override;
        void EndStreamOutput() override;

        /* ----- Drawing ----- */

        void Draw(std::uint32_t numVertices, std::uint32_t firstVertex) override;

        void DrawIndexed(std::uint32_t numIndices, std::uint32_t firstIndex) override;
        void DrawIndexed(std::uint32_t numIndices, std::uint32_t firstIndex, std::int32_t vertexOffset) override;

        void DrawInstanced(std::uint32_t numVertices, std::uint32_t firstVertex, std::uint32_t numInstances) override;
        void DrawInstanced(std::uint32_t numVertices, std::uint32_t firstVertex, std::uint32_t numInstances, std::uint32_t firstInstance) override;

        void DrawIndexedInstanced(std::uint32_t numIndices, std::uint32_t numInstances, std::uint32_t firstIndex) override;
        void DrawIndexedInstanced(std::uint32_t numIndices, std::uint32_t numInstances, std::uint32_t firstIndex, std::int32_t vertexOffset) override;
        void DrawIndexedInstanced(std::uint32_t numIndices, std::uint32_t numInstances, std::uint32_t firstIndex, std::int32_t vertexOffset, std::uint32_t firstInstance) override;

        void DrawIndirect(Buffer& buffer, std::uint64_t offset) override;
        void DrawIndirect(Buffer& buffer, std::uint64_t offset, std::uint32_t numCommands, std::uint32_t stride) override;

        void DrawIndexedIndirect(Buffer& buffer, std::uint64_t offset) override;
        void DrawIndexedIndirect(Buffer& buffer, std::uint64_t offset, std::uint32_t numCommands, std::uint32_t stride) override;

        /* ----- Compute ----- */

        void Dispatch(std::uint32_t numWorkGroupsX, std::uint32_t numWorkGroupsY, std::uint32_t numWorkGroupsZ) override;
        void DispatchIndirect(Buffer& buffer, std::uint64_t offset) override;

        /* ----- Debugging ----- */

        void PushDebugGroup(const char* name) override;
        void PopDebugGroup() override;

        /* ----- Extensions ----- */

        void SetGraphicsAPIDependentState(const void* stateDesc, std::size_t stateDescSize) override;

    public:

        /* ----- Internals ----- */

        // Returns the native VkCommandBuffer object.
        inline VkCommandBuffer GetVkCommandBuffer() const
        {
            return commandBuffer_;
        }

        // Returns the fence used to submit the command buffer to the queue.
        inline VkFence GetQueueSubmitFence() const
        {
            return recordingFence_;
        }

        // Returns true if this is an immediate command buffer, otherwise it is a deferred command buffer.
        inline bool IsImmediateCmdBuffer() const
        {
            return immediateSubmit_;
        }

    private:

        enum class RecordState
        {
            Undefined,          // before "Begin"
            OutsideRenderPass,  // after "Begin"
            InsideRenderPass,   // after "BeginRenderPass"
            ReadyForSubmit,     // after "End"
        };

    private:

        void CreateVkCommandPool(std::uint32_t queueFamilyIndex);
        void CreateVkCommandBuffers();
        void CreateVkRecordingFences();

        void ClearFramebufferAttachments(std::uint32_t numAttachments, const VkClearAttachment* attachments);

        void ConvertRenderPassClearValues(
            const VKRenderPass& renderPass,
            std::uint32_t&      dstClearValuesCount,
            VkClearValue*       dstClearValues,
            std::uint32_t       srcClearValuesCount,
            const ClearValue*   srcClearValues
        );

        void PauseRenderPass();
        void ResumeRenderPass();

        bool IsInsideRenderPass() const;

        void BufferPipelineBarrier(
            VkBuffer                buffer,
            VkDeviceSize            offset,
            VkDeviceSize            size,
            VkAccessFlags           srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT,
            VkAccessFlags           dstAccessMask   = VK_ACCESS_SHADER_READ_BIT,
            VkPipelineStageFlags    srcStageMask    = VK_PIPELINE_STAGE_TRANSFER_BIT,
            VkPipelineStageFlags    dstStageMask    = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
        );

        void FlushDescriptorCache();

        // Acquires the next native VkCommandBuffer object.
        void AcquireNextBuffer();

        void ResetBindingStates();

        #if 1//TODO: optimize
        void ResetQueryPoolsInFlight();
        void AppendQueryPoolInFlight(VKQueryHeap* queryHeap);
        #endif

    private:

        static const std::uint32_t maxNumCommandBuffers = 3;

        VKDevice&                       device_;

        VkQueue                         commandQueue_               = VK_NULL_HANDLE;

        VKPtr<VkCommandPool>            commandPool_;

        VKPtr<VkFence>                  recordingFenceArray_[maxNumCommandBuffers];
        VkFence                         recordingFence_             = VK_NULL_HANDLE;
        VkCommandBuffer                 commandBufferArray_[maxNumCommandBuffers];
        VkCommandBuffer                 commandBuffer_              = VK_NULL_HANDLE;
        std::uint32_t                   commandBufferIndex_         = 0;
        std::uint32_t                   numCommandBuffers_          = 2;

        RecordState                     recordState_                = RecordState::Undefined;

        VkCommandBufferUsageFlags       usageFlags_                 = 0;
        VkCommandBufferLevel            bufferLevel_                = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        bool                            immediateSubmit_            = false;

        VkRenderPass                    renderPass_                 = VK_NULL_HANDLE; // primary render pass
        VkRenderPass                    secondaryRenderPass_        = VK_NULL_HANDLE; // to pause/resume render pass (load and store content)
        VkFramebuffer                   framebuffer_                = VK_NULL_HANDLE; // active framebuffer handle
        VkRect2D                        framebufferRenderArea_      = { { 0, 0 }, { 0, 0 } };
        std::uint32_t                   numColorAttachments_        = 0;
        bool                            hasDepthStencilAttachment_  = false;

        std::uint32_t                   queuePresentFamily_         = 0;

        bool                            scissorEnabled_             = false;
        bool                            scissorRectInvalidated_     = true;
        VkPipelineBindPoint             pipelineBindPoint_          = VK_PIPELINE_BIND_POINT_MAX_ENUM;
        const VKPipelineLayout*         boundPipelineLayout_        = nullptr;
        VKPipelineState*                boundPipelineState_         = nullptr;

        std::uint32_t                   maxDrawIndirectCount_       = 0;

        VKStagingDescriptorSetPool      descriptorSetPoolArray_[maxNumCommandBuffers];
        VKStagingDescriptorSetPool*     descriptorSetPool_          = nullptr;
        VKDescriptorCache*              descriptorCache_            = nullptr;

        #if 1//TODO: optimize usage of query pools
        std::vector<VKQueryHeap*>       queryHeapsInFlight_;
        std::size_t                     numQueryHeapsInFlight_      = 0;
        #endif

};


} // /namespace LLGL


#endif



// ================================================================================
