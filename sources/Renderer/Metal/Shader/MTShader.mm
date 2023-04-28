/*
 * MTShader.mm
 * 
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "MTShader.h"
#include "../MTTypes.h"
#include <LLGL/Platform/Platform.h>
#include <LLGL/Utils/ForRange.h>
#include <cstring>
#include <set>


namespace LLGL
{


MTShader::MTShader(id<MTLDevice> device, const ShaderDescriptor& desc) :
    Shader  { desc.type },
    device_ { device    }
{
    if (Compile(device, desc))
    {
        /* Build vertex input layout */
        BuildInputLayout(desc.vertex.inputAttribs.size(), desc.vertex.inputAttribs.data());

        /* Store work group size for compute shaders */
        if (desc.type == ShaderType::Compute)
        {
            const auto& workGroupSize = desc.compute.workGroupSize;
            numThreadsPerGroup_ = MTLSizeMake(workGroupSize.width, workGroupSize.height, workGroupSize.depth);
        }
    }
}

MTShader::~MTShader()
{
    if (vertexDesc_)
        [vertexDesc_ release];
    if (native_)
        [native_ release];
    if (library_)
        [library_ release];
}

static MTLLanguageVersion GetMTLLanguageVersion(const ShaderDescriptor& desc)
{
    if (desc.profile != nullptr)
    {
        if (std::strcmp(desc.profile, "2.1") == 0)
            return MTLLanguageVersion2_1;
        if (std::strcmp(desc.profile, "2.0") == 0)
            return MTLLanguageVersion2_0;
        if (std::strcmp(desc.profile, "1.2") == 0)
            return MTLLanguageVersion1_2;
        if (std::strcmp(desc.profile, "1.1") == 0)
            return MTLLanguageVersion1_1;
        #ifdef LLGL_OS_IOS
        if (std::strcmp(desc.profile, "1.0") == 0)
            return MTLLanguageVersion1_0;
        #endif // /LLGL_OS_IOS
    }
    throw std::invalid_argument("invalid Metal shader version specified");
}

const Report* MTShader::GetReport() const
{
    return (report_ ? &report_ : nullptr);
}

bool MTShader::Reflect(ShaderReflection& reflection) const
{
    if (GetType() == ShaderType::Compute)
        return ReflectComputePipeline(reflection);
    else
        return false; // Metal shader reflection not supported for render pipeline
}

bool MTShader::IsPostTessellationVertex() const
{
    return (GetType() == ShaderType::Vertex && native_ != nil && [native_ patchType] != MTLPatchTypeNone);
}

NSUInteger MTShader::GetNumPatchControlPoints() const
{
    if (IsPostTessellationVertex())
        return [native_ patchControlPointCount];
    else
        return 0;
}


/*
 * ======= Private: =======
 */

bool MTShader::Compile(id<MTLDevice> device, const ShaderDescriptor& shaderDesc)
{
    if (IsShaderSourceCode(shaderDesc.sourceType))
        return CompileSource(device, shaderDesc);
    else
        return CompileBinary(device, shaderDesc);
}

static NSString* ToNSString(const char* s)
{
    return [[NSString alloc] initWithUTF8String:(s != nullptr ? s : "")];
}

static MTLCompileOptions* ToMTLCompileOptions(const ShaderDescriptor& shaderDesc)
{
    MTLCompileOptions* opt = [MTLCompileOptions alloc];

    [opt setLanguageVersion:GetMTLLanguageVersion(shaderDesc)];
    if ((shaderDesc.flags & (ShaderCompileFlags::OptimizationLevel1 | ShaderCompileFlags::OptimizationLevel2 | ShaderCompileFlags::OptimizationLevel3)) != 0)
        [opt setFastMathEnabled:YES];

    return opt;
}

bool MTShader::CompileSource(id<MTLDevice> device, const ShaderDescriptor& shaderDesc)
{
    /* Get source */
    NSString* sourceString = nil;

    if (shaderDesc.sourceType == ShaderSourceType::CodeFile)
    {
        NSString* filePath = [[NSString alloc] initWithUTF8String:shaderDesc.source];
        sourceString = [[NSString alloc] initWithContentsOfFile:filePath encoding:NSUTF8StringEncoding error:nil];
        [filePath release];
    }
    else
        sourceString = [[NSString alloc] initWithUTF8String:shaderDesc.source];

    if (sourceString == nil)
        throw std::runtime_error("cannot compile Metal shader without source");

    /* Convert entry point to NSString, and initialize shader compile options */
    MTLCompileOptions* opt = ToMTLCompileOptions(shaderDesc);

    /* Load shader library */
    NSError* error = [NSError alloc];

    library_ = [device
        newLibraryWithSource:   sourceString
        options:                opt
        error:                  &error
    ];

    [sourceString release];
    [opt release];

    /* Load shader function with entry point */
    const bool success = LoadFunction(shaderDesc.entryPoint);

    const StringView errorText = [[error localizedDescription] cStringUsingEncoding:NSUTF8StringEncoding];
    report_.Reset(errorText, !success);
    [error release];

    return success;
}

//TODO: this is untested!!!
bool MTShader::CompileBinary(id<MTLDevice> device, const ShaderDescriptor& shaderDesc)
{
    /* Get source */
    dispatch_data_t dispatchData = nil;
    NSData* source = nullptr;

    if (shaderDesc.source != nullptr)
    {
        if (shaderDesc.sourceType == ShaderSourceType::BinaryFile)
        {
            NSString* filePath = [[NSString alloc] initWithUTF8String:shaderDesc.source];
            source = [NSData dataWithContentsOfFile:filePath];
            dispatchData = dispatch_data_create([source bytes], [source length], nil, nil);
            [filePath release];
        }
        else if (shaderDesc.sourceSize > 0)
            dispatchData = dispatch_data_create(shaderDesc.source, shaderDesc.sourceSize, nil, nil);
    }

    if (dispatchData == nil)
        throw std::runtime_error("cannot compile Metal shader without source");

    /* Load shader library */
    NSError* error = [NSError alloc];

    library_ = [device
        newLibraryWithData: reinterpret_cast<dispatch_data_t>(dispatchData)
        error:              &error
    ];

    if (source != nullptr)
        [source release];

    [dispatchData release];

    /* Load shader function with entry point */
    const bool success = LoadFunction(shaderDesc.entryPoint);

    const StringView errorText = [[error localizedDescription] cStringUsingEncoding:NSUTF8StringEncoding];
    report_.Reset(errorText, !success);
    [error release];

    return success;
}

// Converts the vertex attribute to a Metal vertex buffer layout
static void Convert(MTLVertexBufferLayoutDescriptor* dst, const VertexAttribute& src, bool isPatchControlPoint)
{
    if (src.instanceDivisor > 0)
    {
        dst.stepFunction = MTLVertexStepFunctionPerInstance;
        dst.stepRate     = static_cast<NSUInteger>(src.instanceDivisor);
    }
    else
    {
        dst.stepFunction = (isPatchControlPoint ? MTLVertexStepFunctionPerPatchControlPoint : MTLVertexStepFunctionPerVertex);
        dst.stepRate     = 1;
    }
    dst.stride = static_cast<NSUInteger>(src.stride);
}

// Converts the vertex attribute to a Metal vertex attribute
static void Convert(MTLVertexAttributeDescriptor* dst, const VertexAttribute& src)
{
    dst.format      = MTTypes::ToMTLVertexFormat(src.format);
    dst.offset      = static_cast<NSUInteger>(src.offset);
    dst.bufferIndex = static_cast<NSUInteger>(src.slot);
}

void MTShader::BuildInputLayout(std::size_t numVertexAttribs, const VertexAttribute* vertexAttribs)
{
    if (numVertexAttribs == 0 || vertexAttribs == nullptr)
        return;

    /* Allocate new vertex descriptor */
    if (vertexDesc_)
        [vertexDesc_ release];
    vertexDesc_ = [[MTLVertexDescriptor alloc] init];

    /* If the patch type of the vertex function is not MTLPatchTypeNone, the vertex layout declares a patch control point */
    bool isPatchControlPoint = (native_ != nil && [native_ patchType] != MTLPatchTypeNone);

    /* Convert vertex attributes to Metal vertex buffer layouts and attribute descriptors */
    std::set<std::uint32_t> slotOccupied;

    for_range(i, numVertexAttribs)
    {
        const auto& attr = vertexAttribs[i];

        auto occupied = slotOccupied.insert(attr.slot);
        if (occupied.second)
            Convert(vertexDesc_.layouts[attr.slot], attr, isPatchControlPoint);

        Convert(vertexDesc_.attributes[attr.location], attr);
    }
}

bool MTShader::LoadFunction(const char* entryPoint)
{
    bool result = false;

    if (library_)
    {
        NSString* entryPointStr = ToNSString(entryPoint);

        /* Load shader function with entry point name */
        native_ = [library_ newFunctionWithName:entryPointStr];
        if (native_)
            result = true;

        [entryPointStr release];
    }

    return result;
}

static ResourceType ToResourceType(MTLArgumentType type)
{
    switch (type)
    {
        case MTLArgumentTypeBuffer:     return ResourceType::Buffer;
        case MTLArgumentTypeTexture:    return ResourceType::Texture;
        case MTLArgumentTypeSampler:    return ResourceType::Sampler;
        default:                        return ResourceType::Undefined;
    }
}

static long GetShaderArgumentBindFlags(MTLArgument* arg, const ResourceType resourceType)
{
    long bindFlags = 0;

    bool isRead     = (arg.access != MTLArgumentAccessWriteOnly);
    bool isWritten  = (arg.access != MTLArgumentAccessReadOnly);

    if (resourceType == ResourceType::Buffer || resourceType == ResourceType::Texture)
    {
        if (isRead)
            bindFlags |= BindFlags::Sampled;
        if (isWritten)
            bindFlags |= BindFlags::Storage;
    }

    return bindFlags;
}

static void ReflectShaderArgument(MTLArgument* arg, ShaderReflection& reflection, long stageFlags)
{
    auto resourceType = ToResourceType(arg.type);
    if (resourceType != ResourceType::Undefined)
    {
        ShaderResourceReflection resource;
        {
            resource.binding.name       = [arg.name UTF8String];
            resource.binding.type       = resourceType;
            resource.binding.bindFlags  = GetShaderArgumentBindFlags(arg, resourceType);
            resource.binding.stageFlags = stageFlags;
            resource.binding.slot       = static_cast<std::uint32_t>(arg.index);
            resource.binding.arraySize  = static_cast<std::uint32_t>(arg.arrayLength);
            if (resourceType == ResourceType::Buffer)
                resource.constantBufferSize = static_cast<std::uint32_t>(arg.bufferDataSize);
        }
        reflection.resources.push_back(resource);
    }
}

bool MTShader::ReflectComputePipeline(ShaderReflection& reflection) const
{
    /* Create temporary compute pipeline state to retrieve shader reflection */
    MTLAutoreleasedComputePipelineReflection psoReflect = nullptr;
    MTLPipelineOption opt = (MTLPipelineOptionArgumentInfo | MTLPipelineOptionBufferTypeInfo);

    id<MTLComputePipelineState> pso = [device_
        newComputePipelineStateWithFunction:    GetNative()
        options:                                opt
        reflection:                             &psoReflect
        error:                                  nullptr
    ];

    if (pso != nil)
    {
        for (MTLArgument* arg in psoReflect.arguments)
            ReflectShaderArgument(arg, reflection, StageFlags::ComputeStage);
        [pso release];
        return true;
    }

    return false;
}


} // /namespace LLGL



// ================================================================================
