/*
 * D3D11StagingBuffer.cpp
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "D3D11StagingBuffer.h"
#include "../D3D11ResourceFlags.h"
#include "../../DXCommon/DXCore.h"
#include <algorithm>


namespace LLGL
{


static D3D11_USAGE DXGetOptimalUsageForBindFlags(UINT bindFlags)
{
    if ((bindFlags & D3D11_BIND_CONSTANT_BUFFER) != 0)
        return D3D11_USAGE_DYNAMIC;
    else
        return D3D11_USAGE_DEFAULT;
}

D3D11StagingBuffer::D3D11StagingBuffer(
    ID3D11Device*   device,
    UINT            size,
    UINT            bindFlags,
    UINT            miscFlags)
:
    usage_     { DXGetOptimalUsageForBindFlags(bindFlags) },
    size_      { size                                     }
{
    /* Create new D3D11 hardware buffer (for CPU access) */
    D3D11_BUFFER_DESC descD3D;
    {
        descD3D.ByteWidth           = size;
        descD3D.Usage               = usage_;
        descD3D.BindFlags           = bindFlags;
        descD3D.CPUAccessFlags      = (usage_ == D3D11_USAGE_DYNAMIC ? D3D11_CPU_ACCESS_WRITE : 0);
        descD3D.MiscFlags           = miscFlags;
        descD3D.StructureByteStride = 0;
    }
    auto hr = device->CreateBuffer(&descD3D, nullptr, native_.GetAddressOf());
    DXThrowIfCreateFailed(hr, "ID3D11Buffer", "for CPU-access buffer");
}

D3D11StagingBuffer::D3D11StagingBuffer(D3D11StagingBuffer&& rhs) :
    native_ { std::move(rhs.native_) },
    size_   { rhs.size_              },
    offset_ { rhs.offset_            }
{
}

D3D11StagingBuffer& D3D11StagingBuffer::operator = (D3D11StagingBuffer&& rhs)
{
    if (this != &rhs)
    {
        native_ = std::move(rhs.native_);
        size_   = rhs.size_;
        offset_ = rhs.offset_;
    }
    return *this;
}

void D3D11StagingBuffer::Reset()
{
    offset_ = 0;
}

bool D3D11StagingBuffer::Capacity(UINT dataSize) const
{
    return (offset_ + dataSize <= size_);
}

void D3D11StagingBuffer::Write(
    ID3D11DeviceContext*    context,
    const void*             data,
    UINT                    dataSize)
{
    if (usage_ == D3D11_USAGE_DYNAMIC)
    {
        /* Discard previous content if the offset starts at zero, because intermediate buffers will be filled from start to end */
        const bool writeDiscard = (offset_ == 0);

        /* Update partial subresource by mapping buffer from GPU into CPU memory space */
        D3D11_MAPPED_SUBRESOURCE subresource;
        context->Map(GetNative(), 0, DXGetMapWrite(writeDiscard), 0, &subresource);
        {
            ::memcpy(reinterpret_cast<char*>(subresource.pData) + offset_, data, dataSize);
        }
        context->Unmap(GetNative(), 0);
    }
    else
    {
        /* Update sub region of buffer */
        const D3D11_BOX dstBox{ offset_, 0, 0, offset_ + dataSize, 1, 1 };
        context->UpdateSubresource(GetNative(), 0, &dstBox, data, 0, 0);
    }
}

void D3D11StagingBuffer::WriteAndIncrementOffset(
    ID3D11DeviceContext*    context,
    const void*             data,
    UINT                    dataSize,
    UINT                    stride)
{
    Write(context, data, dataSize);
    offset_ += std::max(dataSize, stride);
}


} // /namespace LLGL



// ================================================================================
