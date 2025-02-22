/*
 * NullShader.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_NULL_SHADER_H
#define LLGL_NULL_SHADER_H


#include <LLGL/Shader.h>
#include <string>


namespace LLGL
{


class NullShader final : public Shader
{

    public:

        void SetName(const char* name) override;
        const Report* GetReport() const override;
        bool Reflect(ShaderReflection& reflection) const override;

    public:

        NullShader(const ShaderDescriptor& desc);

    public:

        const ShaderDescriptor desc;

    public:

        std::string label_;

};


} // /namespace LLGL


#endif



// ================================================================================
