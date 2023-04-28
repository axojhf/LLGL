/*
 * DbgRenderTarget.cpp
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "DbgRenderTarget.h"
#include "DbgTexture.h"
#include "../DbgCore.h"
#include "../../CheckedCast.h"
#include "../../RenderTargetUtils.h"
#include "../../../Core/CoreUtils.h"


namespace LLGL
{


static void Convert(AttachmentFormatDescriptor& dst, const AttachmentDescriptor& src)
{
    dst.format  = GetAttachmentFormat(src);
    dst.loadOp  = AttachmentLoadOp::Load;
    dst.storeOp = AttachmentStoreOp::Store;
}

static void Convert(RenderPassDescriptor& dst, const RenderTargetDescriptor& src)
{
    std::uint32_t numColorAttachments = 0;
    for (const auto& attachment : src.attachments)
    {
        const Format format = GetAttachmentFormat(attachment);
        if (IsDepthAndStencilFormat(format))
        {
            Convert(dst.depthAttachment, attachment);
            Convert(dst.stencilAttachment, attachment);
        }
        else if (IsDepthFormat(format))
            Convert(dst.depthAttachment, attachment);
        else if (IsStencilFormat(format))
            Convert(dst.stencilAttachment, attachment);
        else if (numColorAttachments < LLGL_MAX_NUM_COLOR_ATTACHMENTS)
            Convert(dst.colorAttachments[numColorAttachments++], attachment);
    }
    dst.samples = src.samples;
}

static RenderPassDescriptor MakeRenderPassDesc(const RenderTargetDescriptor& renderTargetDesc)
{
    RenderPassDescriptor renderPassDesc;
    Convert(renderPassDesc, renderTargetDesc);
    return renderPassDesc;
}

DbgRenderTarget::DbgRenderTarget(RenderTarget& instance, RenderingDebugger* debugger, const RenderTargetDescriptor& desc) :
    instance { instance },
    desc     { desc     }
{
    if (const auto* renderPass = instance.GetRenderPass())
        renderPass_ = MakeUnique<DbgRenderPass>(*renderPass, MakeRenderPassDesc(desc));
}

void DbgRenderTarget::SetName(const char* name)
{
    DbgSetObjectName(*this, name);
}

Extent2D DbgRenderTarget::GetResolution() const
{
    return instance.GetResolution();
}

std::uint32_t DbgRenderTarget::GetSamples() const
{
    return instance.GetSamples();
}

std::uint32_t DbgRenderTarget::GetNumColorAttachments() const
{
    return instance.GetNumColorAttachments();
}

bool DbgRenderTarget::HasDepthAttachment() const
{
    return instance.HasDepthAttachment();
}

bool DbgRenderTarget::HasStencilAttachment() const
{
    return instance.HasStencilAttachment();
}

const RenderPass* DbgRenderTarget::GetRenderPass() const
{
    return renderPass_.get();
}


} // /namespace LLGL



// ================================================================================
