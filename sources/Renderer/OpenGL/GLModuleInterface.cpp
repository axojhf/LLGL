/*
 * GLModuleInterface.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "../ModuleInterface.h"
#include "GLRenderSystem.h"
#include "GLProfile.h"


namespace LLGL
{


#ifdef LLGL_BUILD_RENDERER_OPENGLES3
#define ModuleOpenGL ModuleOpenGLES3
#endif

namespace ModuleOpenGL
{
    int GetRendererID()
    {
        return GLProfile::GetRendererID();
    }

    const char* GetModuleName()
    {
        return GLProfile::GetModuleName();
    }

    const char* GetRendererName()
    {
        return GLProfile::GetRendererName();
    }

    RenderSystem* AllocRenderSystem(const LLGL::RenderSystemDescriptor* renderSystemDesc)
    {
        return new GLRenderSystem(*renderSystemDesc);
    }
} // /namespace ModuleOpenGL


} // /namespace LLGL

#ifndef LLGL_BUILD_STATIC_LIB

extern "C"
{

LLGL_EXPORT int LLGL_RenderSystem_BuildID()
{
    return LLGL_BUILD_ID;
}

LLGL_EXPORT int LLGL_RenderSystem_RendererID()
{
    return LLGL::ModuleOpenGL::GetRendererID();
}

LLGL_EXPORT const char* LLGL_RenderSystem_Name()
{
    return LLGL::ModuleOpenGL::GetRendererName();
}

LLGL_EXPORT void* LLGL_RenderSystem_Alloc(const void* renderSystemDesc, int renderSystemDescSize)
{
    if (renderSystemDesc != nullptr && static_cast<std::size_t>(renderSystemDescSize) == sizeof(LLGL::RenderSystemDescriptor))
    {
        auto desc = reinterpret_cast<const LLGL::RenderSystemDescriptor*>(renderSystemDesc);
        return LLGL::ModuleOpenGL::AllocRenderSystem(desc);
    }
    return nullptr;
}

} // /extern "C"

#endif // /LLGL_BUILD_STATIC_LIB

#ifdef LLGL_BUILD_RENDERER_OPENGLES3
#undef ModuleOpenGL
#endif



// ================================================================================
