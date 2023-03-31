/*
 * MTPipelineState.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015-2019 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef LLGL_MT_PIPELINE_STATE_H
#define LLGL_MT_PIPELINE_STATE_H


#import <Metal/Metal.h>

#include <LLGL/PipelineState.h>
#include "MTDescriptorCache.h"
#include "../../../Core/BasicReport.h"
#include <memory>


namespace LLGL
{


class PipelineLayout;
class MTPipelineLayout;

class MTPipelineState : public PipelineState
{

    public:

        MTPipelineState(bool isGraphicsPSO, const PipelineLayout* pipelineLayout);

        const Report* GetReport() const override final;

        // Returns true if this is a graphics PSO.
        inline bool IsGraphicsPSO() const
        {
            return isGraphicsPSO_;
        }

        // Returns the descriptor cache for this PSO or null if there is none.
        inline MTDescriptorCache* GetDescriptorCache() const
        {
            return descriptorCache_.get();
        }

    protected:

        // Writes the report with the specified message and error bit.
        void ResetReport(std::string&& text, bool hasErrors = false);

        // Returns the pipeline layout this PSO was created with. May also be null.
        inline const MTPipelineLayout* GetPipelineLayout() const
        {
            return pipelineLayout_;
        }

    private:

        const bool                          isGraphicsPSO_      = false;
        const MTPipelineLayout*             pipelineLayout_     = nullptr;
        std::unique_ptr<MTDescriptorCache>  descriptorCache_;
        BasicReport                         report_;

};


} // /namespace LLGL


#endif



// ================================================================================
