/*
 * Assertion.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_ASSERTION_H
#define LLGL_ASSERTION_H


#include "Exception.h"


#define LLGL_ASSERT(EXPR, ...) \
    if (!(EXPR)) { LLGL::TrapAssertionFailed(__FUNCTION__, #EXPR LLGL_VA_ARGS(__VA_ARGS__)); }

#define LLGL_ASSERT_PTR(PARAM) \
    if (!(PARAM)) { LLGL::TrapParamNullPointer(__FUNCTION__, #PARAM); }

#define LLGL_ASSERT_UPPER_BOUND(PARAM, UPPER_BOUND) \
    if ((PARAM) >= (UPPER_BOUND)) { LLGL::TrapParamExceededUpperBound(__FUNCTION__, #PARAM, static_cast<int>(PARAM), static_cast<int>(UPPER_BOUND)); }

#define LLGL_ASSERT_RANGE(PARAM, MAXIMUM) \
    if ((PARAM) > (MAXIMUM)) { LLGL::TrapParamExceededMaximum(__FUNCTION__, #PARAM, static_cast<int>(PARAM), static_cast<int>(MAXIMUM)); }

#define LLGL_ASSERT_RENDERING_FEATURE_SUPPORT(FEATURE) \
    if (!GetRenderingCaps().features.FEATURE) { LLGL::TrapRenderingFeatureNotSupported(__FUNCTION__, #FEATURE); }


#endif



// ================================================================================
