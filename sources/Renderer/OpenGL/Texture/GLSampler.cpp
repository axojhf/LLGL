/*
 * GLSampler.cpp
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "GLSampler.h"
#include "../GLTypes.h"
#include "../GLObjectUtils.h"
#include "../Ext/GLExtensions.h"
#include "../RenderState/GLStateManager.h"


namespace LLGL
{


GLSampler::GLSampler()
{
    glGenSamplers(1, &id_);
}

GLSampler::~GLSampler()
{
    glDeleteSamplers(1, &id_);
    GLStateManager::Get().NotifySamplerRelease(id_);
}

void GLSampler::SetName(const char* name)
{
    GLSetObjectLabel(GL_SAMPLER, GetID(), name);
}

static GLenum GetGLSamplerMinFilter(const SamplerDescriptor& desc)
{
    if (desc.mipMapEnabled)
        return GLTypes::Map(desc.minFilter, desc.mipMapFilter);
    else
        return GLTypes::Map(desc.minFilter);
}

void GLSampler::SamplerParameters(const SamplerDescriptor& desc)
{
    /* Set texture coordinate wrap modes */
    glSamplerParameteri(id_, GL_TEXTURE_WRAP_S, GLTypes::Map(desc.addressModeU));
    glSamplerParameteri(id_, GL_TEXTURE_WRAP_T, GLTypes::Map(desc.addressModeV));
    glSamplerParameteri(id_, GL_TEXTURE_WRAP_R, GLTypes::Map(desc.addressModeW));

    /* Set filter states */
    glSamplerParameteri(id_, GL_TEXTURE_MIN_FILTER, GetGLSamplerMinFilter(desc));
    glSamplerParameteri(id_, GL_TEXTURE_MAG_FILTER, GLTypes::Map(desc.magFilter));
    #ifdef LLGL_OPENGL
    glSamplerParameterf(id_, GL_TEXTURE_MAX_ANISOTROPY_EXT, static_cast<float>(desc.maxAnisotropy));
    #endif

    /* Set MIP-map level selection */
    glSamplerParameterf(id_, GL_TEXTURE_MIN_LOD, desc.minLOD);
    glSamplerParameterf(id_, GL_TEXTURE_MAX_LOD, desc.maxLOD);
    #ifdef LLGL_OPENGL
    glSamplerParameterf(id_, GL_TEXTURE_LOD_BIAS, desc.mipMapLODBias);
    #endif

    /* Set compare operation */
    if (desc.compareEnabled)
    {
        glSamplerParameteri(id_, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(id_, GL_TEXTURE_COMPARE_FUNC, GLTypes::Map(desc.compareOp));
    }
    else
        glSamplerParameteri(id_, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    /* Set border color */
    #ifdef LLGL_SAMPLER_BORDER_COLOR
    glSamplerParameterfv(id_, GL_TEXTURE_BORDER_COLOR, desc.borderColor);
    #endif
}


} // /namespace LLGL



// ================================================================================
