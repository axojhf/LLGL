/*
 * Win32Module.cpp
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "Win32Module.h"
#include "../../Core/CoreUtils.h"
#include <stdexcept>


namespace LLGL
{


std::string Module::GetModuleFilename(const char* moduleName)
{
    /* Extend module name to Win32 dynamic link library name (DLL) */
    #ifdef __MINGW32__
    std::string s = "libLLGL_";
    #else
    std::string s = "LLGL_";
    #endif
    s += moduleName;
    #ifdef LLGL_DEBUG
    s += "D";
    #endif
    s += ".dll";
    return s;
}

// Call Win32 function 'LoadLibrary' but with dialog error messages disabled
static HMODULE LoadLibrarySafe(LPCSTR filename)
{
    /* Disable dialog error messages */
    UINT prevMode = SetErrorMode(0);
    SetErrorMode(prevMode | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    /* Load library */
    HMODULE module = LoadLibraryA(filename);

    /* Reset previous error mode */
    SetErrorMode(prevMode);

    return module;
}

bool Module::IsAvailable(const char* moduleFilename)
{
    /* Check if Win32 dynamic link library can be loaded properly */
    if (auto handle = LoadLibrarySafe(moduleFilename))
    {
        FreeLibrary(handle);
        return true;
    }
    return false;
}

std::unique_ptr<Module> Module::Load(const char* moduleFilename)
{
    return MakeUnique<Win32Module>(moduleFilename);
}

Win32Module::Win32Module(const char* moduleFilename)
{
    /* Open Win32 dynamic link library (DLL) */
    handle_ = LoadLibrarySafe(moduleFilename);

    /* Check if loading has failed */
    if (!handle_)
        throw std::runtime_error("failed to load dynamic link library (DLL): \"" + std::string(moduleFilename) + "\"");
}

Win32Module::~Win32Module()
{
    FreeLibrary(handle_);
}

void* Win32Module::LoadProcedure(const char* procedureName)
{
    /* Get procedure address from library module and return it as raw-pointer */
    auto procAddr = GetProcAddress(handle_, procedureName);
    return reinterpret_cast<void*>(procAddr);
}


} // /namespace LLGL



// ================================================================================
