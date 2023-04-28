/*
 * Blob.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_BLOB_H
#define LLGL_BLOB_H


#include <LLGL/NonCopyable.h>
#include <LLGL/ImageFlags.h>
#include <memory>
#include <vector>
#include <string>


namespace LLGL
{


/**
\brief CPU read-only buffer of arbitrary size.
\see RenderSystem::CreatePipelineState
*/
class LLGL_EXPORT Blob : public NonCopyable
{

    public:

        /**
        \brief Creates a new Blob instance with a copy of the specified data.
        \param[in] data Pointer to the data that is to be copied and managed by this blob.
        \param[in] size Specifies the size (in bytes) of the data.
        \return New instance of Blob that manages the memory that is copied.
        */
        static std::unique_ptr<Blob> CreateCopy(const void* data, std::size_t size);

        /**
        \brief Creates a new Blob instance with a weak reference to the specified data.
        \param[in] data Pointer to the data that is to be referenced. This pointer must remain valid for the lifetime of this Blob instance.
        \param[in] size Specifies the size (in bytes) of the data.
        \return New instance of Blob that refers to the specified memory.
        */
        static std::unique_ptr<Blob> CreateWeakRef(const void* data, std::size_t size);

        /**
        \brief Creates a new Blob instance with a strong reference to the specified vector container.
        \param[in] container Specifies the container whose data is to be moved into this Blob instance.
        \return New instance of Blob that manages the specified container.
        */
        static std::unique_ptr<Blob> CreateStrongRef(std::vector<std::int8_t>&& container);

        /**
        \brief Creates a new Blob instance with a strong reference to the specified string container.
        \param[in] container Specifies the container whose data is to be moved into this Blob instance.
        \return New instance of Blob that manages the specified container.
        */
        static std::unique_ptr<Blob> CreateStrongRef(std::string&& container);

        /**
        \brief Creates a new Blob instance with the data read from the specified binary file.
        \param[in] filename Specifies the file that is to be read.
        \return New instance of Blob that manages the memory of a conent copy from the specified file or null if the file could not be read.
        */
        static std::unique_ptr<Blob> CreateFromFile(const char* filename);

        /**
        \brief Creates a new Blob instance with the data read from the specified binary file.
        \param[in] filename Specifies the file that is to be read.
        \return New instance of Blob that manages the memory of a conent copy from the specified file or null if the file could not be read.
        */
        static std::unique_ptr<Blob> CreateFromFile(const std::string& filename);

    public:

        //! Returns a constant pointer to the internal buffer.
        virtual const void* GetData() const = 0;

        //! Returns the size (in bytes) of the internal buffer.
        virtual std::size_t GetSize() const = 0;


};


} // /namespace LLGL


#endif



// ================================================================================
