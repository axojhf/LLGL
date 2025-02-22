/*
 * LinuxGLContext.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_LINUX_GL_CONTEXT_H
#define LLGL_LINUX_GL_CONTEXT_H


#include "../GLContext.h"
#include "../../OpenGL.h"
#include <LLGL/RendererConfiguration.h>
#include <LLGL/Platform/NativeHandle.h>
#include <X11/Xlib.h>


namespace LLGL
{


// Implementation of the <GLContext> interface for GNU/Linux and wrapper for a native GLX context.
class LinuxGLContext : public GLContext
{

    public:

        LinuxGLContext(
            const GLPixelFormat&                pixelFormat,
            const RendererConfigurationOpenGL&  profile,
            Surface&                            surface,
            LinuxGLContext*                     sharedContext
        );
        ~LinuxGLContext();

        void Resize(const Extent2D& resolution) override;
        int GetSamples() const override;

    public:

        // Returns the native X11 <GLXContext> object.
        inline ::GLXContext GetGLXContext() const
        {
            return glc_;
        }

    private:

        bool SetSwapInterval(int interval) override;

    private:

        void CreateContext(
            const GLPixelFormat&                pixelFormat,
            const RendererConfigurationOpenGL&  profile,
            const NativeHandle&                 nativeHandle,
            LinuxGLContext*                     sharedContext
        );
        void DeleteContext();

        GLXContext CreateContextCoreProfile(GLXContext glcShared, int major, int minor, int depthBits, int stencilBits);
        GLXContext CreateContextCompatibilityProfile(XVisualInfo* visual, GLXContext glcShared);

    private:

        ::Display*      display_    = nullptr;
        ::GLXContext    glc_        = nullptr;
        int             samples_    = 1;

};


} // /namespace LLGL


#endif



// ================================================================================
