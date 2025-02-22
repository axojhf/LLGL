/*
 * MTShader.h
 * 
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_MT_SHADER_H
#define LLGL_MT_SHADER_H


#import <Metal/Metal.h>

#include <LLGL/Shader.h>
#include "../../../Core/BasicReport.h"


namespace LLGL
{


class MTShader final : public Shader
{

    public:

        MTShader(id<MTLDevice> device, const ShaderDescriptor& desc);
        ~MTShader();

        const Report* GetReport() const override;

        bool Reflect(ShaderReflection& reflection) const override;

    public:

        /*
        Returns true if the MTLFunction is a vertex shader with a valid patch type (i.e. other than MTLPatchTypeNone).
        Such a shader is used as a post-tessellation vertex shader in conjunction with a compute kernel.
        */
        bool IsPostTessellationVertex() const;

        // Returns the number of patch control points for a post-tessellation vertex shader or 0 if this is not a vertex shader.
        NSUInteger GetNumPatchControlPoints() const;

        // Returns the native MTLFunction object.
        inline id<MTLFunction> GetNative() const
        {
            return native_;
        }

        // Returns the MTLVertexDescriptor object for this shader program.
        inline MTLVertexDescriptor* GetMTLVertexDesc() const
        {
            return vertexDesc_;
        }

        // Returns the number of threads per thread-group for compute kernels.
        inline const MTLSize& GetNumThreadsPerGroup() const
        {
            return numThreadsPerGroup_;
        }

    private:

        bool Compile(id<MTLDevice> device, const ShaderDescriptor& shaderDesc);
        bool CompileSource(id<MTLDevice> device, const ShaderDescriptor& shaderDesc);
        bool CompileBinary(id<MTLDevice> device, const ShaderDescriptor& shaderDesc);

        void BuildInputLayout(std::size_t numVertexAttribs, const VertexAttribute* vertexAttribs);

        bool LoadFunction(const char* entryPoint);

        bool ReflectComputePipeline(ShaderReflection& reflection) const;

    private:

        id<MTLDevice>           device_             = nil;
        id<MTLLibrary>          library_            = nil;
        id<MTLFunction>         native_             = nil;

        BasicReport             report_;
        MTLSize                 numThreadsPerGroup_ = {};

        MTLVertexDescriptor*    vertexDesc_         = nullptr;

};


} // /namespace LLGL


#endif



// ================================================================================
