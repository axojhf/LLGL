/*
 * MTCommandBuffer.mm
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "MTCommandBuffer.h"
#include "MTSwapChain.h"
#include "MTTypes.h"
#include "Buffer/MTBuffer.h"
#include "Buffer/MTBufferArray.h"
#include "RenderState/MTGraphicsPSO.h"
#include "RenderState/MTComputePSO.h"
#include "RenderState/MTResourceHeap.h"
#include "RenderState/MTBuiltinPSOFactory.h"
#include "RenderState/MTDescriptorCache.h"
#include "RenderState/MTConstantsCache.h"
#include "Shader/MTShader.h"
#include "Texture/MTTexture.h"
#include "Texture/MTSampler.h"
#include "Texture/MTRenderTarget.h"
#include "../CheckedCast.h"
#include <LLGL/TypeInfo.h>
#include <LLGL/Utils/ForRange.h>
#include <algorithm>
#include <limits.h>


namespace LLGL
{


/*
Minimum size for the "FillBuffer" command to use a GPU kernel.
for smaller buffers an emulated CPU copy operation will be used.
*/
static const NSUInteger g_minFillBufferForKernel = 64;

// Default value when no compute PSO is bound.
static const MTLSize g_defaultNumThreadsPerGroup{ 1, 1, 1 };

MTCommandBuffer::MTCommandBuffer(id<MTLDevice> device, id<MTLCommandQueue> cmdQueue, const CommandBufferDescriptor& desc) :
    device_            { device                                                    },
    cmdQueue_          { cmdQueue                                                  },
    stagingBufferPool_ { device, USHRT_MAX                                         },
    immediateSubmit_   { ((desc.flags & CommandBufferFlags::ImmediateSubmit) != 0) },
    tessFactorBuffer_  { device                                                    }
{
    const NSUInteger maxCmdBuffers = 3;
    cmdBufferSemaphore_ = dispatch_semaphore_create(maxCmdBuffers);
}

MTCommandBuffer::~MTCommandBuffer()
{
    [cmdBuffer_ release];
}

/* ----- Encoding ----- */

void MTCommandBuffer::Begin()
{
    /* Wait until next command buffer becomes available */
    dispatch_semaphore_wait(cmdBufferSemaphore_, DISPATCH_TIME_FOREVER);

    /* Allocate new command buffer from command queue */
    cmdBuffer_ = [cmdQueue_ commandBuffer];

    /* Append complete handler to signal semaphore */
    __block dispatch_semaphore_t blockSemaphore = cmdBufferSemaphore_;
    [cmdBuffer_
        addCompletedHandler:^(id<MTLCommandBuffer> cmdBuffer)
        {
            dispatch_semaphore_signal(blockSemaphore);
        }
    ];

    /* Reset schedulers and pools */
    context_.Reset(cmdBuffer_);
    stagingBufferPool_.Reset();

    /* Reset references */
    numThreadsPerGroup_ = &g_defaultNumThreadsPerGroup;
}

void MTCommandBuffer::End()
{
    context_.Flush();
    PresentDrawables();

    /* Commit native buffer right after encoding for immediate command buffers */
    if (IsImmediateCmdBuffer())
        [cmdBuffer_ commit];

    ResetRenderStates();
}

void MTCommandBuffer::Execute(CommandBuffer& deferredCommandBuffer)
{
    //TODO
}

/* ----- Blitting ----- */

void MTCommandBuffer::UpdateBuffer(
    Buffer&         dstBuffer,
    std::uint64_t   dstOffset,
    const void*     data,
    std::uint16_t   dataSize)
{
    auto& dstBufferMT = LLGL_CAST(MTBuffer&, dstBuffer);

    /* Copy data to staging buffer */
    id<MTLBuffer> srcBuffer = nil;
    NSUInteger srcOffset = 0;

    stagingBufferPool_.Write(data, static_cast<NSUInteger>(dataSize), srcBuffer, srcOffset);

    /* Encode blit command to copy staging buffer region to destination buffer */
    context_.PauseRenderEncoder();
    {
        auto blitEncoder = context_.BindBlitEncoder();
        [blitEncoder
            copyFromBuffer:     srcBuffer
            sourceOffset:       srcOffset
            toBuffer:           dstBufferMT.GetNative()
            destinationOffset:  static_cast<NSUInteger>(dstOffset)
            size:               static_cast<NSUInteger>(dataSize)
        ];
    }
    context_.ResumeRenderEncoder();
}

void MTCommandBuffer::CopyBuffer(
    Buffer&         dstBuffer,
    std::uint64_t   dstOffset,
    Buffer&         srcBuffer,
    std::uint64_t   srcOffset,
    std::uint64_t   size)
{
    auto& dstBufferMT = LLGL_CAST(MTBuffer&, dstBuffer);
    auto& srcBufferMT = LLGL_CAST(MTBuffer&, srcBuffer);

    context_.PauseRenderEncoder();
    {
        auto blitEncoder = context_.BindBlitEncoder();
        [blitEncoder
            copyFromBuffer:     srcBufferMT.GetNative()
            sourceOffset:       static_cast<NSUInteger>(srcOffset)
            toBuffer:           dstBufferMT.GetNative()
            destinationOffset:  static_cast<NSUInteger>(dstOffset)
            size:               static_cast<NSUInteger>(size)
        ];
    }
    context_.ResumeRenderEncoder();
}

void MTCommandBuffer::CopyBufferFromTexture(
    Buffer&                 dstBuffer,
    std::uint64_t           dstOffset,
    Texture&                srcTexture,
    const TextureRegion&    srcRegion,
    std::uint32_t           rowStride,
    std::uint32_t           layerStride)
{
    auto& dstBufferMT = LLGL_CAST(MTBuffer&, dstBuffer);
    auto& srcTextureMT = LLGL_CAST(MTTexture&, srcTexture);

    /* Determine actual row and layer strides */
    const auto format = MTTypes::ToFormat([srcTextureMT.GetNative() pixelFormat]);
    const auto rowSize = GetMemoryFootprint(format, srcRegion.extent.width);

    if (rowStride == 0)
        rowStride = rowSize;
    if (layerStride == 0)
        layerStride = rowStride * srcRegion.extent.height;

    /* Convert to Metal origin and size */
    MTLOrigin srcOrigin;
    MTTypes::Convert(srcOrigin, srcRegion.offset);

    MTLSize srcSize;
    MTTypes::Convert(srcSize, srcRegion.extent);

    /* Encode blit commands to copy texture form buffer */
    context_.PauseRenderEncoder();
    {
        auto blitEncoder = context_.BindBlitEncoder();
        for (std::uint32_t arrayLayer = 0; arrayLayer < srcRegion.subresource.numArrayLayers; ++arrayLayer)
        {
            [blitEncoder
                copyFromTexture:            srcTextureMT.GetNative()
                sourceSlice:                (srcRegion.subresource.baseArrayLayer + arrayLayer)
                sourceLevel:                srcRegion.subresource.baseMipLevel
                sourceOrigin:               srcOrigin
                sourceSize:                 srcSize
                toBuffer:                   dstBufferMT.GetNative()
                destinationOffset:          static_cast<NSUInteger>(dstOffset)
                destinationBytesPerRow:     rowStride
                destinationBytesPerImage:   layerStride
            ];
            dstOffset += layerStride;
        }
    }
    context_.ResumeRenderEncoder();
}

void MTCommandBuffer::FillBuffer(
    Buffer&         dstBuffer,
    std::uint64_t   dstOffset,
    std::uint32_t   value,
    std::uint64_t   fillSize)
{
    if (fillSize == 0)
        return;

    auto& dstBufferMT = LLGL_CAST(MTBuffer&, dstBuffer);

    /* Check if native "fillBuffer" command can be used */
    const bool valueBytesAreEqual =
    (
        ((value >> 24) & 0x000000FF) == (value & 0x000000FF) &&
        ((value >> 16) & 0x000000FF) == (value & 0x000000FF) &&
        ((value >>  8) & 0x000000FF) == (value & 0x000000FF)
    );

    /* Determine buffer range for fill command */
    NSRange range;
    if (fillSize == Constants::wholeSize)
    {
        NSUInteger bufferSize = [dstBufferMT.GetNative() length];
        range = NSMakeRange(0, bufferSize);
    }
    else
    {
        range = NSMakeRange(
            static_cast<NSUInteger>(dstOffset),
            static_cast<NSUInteger>(fillSize)
        );
    }

    /* Fill with native command if all four bytes are equal, otherwise use blit and comput commands */
    if (valueBytesAreEqual)
        FillBufferByte1(dstBufferMT, range, static_cast<std::uint8_t>(value & 0x000000FF));
    else
        FillBufferByte4(dstBufferMT, range, value);
}

void MTCommandBuffer::CopyTexture(
    Texture&                dstTexture,
    const TextureLocation&  dstLocation,
    Texture&                srcTexture,
    const TextureLocation&  srcLocation,
    const Extent3D&         extent)
{
    auto& dstTextureMT = LLGL_CAST(MTTexture&, dstTexture);
    auto& srcTextureMT = LLGL_CAST(MTTexture&, srcTexture);

    MTLOrigin srcOrigin, dstOrigin;
    MTTypes::Convert(srcOrigin, srcLocation.offset);
    MTTypes::Convert(dstOrigin, dstLocation.offset);

    MTLSize srcSize;
    MTTypes::Convert(srcSize, extent);

    context_.PauseRenderEncoder();
    {
        auto blitEncoder = context_.BindBlitEncoder();
        [blitEncoder
            copyFromTexture:    srcTextureMT.GetNative()
            sourceSlice:        srcLocation.arrayLayer
            sourceLevel:        srcLocation.mipLevel
            sourceOrigin:       srcOrigin
            sourceSize:         srcSize
            toTexture:          dstTextureMT.GetNative()
            destinationSlice:   dstLocation.arrayLayer
            destinationLevel:   dstLocation.mipLevel
            destinationOrigin:  dstOrigin
        ];
    }
    context_.ResumeRenderEncoder();
}

void MTCommandBuffer::CopyTextureFromBuffer(
    Texture&                dstTexture,
    const TextureRegion&    dstRegion,
    Buffer&                 srcBuffer,
    std::uint64_t           srcOffset,
    std::uint32_t           rowStride,
    std::uint32_t           layerStride)
{
    auto& dstTextureMT = LLGL_CAST(MTTexture&, dstTexture);
    auto& srcBufferMT = LLGL_CAST(MTBuffer&, srcBuffer);

    /* Determine actual row and layer strides */
    const auto format = MTTypes::ToFormat([dstTextureMT.GetNative() pixelFormat]);
    const auto rowSize = GetMemoryFootprint(format, dstRegion.extent.width);

    if (rowStride == 0)
        rowStride = rowSize;
    if (layerStride == 0)
        layerStride = rowStride * dstRegion.extent.height;

    /* Convert to Metal origin and size */
    MTLOrigin dstOrigin;
    MTTypes::Convert(dstOrigin, dstRegion.offset);

    MTLSize srcSize;
    MTTypes::Convert(srcSize, dstRegion.extent);

    /* Encode blit commands to copy texture form buffer */
    context_.PauseRenderEncoder();
    {
        auto blitEncoder = context_.BindBlitEncoder();
        for (std::uint32_t arrayLayer = 0; arrayLayer < dstRegion.subresource.numArrayLayers; ++arrayLayer)
        {
            [blitEncoder
                copyFromBuffer:         srcBufferMT.GetNative()
                sourceOffset:           static_cast<NSUInteger>(srcOffset)
                sourceBytesPerRow:      rowStride
                sourceBytesPerImage:    layerStride
                sourceSize:             srcSize
                toTexture:              dstTextureMT.GetNative()
                destinationSlice:       (dstRegion.subresource.baseArrayLayer + arrayLayer)
                destinationLevel:       dstRegion.subresource.baseMipLevel
                destinationOrigin:      dstOrigin
            ];
            srcOffset += layerStride;
        }
    }
    context_.ResumeRenderEncoder();
}

void MTCommandBuffer::GenerateMips(Texture& texture)
{
    auto& textureMT = LLGL_CAST(MTTexture&, texture);
    if ([textureMT.GetNative() mipmapLevelCount] > 1)
    {
        context_.PauseRenderEncoder();
        {
            auto blitEncoder = context_.BindBlitEncoder();
            [blitEncoder generateMipmapsForTexture:textureMT.GetNative()];
        }
        context_.ResumeRenderEncoder();
    }
}

void MTCommandBuffer::GenerateMips(Texture& texture, const TextureSubresource& subresource)
{
    if (subresource.numMipLevels > 1)
    {
        auto& textureMT = LLGL_CAST(MTTexture&, texture);

        // Create temporary subresource texture to generate MIP-maps only on that range
        id<MTLTexture> intermediateTexture = textureMT.CreateSubresourceView(subresource);

        context_.PauseRenderEncoder();
        {
            auto blitEncoder = context_.BindBlitEncoder();
            [blitEncoder generateMipmapsForTexture:intermediateTexture];
        }
        context_.ResumeRenderEncoder();

        [intermediateTexture release];
    }
}

/* ----- Viewport and Scissor ----- */

void MTCommandBuffer::SetViewport(const Viewport& viewport)
{
    context_.SetViewports(&viewport, 1u);
}

void MTCommandBuffer::SetViewports(std::uint32_t numViewports, const Viewport* viewports)
{
    context_.SetViewports(viewports, numViewports);
}

void MTCommandBuffer::SetScissor(const Scissor& scissor)
{
    context_.SetScissorRects(&scissor, 1u);
}

void MTCommandBuffer::SetScissors(std::uint32_t numScissors, const Scissor* scissors)
{
    context_.SetScissorRects(scissors, numScissors);
}

/* ----- Input Assembly ------ */

void MTCommandBuffer::SetVertexBuffer(Buffer& buffer)
{
    auto& bufferMT = LLGL_CAST(MTBuffer&, buffer);
    context_.SetVertexBuffer(bufferMT.GetNative(), 0);
}

void MTCommandBuffer::SetVertexBufferArray(BufferArray& bufferArray)
{
    auto& bufferArrayMT = LLGL_CAST(MTBufferArray&, bufferArray);
    context_.SetVertexBuffers(
        bufferArrayMT.GetIDArray().data(),
        bufferArrayMT.GetOffsets().data(),
        static_cast<NSUInteger>(bufferArrayMT.GetIDArray().size())
    );
}

void MTCommandBuffer::SetIndexBuffer(Buffer& buffer)
{
    auto& bufferMT = LLGL_CAST(MTBuffer&, buffer);
    indexBuffer_        = bufferMT.GetNative();
    indexBufferOffset_  = 0;
    SetIndexType(bufferMT.IsIndexType16Bits());
}

void MTCommandBuffer::SetIndexBuffer(Buffer& buffer, const Format format, std::uint64_t offset)
{
    auto& bufferMT = LLGL_CAST(MTBuffer&, buffer);
    indexBuffer_        = bufferMT.GetNative();
    indexBufferOffset_  = static_cast<NSUInteger>(offset);
    SetIndexType(format == Format::R16UInt);
}

/* ----- Resources ----- */

void MTCommandBuffer::SetResourceHeap(ResourceHeap& resourceHeap, std::uint32_t descriptorSet)
{
    if (boundPipelineState_ == nullptr)
        return /*Invalid state*/;

    auto& resourceHeapMT = LLGL_CAST(MTResourceHeap&, resourceHeap);
    if (boundPipelineState_->IsGraphicsPSO())
    {
        if (resourceHeapMT.HasGraphicsResources())
            context_.SetGraphicsResourceHeap(&resourceHeapMT, descriptorSet);
    }
    else
    {
        if (resourceHeapMT.HasComputeResources())
            context_.SetComputeResourceHeap(&resourceHeapMT, descriptorSet);
    }
}

void MTCommandBuffer::SetResource(std::uint32_t descriptor, Resource& resource)
{
    if (descriptorCache_ != nullptr)
        descriptorCache_->SetResource(descriptor, resource);
}

void MTCommandBuffer::ResetResourceSlots(
    const ResourceType  resourceType,
    std::uint32_t       firstSlot,
    std::uint32_t       numSlots,
    long                bindFlags,
    long                stageFlags)
{
    //todo
}

/* ----- Render Passes ----- */

void MTCommandBuffer::BeginRenderPass(
    RenderTarget&       renderTarget,
    const RenderPass*   renderPass,
    std::uint32_t       numClearValues,
    const ClearValue*   clearValues)
{
    if (LLGL::IsInstanceOf<SwapChain>(renderTarget))
    {
        /* Put current drawable into queue */
        auto& swapChainMT = LLGL_CAST(MTSwapChain&, renderTarget);
        QueueDrawable(swapChainMT.GetMTKView().currentDrawable);

        /* Get next render pass descriptor from MetalKit view */
        if (renderPass != nullptr)
        {
            auto* renderPassMT = LLGL_CAST(const MTRenderPass*, renderPass);
            context_.BindRenderEncoder(swapChainMT.GetAndUpdateNativeRenderPass(*renderPassMT, numClearValues, clearValues), true);
        }
        else
            context_.BindRenderEncoder(swapChainMT.GetNativeRenderPass(), true);
    }
    else
    {
        /* Get render pass descriptor from render target */
        auto& renderTargetMT = LLGL_CAST(MTRenderTarget&, renderTarget);
        if (renderPass != nullptr)
        {
            auto* renderPassMT = LLGL_CAST(const MTRenderPass*, renderPass);
            context_.BindRenderEncoder(renderTargetMT.GetAndUpdateNativeRenderPass(*renderPassMT, numClearValues, clearValues), true);
        }
        else
            context_.BindRenderEncoder(renderTargetMT.GetNativeRenderPass(), true);
    }
}

void MTCommandBuffer::EndRenderPass()
{
    context_.Flush();
}

void MTCommandBuffer::Clear(long flags, const ClearValue& clearValue)
{
    if (context_.GetRenderEncoder() != nil && flags != 0)
    {
        /* Make new render pass descriptor with current clear values */
        auto renderPassDesc = context_.CopyRenderPassDesc();

        if ((flags & ClearFlags::Color) != 0)
        {
            renderPassDesc.colorAttachments[0].loadAction   = MTLLoadActionClear;
            renderPassDesc.colorAttachments[0].clearColor   = MTTypes::ToMTLClearColor(clearValue.color);
        }

        if ((flags & ClearFlags::Depth) != 0)
        {
            renderPassDesc.depthAttachment.loadAction       = MTLLoadActionClear;
            renderPassDesc.depthAttachment.clearDepth       = static_cast<double>(clearValue.depth);
        }

        if ((flags & ClearFlags::Stencil) != 0)
        {
            renderPassDesc.stencilAttachment.loadAction     = MTLLoadActionClear;
            renderPassDesc.stencilAttachment.clearStencil   = clearValue.stencil;
        }

        /* Begin with new render pass to clear buffers */
        context_.BindRenderEncoder(renderPassDesc);
        [renderPassDesc release];
    }
}

// Fills the MTLRenderPassDescriptor object according to the secified attachment clear command
static void FillMTRenderPassDesc(MTLRenderPassDescriptor* renderPassDesc, const AttachmentClear& attachment)
{
    if ((attachment.flags & ClearFlags::Color) != 0)
    {
        /* Clear color buffer */
        auto colorBuffer = attachment.colorAttachment;
        renderPassDesc.colorAttachments[colorBuffer].loadAction = MTLLoadActionClear;
        renderPassDesc.colorAttachments[colorBuffer].clearColor = MTTypes::ToMTLClearColor(attachment.clearValue.color);
    }
    else if ((attachment.flags & ClearFlags::Depth) != 0)
    {
        /* Clear depth buffer */
        renderPassDesc.depthAttachment.loadAction = MTLLoadActionClear;
        renderPassDesc.depthAttachment.clearDepth = static_cast<double>(attachment.clearValue.depth);
    }
    else if ((attachment.flags & ClearFlags::Stencil) != 0)
    {
        /* Clear stencil buffer */
        renderPassDesc.stencilAttachment.loadAction     = MTLLoadActionClear;
        renderPassDesc.stencilAttachment.clearStencil   = attachment.clearValue.stencil;
    }
}

void MTCommandBuffer::ClearAttachments(std::uint32_t numAttachments, const AttachmentClear* attachments)
{
    if (context_.GetRenderEncoder() != nil && numAttachments > 0)
    {
        /* Make new render pass descriptor with current clear values */
        auto renderPassDesc = context_.CopyRenderPassDesc();

        for_range(i, numAttachments)
            FillMTRenderPassDesc(renderPassDesc, attachments[i]);

        /* Begin with new render pass to clear buffers */
        context_.BindRenderEncoder(renderPassDesc);
        [renderPassDesc release];
    }
}

/* ----- Pipeline States ----- */

static NSUInteger GetTessFactorSize(const MTLPatchType patchType)
{
    switch (patchType)
    {
        case MTLPatchTypeNone:      return 0;
        case MTLPatchTypeTriangle:  return sizeof(MTLTriangleTessellationFactorsHalf);
        case MTLPatchTypeQuad:      return sizeof(MTLQuadTessellationFactorsHalf);
    }
    return 0;
}

void MTCommandBuffer::SetPipelineState(PipelineState& pipelineState)
{
    /* Set graphics pipeline with encoder scheduler */
    auto& pipelineStateMT = LLGL_CAST(MTPipelineState&, pipelineState);
    boundPipelineState_ = &pipelineStateMT;

    /* Store and reset descriptor cache */
    descriptorCache_    = pipelineStateMT.GetDescriptorCache();
    constantsCache_     = pipelineStateMT.GetConstantsCache();

    if (pipelineStateMT.IsGraphicsPSO())
    {
        /* Schedule graphics pipeline and store primitive type for draw commands */
        auto& graphicsPSO = LLGL_CAST(MTGraphicsPSO&, pipelineStateMT);
        context_.SetGraphicsPSO(&graphicsPSO);

        /* Store current primitive type and tessellation data */
        primitiveType_          = graphicsPSO.GetMTLPrimitiveType();
        numPatchControlPoints_  = graphicsPSO.GetNumPatchControlPoints();
        tessPipelineState_      = graphicsPSO.GetTessPipelineState();
        tessFactorSize_         = GetTessFactorSize(graphicsPSO.GetPatchType());
    }
    else
    {
        /* Set compute pipeline with encoder scheduler */
        auto& computePSO = LLGL_CAST(MTComputePSO&, pipelineStateMT);
        context_.SetComputePSO(&computePSO);

        /* Store reference to work group size of shader program */
        if (auto computeShader = computePSO.GetComputeShader())
            numThreadsPerGroup_ = &(computeShader->GetNumThreadsPerGroup());
        else
            numThreadsPerGroup_ = &g_defaultNumThreadsPerGroup;
    }
}

void MTCommandBuffer::SetBlendFactor(const float color[4])
{
    context_.SetBlendColor(color);
}

void MTCommandBuffer::SetStencilReference(std::uint32_t reference, const StencilFace stencilFace)
{
    context_.SetStencilRef(reference, stencilFace);
}

void MTCommandBuffer::SetUniforms(std::uint32_t first, const void* data, std::uint16_t dataSize)
{
    if (constantsCache_ != nullptr)
        constantsCache_->SetUniforms(first, data, dataSize);
}

/* ----- Queries ----- */

void MTCommandBuffer::BeginQuery(QueryHeap& queryHeap, std::uint32_t query)
{
    //todo
}

void MTCommandBuffer::EndQuery(QueryHeap& queryHeap, std::uint32_t query)
{
    //todo
}

void MTCommandBuffer::BeginRenderCondition(QueryHeap& queryHeap, std::uint32_t query, const RenderConditionMode mode)
{
    //todo
}

void MTCommandBuffer::EndRenderCondition()
{
    //todo
}

/* ----- Stream Output ------ */

void MTCommandBuffer::BeginStreamOutput(std::uint32_t numBuffers, Buffer* const * buffers)
{
    //todo
}

void MTCommandBuffer::EndStreamOutput()
{
    //todo
}

/* ----- Drawing ----- */

void MTCommandBuffer::Draw(std::uint32_t numVertices, std::uint32_t firstVertex)
{
    if (numPatchControlPoints_ > 0)
    {
        const NSUInteger firstPatch = (static_cast<NSUInteger>(firstVertex) / numPatchControlPoints_);
        const NSUInteger numPatches = (static_cast<NSUInteger>(numVertices) / numPatchControlPoints_);

        DispatchTessellatorStage(numPatches);

        auto renderEncoder = GetRenderEncoderForPatches(numPatches);
        [renderEncoder
            drawPatches:            numPatchControlPoints_
            patchStart:             firstPatch
            patchCount:             numPatches
            patchIndexBuffer:       nil
            patchIndexBufferOffset: 0
            instanceCount:          1
            baseInstance:           0
        ];
    }
    else
    {
        auto renderEncoder = context_.FlushAndGetRenderEncoder();
        [renderEncoder
            drawPrimitives: primitiveType_
            vertexStart:    static_cast<NSUInteger>(firstVertex)
            vertexCount:    static_cast<NSUInteger>(numVertices)
        ];
    }
}

void MTCommandBuffer::DrawIndexed(std::uint32_t numIndices, std::uint32_t firstIndex)
{
    if (numPatchControlPoints_ > 0)
    {
        const NSUInteger firstPatch = (static_cast<NSUInteger>(firstIndex) / numPatchControlPoints_);
        const NSUInteger numPatches = (static_cast<NSUInteger>(numIndices) / numPatchControlPoints_);

        DispatchTessellatorStage(numPatches);

        auto renderEncoder = GetRenderEncoderForPatches(numPatches);
        [renderEncoder
            drawIndexedPatches:             numPatchControlPoints_
            patchStart:                     firstPatch
            patchCount:                     numPatches
            patchIndexBuffer:               nil
            patchIndexBufferOffset:         0
            controlPointIndexBuffer:        indexBuffer_
            controlPointIndexBufferOffset:  indexBufferOffset_ + indexTypeSize_ * static_cast<NSUInteger>(firstIndex)
            instanceCount:                  1
            baseInstance:                   0
        ];
    }
    else
    {
        auto renderEncoder = context_.FlushAndGetRenderEncoder();
        [renderEncoder
            drawIndexedPrimitives:  primitiveType_
            indexCount:             static_cast<NSUInteger>(numIndices)
            indexType:              indexType_
            indexBuffer:            indexBuffer_
            indexBufferOffset:      indexBufferOffset_ + indexTypeSize_ * static_cast<NSUInteger>(firstIndex)
        ];
    }
}

void MTCommandBuffer::DrawIndexed(std::uint32_t numIndices, std::uint32_t firstIndex, std::int32_t vertexOffset)
{
    MTCommandBuffer::DrawIndexedInstanced(numIndices, /*numInstances:*/1, firstIndex, vertexOffset, /*firstInstance:*/0);
}

void MTCommandBuffer::DrawInstanced(std::uint32_t numVertices, std::uint32_t firstVertex, std::uint32_t numInstances)
{
    MTCommandBuffer::DrawInstanced(numVertices, firstVertex, numInstances, /*firstInstance:*/0);
}

void MTCommandBuffer::DrawInstanced(std::uint32_t numVertices, std::uint32_t firstVertex, std::uint32_t numInstances, std::uint32_t firstInstance)
{
    if (numPatchControlPoints_ > 0)
    {
        const NSUInteger firstPatch = (static_cast<NSUInteger>(firstVertex) / numPatchControlPoints_);
        const NSUInteger numPatches = (static_cast<NSUInteger>(numVertices) / numPatchControlPoints_);

        DispatchTessellatorStage(numPatches * numInstances);

        auto renderEncoder = GetRenderEncoderForPatches(numPatches);
        [renderEncoder
            drawPatches:            numPatchControlPoints_
            patchStart:             firstPatch
            patchCount:             numPatches
            patchIndexBuffer:       nil
            patchIndexBufferOffset: 0
            instanceCount:          static_cast<NSUInteger>(numInstances)
            baseInstance:           static_cast<NSUInteger>(firstInstance)
        ];
    }
    else
    {
        auto renderEncoder = context_.FlushAndGetRenderEncoder();
        [renderEncoder
            drawPrimitives: primitiveType_
            vertexStart:    static_cast<NSUInteger>(firstVertex)
            vertexCount:    static_cast<NSUInteger>(numVertices)
            instanceCount:  static_cast<NSUInteger>(numInstances)
            baseInstance:   static_cast<NSUInteger>(firstInstance)
        ];
    }
}

void MTCommandBuffer::DrawIndexedInstanced(std::uint32_t numIndices, std::uint32_t numInstances, std::uint32_t firstIndex)
{
    MTCommandBuffer::DrawIndexedInstanced(numIndices, numInstances, firstIndex, /*vertexOffset:*/0, /*firstInstance:*/0);
}

void MTCommandBuffer::DrawIndexedInstanced(std::uint32_t numIndices, std::uint32_t numInstances, std::uint32_t firstIndex, std::int32_t vertexOffset)
{
    MTCommandBuffer::DrawIndexedInstanced(numIndices, numInstances, firstIndex, vertexOffset, /*firstInstance:*/0);
}

void MTCommandBuffer::DrawIndexedInstanced(std::uint32_t numIndices, std::uint32_t numInstances, std::uint32_t firstIndex, std::int32_t vertexOffset, std::uint32_t firstInstance)
{
    if (numPatchControlPoints_ > 0)
    {
        const NSUInteger firstPatch = (static_cast<NSUInteger>(firstIndex) / numPatchControlPoints_);
        const NSUInteger numPatches = (static_cast<NSUInteger>(numIndices) / numPatchControlPoints_);

        DispatchTessellatorStage(numPatches * numInstances);

        auto renderEncoder = GetRenderEncoderForPatches(numPatches);
        [renderEncoder
            drawIndexedPatches:             numPatchControlPoints_
            patchStart:                     firstPatch
            patchCount:                     numPatches
            patchIndexBuffer:               nil
            patchIndexBufferOffset:         0
            controlPointIndexBuffer:        indexBuffer_
            controlPointIndexBufferOffset:  indexBufferOffset_ + indexTypeSize_ * static_cast<NSUInteger>(firstIndex)
            instanceCount:                  static_cast<NSUInteger>(numInstances)
            baseInstance:                   static_cast<NSUInteger>(firstInstance)
        ];
    }
    else
    {
        auto renderEncoder = context_.FlushAndGetRenderEncoder();
        [renderEncoder
            drawIndexedPrimitives:  primitiveType_
            indexCount:             static_cast<NSUInteger>(numIndices)
            indexType:              indexType_
            indexBuffer:            indexBuffer_
            indexBufferOffset:      indexBufferOffset_ + indexTypeSize_ * static_cast<NSUInteger>(firstIndex)
            instanceCount:          static_cast<NSUInteger>(numInstances)
            baseVertex:             static_cast<NSUInteger>(vertexOffset)
            baseInstance:           static_cast<NSUInteger>(firstInstance)
        ];
    }
}

[[noreturn]]
static void ThrowIndirectPatchesNotSupported()
{
    throw std::runtime_error("tessellation with indirect arguments not supported in Metal backend yet");
}

//TODO: support patches with indirect arguments
void MTCommandBuffer::DrawIndirect(Buffer& buffer, std::uint64_t offset)
{
    auto& bufferMT = LLGL_CAST(MTBuffer&, buffer);
    if (numPatchControlPoints_ > 0)
    {
        #if 0 //TODO
        [renderEncoder
            drawPatches:            numPatchControlPoints_
            patchIndexBuffer:       nil
            patchIndexBufferOffset: 0
            indirectBuffer:         bufferMT.GetNative()
            indirectBufferOffset:   static_cast<NSUInteger>(offset)
        ];
        #else
        ThrowIndirectPatchesNotSupported();
        #endif
    }
    else
    {
        auto renderEncoder = context_.FlushAndGetRenderEncoder();
        [renderEncoder
            drawPrimitives:         primitiveType_
            indirectBuffer:         bufferMT.GetNative()
            indirectBufferOffset:   static_cast<NSUInteger>(offset)
        ];
    }
}

//TODO: support patches with indirect arguments
void MTCommandBuffer::DrawIndirect(Buffer& buffer, std::uint64_t offset, std::uint32_t numCommands, std::uint32_t stride)
{
    auto& bufferMT = LLGL_CAST(MTBuffer&, buffer);
    if (numPatchControlPoints_ > 0)
    {
        #if 0 //TODO
        while (numCommands-- > 0)
        {
            [renderEncoder
                drawPatches:            numPatchControlPoints_
                patchIndexBuffer:       nil
                patchIndexBufferOffset: 0
                indirectBuffer:         bufferMT.GetNative()
                indirectBufferOffset:   static_cast<NSUInteger>(offset)
            ];
            offset += stride;
        }
        #else
        ThrowIndirectPatchesNotSupported();
        #endif
    }
    else
    {
        auto renderEncoder = context_.FlushAndGetRenderEncoder();
        while (numCommands-- > 0)
        {
            [renderEncoder
                drawPrimitives:         primitiveType_
                indirectBuffer:         bufferMT.GetNative()
                indirectBufferOffset:   static_cast<NSUInteger>(offset)
            ];
            offset += stride;
        }
    }
}

//TODO: support patches with indirect arguments
void MTCommandBuffer::DrawIndexedIndirect(Buffer& buffer, std::uint64_t offset)
{
    auto& bufferMT = LLGL_CAST(MTBuffer&, buffer);
    if (numPatchControlPoints_ > 0)
    {
        #if 0 //TODO
        [renderEncoder
            drawIndexedPatches:             numPatchControlPoints_
            patchIndexBuffer:               nil
            patchIndexBufferOffset:         0
            controlPointIndexBuffer:        indexBuffer_
            controlPointIndexBufferOffset:  0
            indirectBuffer:                 bufferMT.GetNative()
            indirectBufferOffset:           static_cast<NSUInteger>(offset)
        ];
        #else
        ThrowIndirectPatchesNotSupported();
        #endif
    }
    else
    {
        auto renderEncoder = context_.FlushAndGetRenderEncoder();
        [renderEncoder
            drawIndexedPrimitives:  primitiveType_
            indexType:              indexType_
            indexBuffer:            indexBuffer_
            indexBufferOffset:      0
            indirectBuffer:         bufferMT.GetNative()
            indirectBufferOffset:   static_cast<NSUInteger>(offset)
        ];
    }
}

//TODO: support patches with indirect arguments
void MTCommandBuffer::DrawIndexedIndirect(Buffer& buffer, std::uint64_t offset, std::uint32_t numCommands, std::uint32_t stride)
{
    auto& bufferMT = LLGL_CAST(MTBuffer&, buffer);
    if (numPatchControlPoints_ > 0)
    {
        #if 0 //TODO
        while (numCommands-- > 0)
        {
            [renderEncoder
                drawIndexedPatches:             numPatchControlPoints_
                patchIndexBuffer:               nil
                patchIndexBufferOffset:         0
                controlPointIndexBuffer:        indexBuffer_
                controlPointIndexBufferOffset:  0
                indirectBuffer:                 bufferMT.GetNative()
                indirectBufferOffset:           static_cast<NSUInteger>(offset)
            ];
            offset += stride;
        }
        #else
        ThrowIndirectPatchesNotSupported();
        #endif
    }
    else
    {
        auto renderEncoder = context_.FlushAndGetRenderEncoder();
        while (numCommands-- > 0)
        {
            [renderEncoder
                drawIndexedPrimitives:  primitiveType_
                indexType:              indexType_
                indexBuffer:            indexBuffer_
                indexBufferOffset:      0
                indirectBuffer:         bufferMT.GetNative()
                indirectBufferOffset:   static_cast<NSUInteger>(offset)
            ];
            offset += stride;
        }
    }
}

/* ----- Compute ----- */

void MTCommandBuffer::Dispatch(std::uint32_t numWorkGroupsX, std::uint32_t numWorkGroupsY, std::uint32_t numWorkGroupsZ)
{
    auto computeEncoder = context_.FlushAndGetComputeEncoder();
    [computeEncoder
        dispatchThreadgroups:   MTLSizeMake(numWorkGroupsX, numWorkGroupsY, numWorkGroupsZ)
        threadsPerThreadgroup:  *numThreadsPerGroup_
    ];
}

void MTCommandBuffer::DispatchIndirect(Buffer& buffer, std::uint64_t offset)
{
    auto& bufferMT = LLGL_CAST(MTBuffer&, buffer);
    auto computeEncoder = context_.FlushAndGetComputeEncoder();
    [computeEncoder
        dispatchThreadgroupsWithIndirectBuffer: bufferMT.GetNative()
        indirectBufferOffset:                   static_cast<NSUInteger>(offset)
        threadsPerThreadgroup:                  *numThreadsPerGroup_
    ];
}

/* ----- Debugging ----- */

void MTCommandBuffer::PushDebugGroup(const char* name)
{
    #ifdef LLGL_DEBUG
    [cmdBuffer_ pushDebugGroup:[NSString stringWithUTF8String:name]];
    #endif // /LLGL_DEBUG
}

void MTCommandBuffer::PopDebugGroup()
{
    #ifdef LLGL_DEBUG
    [cmdBuffer_ popDebugGroup];
    #endif // /LLGL_DEBUG
}

/* ----- Extensions ----- */

void MTCommandBuffer::SetGraphicsAPIDependentState(const void* stateDesc, std::size_t stateDescSize)
{
    if (stateDesc != nullptr && stateDescSize == sizeof(MetalDependentStateDescriptor))
    {
        const auto stateDescMT = reinterpret_cast<const MetalDependentStateDescriptor*>(stateDesc);
        tessFactorBufferSlot_ = stateDescMT->tessFactorBufferSlot;
    }
}


/*
 * ======= Private: =======
 */

void MTCommandBuffer::SetIndexType(bool indexType16Bits)
{
    if (indexType16Bits)
    {
        indexType_      = MTLIndexTypeUInt16;
        indexTypeSize_  = 2;
    }
    else
    {
        indexType_      = MTLIndexTypeUInt32;
        indexTypeSize_  = 4;
    }
}

void MTCommandBuffer::QueueDrawable(id<MTLDrawable> drawable)
{
    for (auto d : drawables_)
    {
        if (d == drawable)
            return;
    }
    drawables_.push_back(drawable);
}

void MTCommandBuffer::PresentDrawables()
{
    for (auto d : drawables_)
        [cmdBuffer_ presentDrawable:d];
    drawables_.clear();
}

void MTCommandBuffer::FillBufferByte1(MTBuffer& bufferMT, const NSRange& range, std::uint8_t value)
{
    context_.PauseRenderEncoder();
    {
        auto blitEncoder = context_.BindBlitEncoder();
        [blitEncoder fillBuffer:bufferMT.GetNative() range:range value:value];
    }
    context_.ResumeRenderEncoder();
}

void MTCommandBuffer::FillBufferByte4(MTBuffer& bufferMT, const NSRange& range, std::uint32_t value)
{
    /* Use emulated fill command if buffer range is small enough to avoid having both a blit and compute encoder */
    if (range.length > g_minFillBufferForKernel)
        FillBufferByte4Accelerated(bufferMT, range, value);
    else
        FillBufferByte4Emulated(bufferMT, range, value);
}

void MTCommandBuffer::FillBufferByte4Emulated(MTBuffer& bufferMT, const NSRange& range, std::uint32_t value)
{
    /* Copy value into stack local buffer */
    std::uint32_t localBuffer[g_minFillBufferForKernel / sizeof(std::uint32_t)];
    std::fill(std::begin(localBuffer), std::end(localBuffer), value);

    /* Write clear value into first range of destination buffer */
    UpdateBuffer(bufferMT, range.location, localBuffer, range.length);
}

//TODO: manage binding of compute PSO in MTCommandContext
void MTCommandBuffer::FillBufferByte4Accelerated(MTBuffer& bufferMT, const NSRange& range, std::uint32_t value)
{
    context_.PauseRenderEncoder();
    {
        auto computeEncoder = context_.BindComputeEncoder();

        /* Bind compute PSO with kernel to fill buffer */
        id<MTLComputePipelineState> pso = MTBuiltinPSOFactory::Get().GetComputePSO(MTBuiltinComputePSO::FillBufferByte4);
        [computeEncoder setComputePipelineState:pso];

        /* Bind destination buffer range and store clear value as input constant buffer */
        [computeEncoder setBuffer:bufferMT.GetNative() offset:range.location atIndex:0];
        [computeEncoder setBytes:&value length:sizeof(value) atIndex:1];

        /* Dispatch compute kernels */
        const NSUInteger numValues = range.length / sizeof(std::uint32_t);
        DispatchThreads1D(computeEncoder, pso, numValues);
    }
    context_.ResumeRenderEncoder();
}

void MTCommandBuffer::DispatchTessellatorStage(NSUInteger numPatchesAndInstances)
{
    if (tessPipelineState_ != nil)
    {
        /* Ensure internal tessellation factor buffer is large enough */
        tessFactorBuffer_.Grow(tessFactorSize_ * numPatchesAndInstances);

        /* Encode kernel dispatch to generate tessellation factors for each patch */
        context_.PauseRenderEncoder();
        {
            auto computeEncoder = context_.BindComputeEncoder();

            /* Rebind resource heap to bind compute stage resources to the new compute command encoder */
            context_.RebindResourceHeap(computeEncoder);

            /* Disaptch kernel to generate patch tessellation factors */
            [computeEncoder setComputePipelineState: tessPipelineState_];
            [computeEncoder setBuffer:tessFactorBuffer_.GetNative() offset:0 atIndex:tessFactorBufferSlot_];

            DispatchThreads1D(computeEncoder, tessPipelineState_, numPatchesAndInstances);
        }
        context_.ResumeRenderEncoder();
    }
}

id<MTLRenderCommandEncoder> MTCommandBuffer::GetRenderEncoderForPatches(NSUInteger numPatches)
{
    auto renderEncoder = context_.FlushAndGetRenderEncoder();

    [renderEncoder
        setTessellationFactorBuffer:    tessFactorBuffer_.GetNative()
        offset:                         0
        instanceStride:                 numPatches * sizeof(MTLQuadTessellationFactorsHalf)
    ];

    return renderEncoder;
}

void MTCommandBuffer::DispatchThreads1D(
    id<MTLComputeCommandEncoder>    computeEncoder,
    id<MTLComputePipelineState>     computePSO,
    NSUInteger                      numThreads)
{
    const NSUInteger maxLocalThreads = std::min(
        device_.maxThreadsPerThreadgroup.width,
        computePSO.maxTotalThreadsPerThreadgroup
    );

    if (@available(iOS 11.0, macOS 10.13, *))
    {
        /* Dispatch all threads with a single command and let Metal distribute the full and partial threadgroups */
        [computeEncoder
            dispatchThreads:        MTLSizeMake(numThreads, 1, 1)
            threadsPerThreadgroup:  MTLSizeMake(std::min(numThreads, maxLocalThreads), 1, 1)
        ];
    }
    else
    {
        /* Disaptch threadgroups with as many local threads as possible */
        const NSUInteger numThreadGroups = numThreads / maxLocalThreads;
        if (numThreadGroups > 0)
        {
            [computeEncoder
                dispatchThreadgroups:   MTLSizeMake(numThreadGroups, 1, 1)
                threadsPerThreadgroup:  MTLSizeMake(maxLocalThreads, 1, 1)
            ];
        }

        /* Dispatch local threads for remaining range */
        const NSUInteger remainingValues = numThreads % maxLocalThreads;
        if (remainingValues > 0)
        {
            [computeEncoder
                dispatchThreadgroups:   MTLSizeMake(1, 1, 1)
                threadsPerThreadgroup:  MTLSizeMake(remainingValues, 1, 1)
            ];
        }
    }
}

void MTCommandBuffer::ResetRenderStates()
{
    numPatchControlPoints_  = 0;
    tessPipelineState_      = nil;
    boundPipelineState_     = nullptr;
    descriptorCache_        = nullptr;
    constantsCache_         = nullptr;
}


} // /namespace LLGL



// ================================================================================
