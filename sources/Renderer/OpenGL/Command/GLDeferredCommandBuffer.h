/*
 * GLDeferredCommandBuffer.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_GL_DEFERRED_COMMAND_BUFFER_H
#define LLGL_GL_DEFERRED_COMMAND_BUFFER_H


#include "GLCommandBuffer.h"
#include "GLCommandOpcode.h"
#include "../../VirtualCommandBuffer.h"
#include <memory>
#include <vector>

#ifdef LLGL_ENABLE_JIT_COMPILER
#   include "../../../JIT/JITProgram.h"
#endif


namespace LLGL
{


class GLBuffer;
class GLBufferArray;
class GLTexture;
class GLSampler;
class GLRenderTarget;
class GLSwapChain;
class GLStateManager;
class GLRenderPass;
class GLShaderPipeline;
#ifdef LLGL_GL_ENABLE_OPENGL2X
class GL2XSampler;
#endif

using GLVirtualCommandBuffer = VirtualCommandBuffer<GLOpcode>;

class GLDeferredCommandBuffer final : public GLCommandBuffer
{

    public:

        GLDeferredCommandBuffer(long flags, std::size_t initialBufferSize = 1024);

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

    public:

        /* ----- Internal ----- */

        // Returns false.
        bool IsImmediateCmdBuffer() const override;

        // Returns true if this is a primary command buffer.
        bool IsPrimary() const;

        // Returns the internal command buffer as raw byte buffer.
        inline const GLVirtualCommandBuffer& GetVirtualCommandBuffer() const
        {
            return buffer_;
        }

        // Returns the flags this command buffer was created with (see CommandBufferDescriptor::flags).
        inline long GetFlags() const
        {
            return flags_;
        }

        #ifdef LLGL_ENABLE_JIT_COMPILER

        // Returns the just-in-time compiled command buffer that can be executed natively, or null if not available.
        inline const std::unique_ptr<JITProgram>& GetExecutable() const
        {
            return executable_;
        }

        // Returns the maximum number of viewports that are set in this command buffer.
        inline std::uint32_t GetMaxNumViewports() const
        {
            return maxNumViewports_;
        }

        // Returns the maximum number of scissors that are set in this command buffer.
        inline std::uint32_t GetMaxNumScissors() const
        {
            return maxNumScissors_;
        }

        #endif // /LLGL_ENABLE_JIT_COMPILER

    private:

        void BindBufferBase(const GLBufferTarget bufferTarget, const GLBuffer& bufferGL, std::uint32_t slot);
        void BindBuffersBase(const GLBufferTarget bufferTarget, std::uint32_t first, std::uint32_t count, const Buffer *const *const buffers);
        void BindTexture(GLTexture& textureGL, std::uint32_t slot);
        void BindImageTexture(const GLTexture& textureGL, std::uint32_t slot);
        void BindSampler(const GLSampler& samplerGL, std::uint32_t slot);
        #ifdef LLGL_GL_ENABLE_OPENGL2X
        void BindGL2XSampler(const GL2XSampler& samplerGL2X, std::uint32_t slot);
        #endif

        /* Allocates only an opcode for empty commands */
        void AllocOpcode(const GLOpcode opcode);

        /* Allocates a new command and stores the specified opcode */
        template <typename TCommand>
        TCommand* AllocCommand(const GLOpcode opcode, std::size_t payloadSize = 0);

    private:

        long                        flags_                  = 0;
        GLVirtualCommandBuffer      buffer_;

        #ifdef LLGL_ENABLE_JIT_COMPILER
        std::unique_ptr<JITProgram> executable_;
        std::uint32_t               maxNumViewports_        = 0;
        std::uint32_t               maxNumScissors_         = 0;
        #endif // /LLGL_ENABLE_JIT_COMPILER

};


} // /namespace LLGL


#endif



// ================================================================================
