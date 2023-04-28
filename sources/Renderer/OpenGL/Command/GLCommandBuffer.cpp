/*
 * GLCommandBuffer.cpp
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "GLCommandBuffer.h"
#include "../RenderState/GLState.h"
#include "../RenderState/GLPipelineLayout.h"
#include "../RenderState/GLPipelineState.h"
#include "../RenderState/GLGraphicsPSO.h"
#include "../../CheckedCast.h"


namespace LLGL
{


void GLCommandBuffer::ResetRenderState()
{
    renderState_.boundPipelineLayout    = nullptr;
    renderState_.boundPipelineState     = nullptr;
}

void GLCommandBuffer::SetIndexFormat(bool indexType16Bits, std::uint64_t offset)
{
    /* Store new index buffer data in global render state */
    if (indexType16Bits)
    {
        renderState_.indexBufferDataType    = GL_UNSIGNED_SHORT;
        renderState_.indexBufferStride      = 2;
    }
    else
    {
        renderState_.indexBufferDataType    = GL_UNSIGNED_INT;
        renderState_.indexBufferStride      = 4;
    }
    renderState_.indexBufferOffset = static_cast<GLsizeiptr>(offset);
}

void GLCommandBuffer::SetPipelineRenderState(const GLPipelineState& pipelineStateGL)
{
    /* Store pipeline state and layout */
    renderState_.boundPipelineLayout    = pipelineStateGL.GetPipelineLayout();
    renderState_.boundPipelineState     = &pipelineStateGL;

    /* Store draw and primitive mode */
    if (pipelineStateGL.IsGraphicsPSO())
    {
        auto& graphicsPSO = LLGL_CAST(const GLGraphicsPSO&, pipelineStateGL);
        renderState_.drawMode       = graphicsPSO.GetDrawMode();
        renderState_.primitiveMode  = graphicsPSO.GetPrimitiveMode();
    }
}

/* ----- Extensions ----- */

void GLCommandBuffer::SetGraphicsAPIDependentState(const void* stateDesc, std::size_t stateDescSize)
{
    // dummy
}


} // /namespace LLGL



// ================================================================================
