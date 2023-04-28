/*
 * GLProgramPipeline.cpp
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "GLProgramPipeline.h"
#include "GLShaderBindingLayout.h"
#include "GLSeparableShader.h"
#include "../Ext/GLExtensions.h"
#include "../Ext/GLExtensionRegistry.h"
#include "../RenderState/GLStateManager.h"
#include "../../../Core/BasicReport.h"
#include <LLGL/Utils/ForRange.h>


namespace LLGL
{


static GLuint GLCreateProgramPipeline()
{
    GLuint id = 0;
    #if defined GL_ARB_direct_state_access && defined LLGL_GL_ENABLE_DSA_EXT
    if (HasExtension(GLExt::ARB_direct_state_access))
    {
        glCreateProgramPipelines(1, &id);
    }
    else
    #endif
    {
        /* Generate new program pipeline and initialize to its default state via glBindProgramPipeline */
        glGenProgramPipelines(1, &id);
        GLStateManager::Get().BindProgramPipeline(id);
    }
    return id;
}

GLProgramPipeline::GLProgramPipeline(std::size_t numShaders, Shader* const* shaders) :
    GLShaderPipeline { GLCreateProgramPipeline() }
{
    UseProgramStages(numShaders, reinterpret_cast<GLSeparableShader* const*>(shaders));
}

GLProgramPipeline::~GLProgramPipeline()
{
    GLuint id = GetID();
    glDeleteProgramPipelines(1, &id);
    GLStateManager::Get().NotifyProgramPipelineRelease(this);
}

static GLbitfield ToGLShaderStageBit(ShaderType type)
{
    switch (type)
    {
        case ShaderType::Vertex:            return GL_VERTEX_SHADER_BIT;
        #if defined(GL_VERSION_4_0) || defined(GL_ES_VERSION_3_2)
        case ShaderType::TessControl:       return GL_TESS_CONTROL_SHADER_BIT;
        case ShaderType::TessEvaluation:    return GL_TESS_EVALUATION_SHADER_BIT;
        #endif
        #if defined(GL_VERSION_3_2) || defined(GL_ES_VERSION_3_2)
        case ShaderType::Geometry:          return GL_GEOMETRY_SHADER_BIT;
        #endif
        case ShaderType::Fragment:          return GL_FRAGMENT_SHADER_BIT;
        #if defined(GL_VERSION_4_3) || defined(GL_ES_VERSION_3_1)
        case ShaderType::Compute:           return GL_COMPUTE_SHADER_BIT;
        #endif
        default:                            return 0;
    }
}

void GLProgramPipeline::Bind(GLStateManager& stateMngr)
{
    stateMngr.BindProgramPipeline(GetID());
}

void GLProgramPipeline::BindResourceSlots(const GLShaderBindingLayout& bindingLayout)
{
    for_range(i, static_cast<std::size_t>(GetSignature().GetNumShaders()))
        separableShaders_[i]->BindResourceSlots(bindingLayout);
}

void GLProgramPipeline::QueryInfoLogs(BasicReport& report)
{
    bool hasErrors = false;
    std::string log;

    for_range(i, GetSignature().GetNumShaders())
        separableShaders_[i]->QueryInfoLog(log, hasErrors);

    report.Reset(std::move(log), hasErrors);
}


/*
 * ======= Private: =======
 */

void GLProgramPipeline::UseProgramStages(std::size_t numShaders, GLSeparableShader* const* shaders)
{
    for (std::size_t i = 0; i < numShaders; ++i)
    {
        auto separableShader = shaders[i];
        if (auto stage = ToGLShaderStageBit(separableShader->GetType()))
        {
            glUseProgramStages(GetID(), stage, separableShader->GetID());
            separableShaders_[i] = separableShader;
        }
    }
    BuildSignature(numShaders, reinterpret_cast<const Shader* const*>(shaders));
}


} // /namespace LLGL



// ================================================================================
