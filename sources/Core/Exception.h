/*
 * Exception.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_EXCEPTION_H
#define LLGL_EXCEPTION_H


#include <LLGL/Export.h>
#include <LLGL/Container/StringView.h>
#include <string>


#define LLGL_VA_ARGS(...) \
    , ## __VA_ARGS__

#define LLGL_TRAP(FORMAT, ...) \
    LLGL::Trap(__FUNCTION__, (FORMAT) LLGL_VA_ARGS(__VA_ARGS__))

#define LLGL_TRAP_NOT_IMPLEMENTED(...) \
    LLGL::TrapNotImplemented(__FUNCTION__ LLGL_VA_ARGS(__VA_ARGS__))

#define LLGL_TRAP_FEATURE_NOT_SUPPORTED(FEATURE) \
    LLGL::TrapFeatureNotSupported(__FUNCTION__, FEATURE)


namespace LLGL
{


// Primary function to trap execution from an unrecoverable state. This might either throw an exception, abort execution, or break the debugger.
[[noreturn]]
LLGL_EXPORT void Trap(const char* origin, const char* format, ...);

// Throws an std::runtime_error exception with the message, that the specified assertion that failed.
[[noreturn]]
LLGL_EXPORT void TrapAssertionFailed(const char* origin, const char* expr, const char* details = nullptr, ...);

// Throws an std::runtime_error exception with the message, that the specified feature is not supported.
[[noreturn]]
LLGL_EXPORT void TrapFeatureNotSupported(const char* origin, const char* featureName);

// Throws an std::runtime_error exception with the message, that the specified rendering feature is not supported by the renderer (see RenderingFeatures).
[[noreturn]]
LLGL_EXPORT void TrapRenderingFeatureNotSupported(const char* origin, const char* featureName);

// Throws an std::runtime_error exception with the message, that the specified OpenGL extension is not supported.
[[noreturn]]
LLGL_EXPORT void TrapGLExtensionNotSupported(const char* origin, const char* extensionName, const char* useCase = nullptr);

// Throws an std::runtime_error exception with the message, that the specified Vulkan extension is not supported.
[[noreturn]]
LLGL_EXPORT void TrapVKExtensionNotSupported(const char* origin, const char* extensionName, const char* useCase = nullptr);

// Throws an std::runtime_error exception with the message, that the specified interface function is not implemented yet.
[[noreturn]]
LLGL_EXPORT void TrapNotImplemented(const char* origin, const char* useCase = nullptr);

// Throws an std::invalid_argument exception with the message, that a null pointer was passed.
[[noreturn]]
LLGL_EXPORT void TrapParamNullPointer(const char* origin, const char* paramName);

// Throws an std::out_of_range exception with the message, that a value has exceeded an upper bound, i.e. <value> is not in the half-open range [0, upperBound).
[[noreturn]]
LLGL_EXPORT void TrapParamExceededUpperBound(const char* origin, const char* paramName, int value, int upperBound);

// Throws an std::out_of_range exception with the message, that a value has exceeded its maximum, i.e. <value> is not in the closed range [0, maximum].
[[noreturn]]
LLGL_EXPORT void TrapParamExceededMaximum(const char* origin, const char* paramName, int value, int maximum);


} // /namespace LLGL


#endif



// ================================================================================
