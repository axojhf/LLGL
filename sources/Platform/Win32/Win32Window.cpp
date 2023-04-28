/*
 * Win32Window.cpp
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "Win32Window.h"
#include "Win32WindowClass.h"
#include "../../Core/CoreUtils.h"
#include "../../Core/Assertion.h"
#include <LLGL/Platform/NativeHandle.h>
#include <LLGL/Platform/Platform.h>


namespace LLGL
{


/* ----- Internal structures ----- */

struct WindowAppearance
{
    DWORD       style   = 0;
    Offset2D    position;
    Extent2D    size;
};


/* ----- Internal functions ----- */

static void SetUserData(HWND wnd, void* userData)
{
    SetWindowLongPtr(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(userData));
}

// Queries window rectangular area by the specified client area size and window style.
static RECT GetClientArea(LONG width, LONG height, DWORD style)
{
    RECT rc;
    {
        rc.left     = 0;
        rc.top      = 0;
        rc.right    = width;
        rc.bottom   = height;
    }
    AdjustWindowRect(&rc, style, FALSE);
    return rc;
}

// Determines the Win32 window style for the specified descriptor.
static DWORD GetWindowStyle(const WindowDescriptor& desc)
{
    DWORD style = (WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    if (desc.windowContext != nullptr && reinterpret_cast<const NativeContextHandle*>(desc.windowContext)->parentWindow != 0)
        style |= WS_CHILD;
    else if (desc.borderless)
        style |= WS_POPUP;
    else
    {
        style |= (WS_SYSMENU | WS_MINIMIZEBOX | WS_CAPTION);
        if (desc.resizable)
            style |= (WS_SIZEBOX | WS_MAXIMIZEBOX);
    }

    if (desc.visible)
        style |= WS_VISIBLE;

    if (desc.acceptDropFiles)
        style |= WM_DROPFILES;

    return style;
}

static Offset2D GetScreenCenteredPosition(const Extent2D& size)
{
    return
    {
        GetSystemMetrics(SM_CXSCREEN)/2 - static_cast<int>(size.width/2),
        GetSystemMetrics(SM_CYSCREEN)/2 - static_cast<int>(size.height/2)
    };
}

static WindowAppearance GetWindowAppearance(const WindowDescriptor& desc)
{
    WindowAppearance appearance;

    /* Get window style and client area */
    appearance.style = GetWindowStyle(desc);

    auto rc = GetClientArea(
        static_cast<LONG>(desc.size.width),
        static_cast<LONG>(desc.size.height),
        appearance.style
    );

    /* Setup window size */
    appearance.size.width   = static_cast<std::uint32_t>(rc.right - rc.left);
    appearance.size.height  = static_cast<std::uint32_t>(rc.bottom - rc.top);

    /* Setup window position */
    appearance.position = (desc.centered ? GetScreenCenteredPosition(desc.size) : desc.position);

    if (desc.centered)
    {
        appearance.position.x += rc.left;
        appearance.position.y += rc.top;
    }

    return appearance;
}


/* ----- Win32Window class ----- */

std::unique_ptr<Window> Window::Create(const WindowDescriptor& desc)
{
    return MakeUnique<Win32Window>(desc);
}

Win32Window::Win32Window(const WindowDescriptor& desc) :
    contextHandle_ { 0                        },
    wnd_           { CreateWindowHandle(desc) }
{
}

Win32Window::~Win32Window()
{
    DestroyWindow(wnd_);
}

bool Win32Window::GetNativeHandle(void* nativeHandle, std::size_t nativeHandleSize) const
{
    if (nativeHandleSize == sizeof(NativeHandle))
    {
        auto handle = reinterpret_cast<NativeHandle*>(nativeHandle);
        handle->window = wnd_;
        return true;
    }
    return false;
}

void Win32Window::ResetPixelFormat()
{
    /* Destroy previous window handle and create a new one with current descriptor settings */
    auto desc = GetDesc();
    DestroyWindow(wnd_);
    wnd_ = CreateWindowHandle(desc);
}

Extent2D Win32Window::GetContentSize() const
{
    /* Return the size of the client area */
    return GetSize(true);
}

void Win32Window::SetPosition(const Offset2D& position)
{
    SetWindowPos(wnd_, HWND_TOP, position.x, position.y, 0, 0, (SWP_NOSIZE | SWP_NOZORDER));
}

Offset2D Win32Window::GetPosition() const
{
    RECT rc;
    GetWindowRect(wnd_, &rc);
    MapWindowPoints(HWND_DESKTOP, GetParent(wnd_), reinterpret_cast<LPPOINT>(&rc), 2);
    return { rc.left, rc.top };
}

void Win32Window::SetSize(const Extent2D& size, bool useClientArea)
{
    int cx, cy;

    if (useClientArea)
    {
        auto rc = GetClientArea(
            static_cast<LONG>(size.width),
            static_cast<LONG>(size.height),
            GetWindowLong(wnd_, GWL_STYLE)
        );
        cx = rc.right - rc.left;
        cy = rc.bottom - rc.top;
    }
    else
    {
        cx = static_cast<int>(size.width);
        cy = static_cast<int>(size.height);
    }

    SetWindowPos(wnd_, HWND_TOP, 0, 0, cx, cy, (SWP_NOMOVE | SWP_NOZORDER));
}

Extent2D Win32Window::GetSize(bool useClientArea) const
{
    if (useClientArea)
    {
        RECT rc;
        GetClientRect(wnd_, &rc);
        return
        {
            static_cast<std::uint32_t>(rc.right - rc.left),
            static_cast<std::uint32_t>(rc.bottom - rc.top)
        };
    }
    else
    {
        RECT rc;
        GetWindowRect(wnd_, &rc);
        return
        {
            static_cast<std::uint32_t>(rc.right - rc.left),
            static_cast<std::uint32_t>(rc.bottom - rc.top)
        };
    }
}

void Win32Window::SetTitle(const UTF8String& title)
{
    #ifdef UNICODE
    auto titleUTF16 = title.to_utf16();
    SetWindowText(wnd_, titleUTF16.data());
    #else
    SetWindowText(wnd_, title.c_str());
    #endif
}

UTF8String Win32Window::GetTitle() const
{
    /* Retrieve window title and return as immutable string */
    int len = GetWindowTextLength(wnd_);
    if (len > 0)
    {
        auto title = MakeUniqueArray<TCHAR>(len + 1);
        GetWindowText(wnd_, &title[0], len + 1);
        return UTF8String{ title.get() };
    }
    return {};
}

void Win32Window::Show(bool show)
{
    ShowWindow(wnd_, (show ? SW_NORMAL : SW_HIDE));
}

bool Win32Window::IsShown() const
{
    return (IsWindowVisible(wnd_) ? true : false);
}

WindowDescriptor Win32Window::GetDesc() const
{
    /* Get window flags and other information for comparision */
    auto windowFlags    = GetWindowLong(wnd_, GWL_STYLE);
    auto windowSize     = GetSize();
    auto centerPoint    = GetScreenCenteredPosition(windowSize);

    /* Setup window descriptor */
    WindowDescriptor desc;

    desc.title              = GetTitle();
    desc.position           = GetPosition();
    desc.size               = windowSize;

    desc.visible            = ((windowFlags & WS_VISIBLE  ) != 0);
    desc.borderless         = ((windowFlags & WS_CAPTION  ) == 0);
    desc.resizable          = ((windowFlags & WS_SIZEBOX  ) != 0);
    desc.acceptDropFiles    = ((windowFlags & WM_DROPFILES) != 0);
    desc.centered           = (centerPoint.x == desc.position.x && centerPoint.y == desc.position.y);

    desc.windowContext      = (contextHandle_.parentWindow != 0 ? (&contextHandle_) : nullptr);

    return desc;
}

void Win32Window::SetDesc(const WindowDescriptor& desc)
{
    /* Get current window flags */
    auto windowFlags = GetWindowLong(wnd_, GWL_STYLE);

    auto borderless = ((windowFlags & WS_CAPTION) == 0);
    auto resizable  = ((windowFlags & WS_SIZEBOX) != 0);

    /* Setup new window flags */
    auto newWindowFlags = GetWindowStyle(desc);

    if ((windowFlags & WS_MAXIMIZE) != 0)
        newWindowFlags |= WS_MAXIMIZE;
    if ((windowFlags & WS_MINIMIZE) != 0)
        newWindowFlags |= WS_MINIMIZE;

    auto flagsChanged = (windowFlags != newWindowFlags);

    /* Check if anything changed */
    auto position           = GetPosition();
    auto size               = GetSize();

    bool positionChanged    = (desc.position.x != position.x || desc.position.y != position.y);
    bool sizeChanged        = (desc.size.width != size.width || desc.size.height != size.height);

    if (flagsChanged || positionChanged || sizeChanged)
    {
        UINT flags = SWP_NOZORDER;

        if (flagsChanged)
        {
            /* Hide temporarily to avoid strange effects during frame change (if frame has changed) */
            ShowWindow(wnd_, SW_HIDE);

            /* Set new window style */
            SetWindowLongPtr(wnd_, GWL_STYLE, newWindowFlags);
            flags |= SWP_FRAMECHANGED;
        }

        /* Set new position and size */
        auto appearance = GetWindowAppearance(desc);

        if (desc.visible)
            flags |= SWP_SHOWWINDOW;

        if ((newWindowFlags & WS_MAXIMIZE) != 0)
            flags |= (SWP_NOSIZE | SWP_NOMOVE);

        if (borderless == desc.borderless && resizable == desc.resizable)
        {
            if (!positionChanged)
                flags |= SWP_NOMOVE;
            if (!sizeChanged)
                flags |= SWP_NOSIZE;
        }

        SetWindowPos(
            wnd_,
            0, // ignore, due to SWP_NOZORDER flag
            appearance.position.x,
            appearance.position.y,
            static_cast<int>(appearance.size.width),
            static_cast<int>(appearance.size.height),
            flags
        );
    }
}

void Win32Window::OnProcessEvents()
{
    /* Peek all queued messages */
    MSG message;
    while (PeekMessage(&message, wnd_, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}


/*
 * ======= Private: =======
 */

HWND Win32Window::CreateWindowHandle(const WindowDescriptor& desc)
{
    auto windowClass = Win32WindowClass::Instance();

    /* Get final window size */
    auto appearance = GetWindowAppearance(desc);

    /* Get parent window */
    HWND parentWnd = HWND_DESKTOP;

    if (auto nativeContext = reinterpret_cast<const NativeContextHandle*>(desc.windowContext))
    {
        if (nativeContext->parentWindow)
        {
            parentWnd = nativeContext->parentWindow;
            contextHandle_ = *nativeContext;
        }
    }

    #ifdef UNICODE
    auto title = desc.title.to_utf16();
    #else
    const auto& title = desc.title;
    #endif

    /* Create frame window object */
    HWND wnd = CreateWindow(
        windowClass->GetName(),
        title.data(),
        appearance.style,
        appearance.position.x,
        appearance.position.y,
        static_cast<int>(appearance.size.width),
        static_cast<int>(appearance.size.height),
        parentWnd,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );

    LLGL_ASSERT(wnd != nullptr, "failed to create Win32 window");

    #ifndef LLGL_ARCH_ARM
    /* Set additional flags */
    if (desc.acceptDropFiles)
        DragAcceptFiles(wnd, TRUE);
    #endif

    /* Set reference of this object to the window user-data */
    SetUserData(wnd, this);

    return wnd;
}


} // /namespace LLGL



// ================================================================================
