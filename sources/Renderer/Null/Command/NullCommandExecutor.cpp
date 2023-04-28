/*
 * NullCommandExecutor.cpp
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "NullCommandExecutor.h"
#include "NullCommand.h"

#include "../Texture/NullTexture.h"
#include "../Texture/NullSampler.h"
#include "../Texture/NullRenderTarget.h"

#include "../Buffer/NullBuffer.h"

#include "../RenderState/NullPipelineState.h"
#include "../RenderState/NullResourceHeap.h"
#include "../RenderState/NullRenderPass.h"
#include "../RenderState/NullQueryHeap.h"


namespace LLGL
{


static std::size_t ExecuteNullCommand(const NullOpcode opcode, const void* pc)
{
    switch (opcode)
    {
        case NullOpcodeBufferWrite:
        {
            auto cmd = reinterpret_cast<const NullCmdBufferWrite*>(pc);
            cmd->buffer->Write(cmd->offset, cmd + 1, cmd->size);
            return (sizeof(*cmd) + cmd->size);
        }
        case NullOpcodeCopySubresource:
        {
            auto cmd = reinterpret_cast<const NullCmdCopySubresource*>(pc);
            //TODO
            return sizeof(*cmd);
        }
        case NullOpcodeGenerateMips:
        {
            auto cmd = reinterpret_cast<const NullCmdGenerateMips*>(pc);
            const TextureSubresource subresource{ cmd->baseArrayLayer, cmd->numArrayLayers, cmd->baseMipLevel, cmd->numMipLevels };
            cmd->texture->GenerateMips(&subresource);
            return sizeof(*cmd);
        }
        //TODO...
        case NullOpcodeDraw:
        {
            auto cmd = reinterpret_cast<const NullCmdDraw*>(pc);
            //TODO
            return (sizeof(*cmd) + cmd->numVertexBuffers * sizeof(const NullBuffer*));
        }
        case NullOpcodeDrawIndexed:
        {
            auto cmd = reinterpret_cast<const NullCmdDrawIndexed*>(pc);
            //TODO
            return (sizeof(*cmd) + cmd->numVertexBuffers * sizeof(const NullBuffer*));
        }
        case NullOpcodePushDebugGroup:
        {
            auto cmd = reinterpret_cast<const NullCmdPushDebugGroup*>(pc);
            //TODO
            return (sizeof(*cmd) + cmd->length + 1);
        }
        case NullOpcodePopDebugGroup:
        {
            //TODO
            return 0;
        }
        default:
            return 0;
    }
}

void ExecuteNullVirtualCommandBuffer(const NullVirtualCommandBuffer& virtualCmdBuffer)
{
    /* Initialize program counter to execute virtual GL commands */
    for (const auto& chunk : virtualCmdBuffer)
    {
        auto pc     = chunk.data;
        auto pcEnd  = chunk.data + chunk.size;

        while (pc < pcEnd)
        {
            /* Read opcode */
            const NullOpcode opcode = *reinterpret_cast<const NullOpcode*>(pc);
            pc += sizeof(NullOpcode);

            /* Execute command and increment program counter */
            pc += ExecuteNullCommand(opcode, pc);
        }
    }
}


} // /namespace LLGL



// ================================================================================
