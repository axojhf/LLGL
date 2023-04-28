/*
 * WindowFlags.h
 * 
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_WINDOW_FLAGS_H
#define LLGL_WINDOW_FLAGS_H


#include <LLGL/Types.h>
#include <LLGL/Constants.h>
#include <LLGL/Container/Strings.h>
#include <cstdint>


namespace LLGL
{


//! Window descriptor structure.
struct WindowDescriptor
{
    //! Window title as unicode string.
    UTF8String      title;

    //! Window position (relative to the client area).
    Offset2D        position;

    //! Window size (this should be the client area size).
    Extent2D        size;

    //! Specifies whether the window is visible at creation time. By default false.
    bool            visible             = false;

    //! Specifies whether the window is borderless. This is required for a fullscreen swap-chain. By default false.
    bool            borderless          = false;

    /**
    \brief Specifies whether the window can be resized. By default false.
    \remarks For every window representing the surface for a SwapChain which has been resized,
    the video mode of that SwapChain must be updated with the resolution of the surface's content size.
    This can be done by setting the video mode with the new resolution before the respective swap-chain is bound as render target,
    or it can be handled by a window event listener on the 'OnResize' callback:
    \code
    // Alternative 1
    class MyEventListener : public LLGL::Window::EventListener {
        void OnResize(Window& sender, const Extent2D& clientAreaSize) override {
            mySwapChain->ResizeBuffers(clientAreaSize);
        }
    };
    myWindow->AddEventListener(std::make_shared<MyEventListener>());

    // Alternative 2
    mySwapChain->ResizeBuffers(myWindow->GetContentSize());
    myCmdBuffer->BeginRenderPass(*mySwapChain);
    \endcode
    \note Not updating the swap-chain on a resized window is undefined behavior.
    \see Surface::GetContentSize
    \see Window::EventListener::OnResize
    */
    bool            resizable           = false;

    /**
    \brief Specifies whether the window allows that files can be draged-and-droped onto the window. By default false.
    \note Only supported on: MS/Windows.
    */
    bool            acceptDropFiles     = false;

    //! Specifies whether the window is centered within the desktop screen. By default false.
    bool            centered            = false;

    /**
    \brief Window context handle.
    \remarks If used, this must be casted from a platform specific structure:
    \code
    #include <LLGL/Platform/NativeHandle.h>
    //...
    LLGL::NativeContextHandle handle;
    //handle.parentWindow = ...
    windowDesc.windowContext = reinterpret_cast<const void*>(&handle);
    \endcode
    */
    const void*     windowContext       = nullptr;
};

/**
\brief Window behavior structure.
\see Window::SetBehavior
*/
struct WindowBehavior
{
    /**
    \brief Specifies whether to clear the content of the window when it is resized. By default false.
    \remarks This is used by Win32 to erase (\c WM_ERASEBKGND message) or keep the background on a window resize.
    If this is false, some kind of flickering during a window resize can be avoided.
    \note Only supported on: Win32.
    */
    bool            disableClearOnResize    = false;

    /**
    \brief Specifies an ID for a timer which will be activated when the window is moved or sized. By default invalidWindowTimerID.
    \remarks This is used by Win32 to set a timer during a window is moved or resized to make continous scene updates.
    Do not reset it during the 'OnTimer' event, otherwise a timer might be not be released correctly!
    \note Only supported on: Win32.
    \see Window::EventListener::OnTimer
    \see invalidWindowTimerID
    */
    std::uint32_t   moveAndResizeTimerID    = Constants::invalidTimerID;
};


} // /namespace LLGL


#endif



// ================================================================================
