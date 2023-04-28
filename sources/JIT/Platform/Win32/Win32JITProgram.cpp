/*
 * Win32JITProgram.cpp
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "Win32JITProgram.h"
#include "../../../Core/CoreUtils.h"
#include <cstdlib>
#include <stdexcept>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace LLGL
{


std::unique_ptr<JITProgram> JITProgram::Create(const void* code, std::size_t size)
{
    return MakeUnique<Win32JITProgram>(code, size);
}

Win32JITProgram::Win32JITProgram(const void* code, std::size_t size) :
    size_ { size }
{
    /* Allocate chunk of executable memory */
    addr_ = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (addr_ == 0)
        throw std::runtime_error("failed to allocate " + std::to_string(size) + " byte(s) of executable memory");

    /* Copy assembly code to executable memory space */
    ::memcpy(addr_, code, size);

    /* Make assembly buffer executable */
    DWORD oldProtect = 0;
    if (VirtualProtect(addr_, size, PAGE_EXECUTE_READ, &oldProtect) == 0)
        throw std::runtime_error("failed to change virtual memory protection");

    /* Set function pointer to executable memory address */
    SetEntryPoint(addr_);
}

Win32JITProgram::~Win32JITProgram()
{
    VirtualFree(addr_, 0, MEM_RELEASE);
}


} // /namespace LLGL



// ================================================================================
