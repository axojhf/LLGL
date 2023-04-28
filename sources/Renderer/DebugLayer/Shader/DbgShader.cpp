/*
 * DbgShader.cpp
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "DbgShader.h"
#include "../DbgCore.h"


namespace LLGL
{


DbgShader::DbgShader(Shader& instance, const ShaderDescriptor& desc) :
    Shader    { desc.type },
    instance  { instance  },
    desc      { desc      }
{
    if (GetType() == ShaderType::Vertex)
        QueryInstanceAndVertexIDs();
}

void DbgShader::SetName(const char* name)
{
    DbgSetObjectName(*this, name);
}

const Report* DbgShader::GetReport() const
{
    return instance.GetReport();
}

bool DbgShader::Reflect(ShaderReflection& reflection) const
{
    return instance.Reflect(reflection);
}

const char* DbgShader::GetVertexID() const
{
    return (vertexID_.empty() ? nullptr : vertexID_.c_str());
}

const char* DbgShader::GetInstanceID() const
{
    return (instanceID_.empty() ? nullptr : instanceID_.c_str());
}

bool DbgShader::IsCompiled() const
{
    if (auto report = instance.GetReport())
        return !report->HasErrors();
    else
        return true;
}


/*
 * ======= Private: =======
 */

void DbgShader::QueryInstanceAndVertexIDs()
{
    ShaderReflection reflect;
    #if 0 //TODO
    if (instance.Reflect(reflect))
    {
        for (const auto& attr : reflect.vertex.inputAttribs)
        {
            if (vertexID_.empty())
            {
                if (attr.systemValue == SystemValue::VertexID)
                    vertexID_ = attr.name;
            }
            if (instanceID_.empty())
            {
                if (attr.systemValue == SystemValue::InstanceID)
                    instanceID_ = attr.name;
            }
            if (!vertexID_.empty() && !instanceID_.empty())
                break;
        }
    }
    #endif
}


} // /namespace LLGL



// ================================================================================
