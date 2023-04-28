/*
 * ResourceUtils.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_RESOURCE_UTILS_H
#define LLGL_RESOURCE_UTILS_H


#include <LLGL/ResourceFlags.h>
#include <LLGL/ResourceHeapFlags.h>
#include <LLGL/RenderSystemFlags.h>
#include <LLGL/Container/ArrayView.h>
#include <stdexcept>
#include <string>


namespace LLGL
{


/* ----- Enumerations ----- */

// Enumeration of predefined static sampler border colors.
enum class StaticSamplerBorderColor
{
    TransparentBlack,   // Predefined border color { 0, 0, 0, 0 }
    OpaqueBlack,        // Predefined border color { 1, 1, 1, 0 }
    OpaqueWhite,        // Predefined border color { 1, 1, 1, 1 }
};


/* ----- Functions ----- */

// Returns true if the specified flags contain any input binding flags.
inline bool HasInputBindFlags(long bindFlags)
{
    const long inputBindFlags = (BindFlags::Sampled | BindFlags::CopySrc | BindFlags::VertexBuffer | BindFlags::IndexBuffer | BindFlags::ConstantBuffer | BindFlags::IndirectBuffer);
    return ((bindFlags & inputBindFlags) != 0);
}

// Returns true if the specified flags contain any output binding flags.
inline bool HasOutputBindFlags(long bindFlags)
{
    const long outputBindFlags = (BindFlags::Storage | BindFlags::CopyDst | BindFlags::ColorAttachment | BindFlags::DepthStencilAttachment | BindFlags::StreamOutputBuffer);
    return ((bindFlags & outputBindFlags) != 0);
}

// Returns true if the specified CPU access value has read access, i.e. ReadOnly or ReadWrite.
inline bool HasReadAccess(const CPUAccess access)
{
    return (access == CPUAccess::ReadOnly || access == CPUAccess::ReadWrite);
}

// Returns true if the specified CPU access value has write access, i.e. WriteOnly, WriteDiscard, or ReadWrite.
inline bool HasWriteAccess(const CPUAccess access)
{
    return (access >= CPUAccess::WriteOnly && access <= CPUAccess::ReadWrite);
}

// Returns the number of resource views for the specified resource heap descriptor and throws an std::invalid_argument exception if validation fails.
inline std::uint32_t GetNumResourceViewsOrThrow(
    std::uint32_t                               numBindings,
    const ResourceHeapDescriptor&               desc,
    const ArrayView<ResourceViewDescriptor>&    initialResourceViews)
{
    /* Resource heaps cannot have pipeline layout with no bindings */
    if (numBindings == 0)
        throw std::invalid_argument("cannot create resource heap without bindings in pipeline layout");

    /* Resource heaps cannot be empty */
    const std::uint32_t numResourceViews = (desc.numResourceViews > 0 ? desc.numResourceViews : static_cast<std::uint32_t>(initialResourceViews.size()));
    if (numResourceViews == 0)
        throw std::invalid_argument("cannot create empty resource heap");

    /* Number of resources must be a multiple of bindings */
    if (numResourceViews % numBindings != 0)
    {
        throw std::invalid_argument(
            "cannot create resource heap because number of resources (" + std::to_string(numResourceViews) +
            ") is not a multiple of bindings (" + std::to_string(numBindings) + ")"
        );
    }

    return numResourceViews;
}

// Returns the enumeration value for a predefined static sampler border color.
inline StaticSamplerBorderColor GetStaticSamplerBorderColor(const float (&color)[4])
{
    if (color[3] > 0.5f)
    {
        if (color[0] <= 0.5f && color[1] <= 0.5f && color[2] <= 0.5f)
            return StaticSamplerBorderColor::OpaqueBlack;
        if (color[0] > 0.5f && color[1] > 0.5f && color[2] > 0.5f)
            return StaticSamplerBorderColor::OpaqueWhite;
    }
    return StaticSamplerBorderColor::TransparentBlack;
}


} // /namespace LLGL


#endif



// ================================================================================
