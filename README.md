# Low Level Graphics Library (LLGL)

<p align="center">
    <a href="https://github.com/LukasBanana/LLGL/blob/master/LICENSE.txt"><img alt="License" src="https://img.shields.io/badge/license-BSD--3%20clause-blue.svg" /></a>
    <a href="https://app.codacy.com/gh/LukasBanana/LLGL/dashboard"><img src="https://api.codacy.com/project/badge/Grade/53e975cd13834e78bbb6120287a5f4d8"/></a>
    <a href="https://gitter.im/LLGL-Project/community"><img alt="Join the chat at https://gitter.im/LLGL-Project/community" src="https://badges.gitter.im/LLGL-Project/LLGL.svg" /></a>
</p>

<p align="center"><img src="docu/LLGL_Logo.png"/></p>


## Documentation

- **NOTE:** *This repository receives bug fixes only, but no major updates. Pull requests may still be accepted.*
- **Version**: 0.03 Beta (see [ChangeLog](docu/ChangeLog))
- [Getting Started with LLGL](docu/GettingStarted/Getting%20Started%20with%20LLGL.pdf) (PDF)
with Introduction, Hello Triangle Tutorial, and Extensibility Example with [GLFW](http://www.glfw.org/)
- [LLGL Reference Manual](docu/refman.pdf) (PDF)
- [LLGL Coding Conventions](docu/CodingConventions/Coding%20Conventions%20for%20LLGL.pdf) (PDF)
- [Examples and Tutorials for C++](examples/Cpp)
- [Examples for C#](examples/CSharp)


## Platform Support

| Platform | CI | D3D12 | D3D11 | Vulkan | OpenGL | OpenGLES 3 | Metal |
|----------|:--:|:-----:|:-----:|:------:|:------:|:----------:|:-----:|
| <img src="docu/Icons/windows.svg" height="20" /> Windows | [![Windows Build](https://ci.appveyor.com/api/projects/status/j09x8n07u3byfky0?svg=true)](https://ci.appveyor.com/project/LukasBanana/llgl) | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | N/A | N/A |
| <img src="docu/Icons/linux.svg" height="20" /> GNU/Linux | [![GNU/Linux Build Status](https://github.com/LukasBanana/LLGL/actions/workflows/ci_linux.yml/badge.svg)](https://github.com/LukasBanana/LLGL/actions/workflows/ci_linux.yml) | N/A | N/A | :heavy_check_mark: | :heavy_check_mark: | N/A | N/A |
| <img src="docu/Icons/macos.svg" height="20" /> macOS | [![macOS Build Status](https://github.com/LukasBanana/LLGL/actions/workflows/ci_macos.yml/badge.svg)](https://github.com/LukasBanana/LLGL/actions/workflows/ci_macos.yml) | N/A | N/A | N/A | :heavy_check_mark: | N/A | :heavy_check_mark: |
| <img src="docu/Icons/ios.svg" height="20" /> iOS | [![iOS Build Status](https://github.com/LukasBanana/LLGL/actions/workflows/ci_ios.yml/badge.svg)](https://github.com/LukasBanana/LLGL/actions/workflows/ci_ios.yml) | N/A | N/A | N/A | N/A | :heavy_multiplication_x: | :heavy_check_mark: |
| <img src="docu/Icons/android.svg" height="20" /> Android | | N/A | N/A | :heavy_multiplication_x: | N/A | :heavy_check_mark: | N/A |


## Build Notes

Build scripts are provided for [**CMake**]((https://cmake.org/)).

### Windows

[**Visual Studio 2015**](https://visualstudio.microsoft.com/) or later is required to build LLGL on Windows.

### macOS, iOS

[**Xcode 9**](https://developer.apple.com/xcode/) or later is required to build LLGL on macOS and iOS.

### GNU/Linux

The following development libraries are required to build LLGL on GNU/Linux:
- **X11**: `libx11-dev`
- **Xrandr**: `libxrandr-dev`

### Android

The [Android NDK](https://developer.android.com/ndk) with at least API level 21 is required.
The build script to generate project files is currently only supported on **GNU/Linux**
and requires [CMake 3.10](https://cmake.org/) or later and the [Code::Blocks](http://www.codeblocks.org/) IDE.

*This platform support is currently in an experimental state.*

## Installing LuaBridge (vcpkg)

Alternatively, you can build and install LLGL using [vcpkg](https://github.com/Microsoft/vcpkg/) dependency manager:

```
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh
    ./vcpkg integrate install
    ./vcpkg install llgl
```

The llgl port in vcpkg is kept up to date by Microsoft team members and community contributors. If the version is out of date, please [create an issue or pull request](https://github.com/Microsoft/vcpkg) on the vcpkg repository.

## Thin Abstraction Layer

```cpp
// Unified Interface:
CommandBuffer::DrawIndexed(std::uint32_t numIndices, std::uint32_t firstIndex);

// OpenGL Implementation:
void GLImmediateCommandBuffer::DrawIndexed(std::uint32_t numIndices, std::uint32_t firstIndex) {
    glDrawElements(GetDrawMode(), static_cast<GLsizei>(numIndices), GetIndexType(), GetIndicesOffset(firstIndex));
}

// Direct3D 11 Implementation
void D3D11CommandBuffer::DrawIndexed(std::uint32_t numIndices, std::uint32_t firstIndex) {
    FlushConstantsCache();
    context_->DrawIndexed(numIndices, firstIndex, 0);
}

// Direct3D 12 Implementation
void D3D12CommandBuffer::DrawIndexed(std::uint32_t numIndices, std::uint32_t firstIndex) {
    FlushGraphicsStagingDescriptorTables();
    commandList_->DrawIndexedInstanced(numIndices, 1, firstIndex, 0, 0);
}

// Vulkan Implementation
void VKCommandBuffer::DrawIndexed(std::uint32_t numIndices, std::uint32_t firstIndex) {
    FlushDescriptorCache();
    vkCmdDrawIndexed(commandBuffer_, numIndices, 1, firstIndex, 0, 0);
}

// Metal implementation
void MTCommandBuffer::DrawIndexed(std::uint32_t numIndices, std::uint32_t firstIndex) {
    if (numPatchControlPoints_ > 0) {
        [renderEncoder_
            drawIndexedPatches:             numPatchControlPoints_
            patchStart:                     static_cast<NSUInteger>(firstIndex) / numPatchControlPoints_
            patchCount:                     static_cast<NSUInteger>(numIndices) / numPatchControlPoints_
            patchIndexBuffer:               nil
            patchIndexBufferOffset:         0
            controlPointIndexBuffer:        indexBuffer_
            controlPointIndexBufferOffset:  indexTypeSize_ * (static_cast<NSUInteger>(firstIndex))
            instanceCount:                  1
            baseInstance:                   0
        ];
    } else {
        [renderEncoder_
            drawIndexedPrimitives:  primitiveType_
            indexCount:             static_cast<NSUInteger>(numIndices)
            indexType:              indexType_
            indexBuffer:            indexBuffer_
            indexBufferOffset:      indexTypeSize_ * static_cast<NSUInteger>(firstIndex)
        ];
    }
}
```


