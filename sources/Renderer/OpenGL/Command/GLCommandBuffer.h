/*
 * GLCommandBuffer.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_GL_COMMAND_BUFFER_H
#define LLGL_GL_COMMAND_BUFFER_H


#include <LLGL/CommandBuffer.h>
#include "../OpenGL.h"
#include "../RenderState/GLPipelineState.h"
#include "../RenderState/GLState.h"


namespace LLGL
{


struct GLRenderState;

class GLCommandBuffer : public CommandBuffer
{

    public:

        // Returns true if this is an immediate command buffer, otherwise it is a deferred command buffer.
        virtual bool IsImmediateCmdBuffer() const = 0;

    public:

        /* ----- Extensions ----- */

        void SetGraphicsAPIDependentState(const void* stateDesc, std::size_t stateDescSize) override final;

    protected:

        // Resets the internal render state of this command buffer.
        void ResetRenderState();

        // Configures the attributes of 'renderState' for the type of index buffers.
        void SetIndexFormat(bool indexType16Bits, std::uint64_t offset);

        // Stores the render states for the specified PSO: Draw mode, primitive mode, binding layout.
        void SetPipelineRenderState(const GLPipelineState& pipelineStateGL);

    protected:

        // Returns the current render state.
        inline const GLRenderState& GetRenderState() const
        {
            return renderState_;
        }

        // Returns the draw mode for the glDraw* commands.
        inline GLenum GetDrawMode() const
        {
            return renderState_.drawMode;
        }

        // Returns the primitive mode for the glBeginTransformFeedback* commands.
        inline GLenum GetPrimitiveMode() const
        {
            return renderState_.primitiveMode;
        }

        // Returns the index data type for the glDraw* commands.
        inline GLenum GetIndexType() const
        {
            return renderState_.indexBufferDataType;
        }

        // Returns the indices offset as GLvoid pointer for the glDrawElements* commands.
        inline const GLvoid* GetIndicesOffset(std::uint32_t firstIndex) const
        {
            const GLintptr indices = (renderState_.indexBufferOffset + firstIndex * renderState_.indexBufferStride);
            return reinterpret_cast<const GLvoid*>(indices);
        }

        // Returns the currently bound pipeline layout.
        inline const GLPipelineLayout* GetBoundPipelineLayout() const
        {
            return renderState_.boundPipelineLayout;
        }

        // Returns the currently bound pipeline state.
        inline const GLPipelineState* GetBoundPipelineState() const
        {
            return renderState_.boundPipelineState;
        }

        // Returns the currently bound shader pipeline.
        inline const GLShaderPipeline* GetBoundShaderPipeline() const
        {
            auto* pipelineState = renderState_.boundPipelineState;
            return (pipelineState != nullptr ? pipelineState->GetShaderPipeline() : nullptr);
        }

    private:

        GLRenderState renderState_;

};


} // /namespace LLGL


#endif



// ================================================================================
