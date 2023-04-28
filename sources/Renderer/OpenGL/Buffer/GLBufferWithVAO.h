/*
 * GLBufferWithVAO.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_GL_BUFFER_WITH_VAO_H
#define LLGL_GL_BUFFER_WITH_VAO_H


#include "GLBuffer.h"
#include "GLVertexArrayObject.h"
#ifdef LLGL_GL_ENABLE_OPENGL2X
#   include "GL2XVertexArray.h"
#endif


namespace LLGL
{


class GLBufferWithVAO final : public GLBuffer
{

    public:

        GLBufferWithVAO(long bindFlags);

        void BuildVertexArray(std::size_t numVertexAttribs, const VertexAttribute* vertexAttribs);

        // Returns the ID of the vertex-array-object (VAO)
        inline GLuint GetVaoID() const
        {
            return vao_.GetID();
        }

        // Returns the list of vertex attributes.
        inline const std::vector<VertexAttribute>& GetVertexAttribs() const
        {
            return vertexAttribs_;
        }

        #ifdef LLGL_GL_ENABLE_OPENGL2X
        // Returns the GL 2.x compatible vertex-array emulator.
        inline const GL2XVertexArray& GetVertexArrayGL2X() const
        {
            return vertexArrayGL2X_;
        }
        #endif

    private:

        void BuildVertexArrayWithVAO();
        #ifdef LLGL_GL_ENABLE_OPENGL2X
        void BuildVertexArrayWithEmulator();
        #endif

    private:

        GLVertexArrayObject             vao_;
        std::vector<VertexAttribute>    vertexAttribs_;

        #ifdef LLGL_GL_ENABLE_OPENGL2X
        GL2XVertexArray                 vertexArrayGL2X_;
        #endif

};


} // /namespace LLGL


#endif



// ================================================================================
