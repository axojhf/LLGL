/*
 * D3D11Buffer.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_D3D11_BUFFER_H
#define LLGL_D3D11_BUFFER_H


#include <LLGL/Buffer.h>
#include "../../DXCommon/ComPtr.h"
#include <d3d11.h>


namespace LLGL
{


class D3D11Buffer : public Buffer
{

    public:

        void SetName(const char* name) override;

        BufferDescriptor GetDesc() const override;

    public:

        D3D11Buffer(ID3D11Device* device, const BufferDescriptor& desc, const void* initialData = nullptr);

        void UpdateSubresource(ID3D11DeviceContext* context, const void* data, UINT dataSize, UINT offset);
        void ReadSubresource(ID3D11DeviceContext* context, void* data, UINT dataSize, UINT offset);

        void* Map(ID3D11DeviceContext* context, const CPUAccess access);
        void* Map(ID3D11DeviceContext* context, const CPUAccess access, UINT offset, UINT size);
        void Unmap(ID3D11DeviceContext* context);

        // Creates a shader-resource-view (SRV) of a subresource of this buffer object.
        void CreateSubresourceSRV(
            ID3D11Device*               device,
            ID3D11ShaderResourceView**  srvOutput,
            const DXGI_FORMAT           format,
            UINT                        firstElement,
            UINT                        numElements,
            bool                        isRawView       = false
        );

        // Returns the native ID3D11Buffer object.
        inline ID3D11Buffer* GetNative() const
        {
            return buffer_.Get();
        }

        // Returns the buffer size (in bytes).
        inline UINT GetSize() const
        {
            return size_;
        }

        // Returns the buffer stride (e.g. vertex stride).
        inline UINT GetStride() const
        {
            return stride_;
        }

        // Returns the native buffer format (i.e. format of index buffer or typed buffer).
        inline DXGI_FORMAT GetDXFormat() const
        {
            return format_;
        }

        // Returns the native usage type.
        inline D3D11_USAGE GetDXUsage() const
        {
            return usage_;
        }

    private:

        void CreateGpuBuffer(ID3D11Device* device, const BufferDescriptor& desc, const void* initialData);
        void CreateCpuAccessBuffer(ID3D11Device* device, const BufferDescriptor& desc);

        void ReadFromStagingBuffer(
            ID3D11DeviceContext*    context,
            ID3D11Buffer*           stagingBuffer,
            UINT                    stagingBufferOffset,
            void*                   data,
            UINT                    dataSize,
            UINT                    srcOffset
        );

        void ReadFromSubresourceCopyWithCpuAccess(
            ID3D11DeviceContext*    context,
            void*                   data,
            UINT                    dataSize,
            UINT                    srcOffset
        );

        D3D11_MAP GetCPUAccessTypeForUsage(const CPUAccess access) const;

    private:

        ComPtr<ID3D11Buffer>    buffer_;
        ComPtr<ID3D11Buffer>    cpuAccessBuffer_;

        UINT                    size_                   = 0;
        UINT                    stride_                 = 0;
        DXGI_FORMAT             format_                 = DXGI_FORMAT_UNKNOWN;
        D3D11_USAGE             usage_                  = D3D11_USAGE_DEFAULT;

        UINT                    mappedWriteRange_[2]    = { 0, 0 };

};


} // /namespace LLGL


#endif



// ================================================================================
