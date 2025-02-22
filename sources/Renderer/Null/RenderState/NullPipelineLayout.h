/*
 * NullPipelineLayout.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_NULL_PIPELINE_LAYOUT_H
#define LLGL_NULL_PIPELINE_LAYOUT_H


#include <LLGL/PipelineLayout.h>
#include <LLGL/PipelineLayoutFlags.h>
#include <string>


namespace LLGL
{


class NullPipelineLayout final : public PipelineLayout
{

    public:

        void SetName(const char* name) override;

        std::uint32_t GetNumHeapBindings() const override;
        std::uint32_t GetNumBindings() const override;
        std::uint32_t GetNumStaticSamplers() const override;
        std::uint32_t GetNumUniforms() const override;

    public:

        NullPipelineLayout(const PipelineLayoutDescriptor& desc);

    public:

        const PipelineLayoutDescriptor desc;

    private:

        std::string label_;

};


} // /namespace LLGL


#endif



// ================================================================================
