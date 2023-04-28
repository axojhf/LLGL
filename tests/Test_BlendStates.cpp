/*
 * Test_BlendStates.cpp
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include <LLGL/LLGL.h>
#include <LLGL/Utils/Utility.h>
#include <LLGL/Utils/VertexFormat.h>
#include <Gauss/Gauss.h>
#include <memory>
#include <iostream>
#include <string>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


int main()
{
    try
    {
        // Load render system module
        LLGL::RendererConfigurationOpenGL rendererConfig;
        {
            rendererConfig.contextProfile = LLGL::OpenGLContextProfile::CoreProfile;
        }
        LLGL::RenderSystemDescriptor rendererDesc;
        {
            rendererDesc.moduleName         = "OpenGL";
            rendererDesc.rendererConfig     = &rendererConfig;
            rendererDesc.rendererConfigSize = sizeof(rendererConfig);
        }
        auto renderer = LLGL::RenderSystem::Load("OpenGL");

        // Create swap-chain
        LLGL::SwapChainDescriptor swapChainDesc;

        swapChainDesc.resolution = { 800, 600 };

        auto swapChain = renderer->CreateSwapChain(swapChainDesc);

        swapChain->SetVsyncInterval(1);

        // Setup window title
        auto& window = static_cast<LLGL::Window&>(swapChain->GetSurface());

        window.SetTitle("LLGL Test 10 ( " + std::string(renderer->GetName()) + " )");

        // Setup input controller
        LLGL::Input input{ window };

        window.Show();

        // Create vertex buffer
        struct Vertex
        {
            Gs::Vector2f        position;
            LLGL::ColorRGBAub   color;
        }
        vertices[] =
        {
            { { -0.5f, -0.5f }, { 255,   0,   0, 255 } },
            { { -0.5f,  0.5f }, {   0, 255,   0, 160 } },
            { {  0.5f, -0.5f }, { 255,   0, 255,  80 } },
            { {  0.5f,  0.5f }, {   0,   0, 255,   0 } },
        };

        LLGL::VertexFormat vertexFormat;
        vertexFormat.AppendAttribute({ "position", LLGL::Format::RG32Float });
        vertexFormat.AppendAttribute({ "color",    LLGL::Format::RGBA8UNorm });
        vertexFormat.SetStride(sizeof(Vertex));

        LLGL::BufferDescriptor vertexBufferDesc;
        {
            vertexBufferDesc.size           = sizeof(vertices);
            vertexBufferDesc.bindFlags      = LLGL::BindFlags::VertexBuffer;
            vertexBufferDesc.vertexAttribs  = vertexFormat.attributes;
        }
        auto vertexBuffer = renderer->CreateBuffer(vertexBufferDesc, vertices);

        // Create shader program
        LLGL::ShaderDescriptor vertexShaderDesc{ LLGL::ShaderType::Vertex,   "Shaders/BlendTest.vert" };
        vertexShaderDesc.vertex.inputAttribs = vertexFormat.attributes;

        auto vertexShader      = renderer->CreateShader(vertexShaderDesc);//{ LLGL::ShaderType::Vertex,   "Shaders/BlendTest.vert" });
        auto fragmentShader    = renderer->CreateShader({ LLGL::ShaderType::Fragment, "Shaders/BlendTest.frag" });

        // Create graphics pipeline
        static const std::size_t numPipelines = 4;

        LLGL::PipelineState* pipeline[numPipelines] = {};

        LLGL::GraphicsPipelineDescriptor pipelineDesc;
        {
            pipelineDesc.vertexShader       = vertexShader;
            pipelineDesc.fragmentShader     = fragmentShader;
            pipelineDesc.primitiveTopology  = LLGL::PrimitiveTopology::TriangleStrip;
        }
        pipeline[0] = renderer->CreatePipelineState(pipelineDesc);

        {
            pipelineDesc.blend.targets[0].blendEnabled = true;
        }
        pipeline[1] = renderer->CreatePipelineState(pipelineDesc);

        {
            pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::LineStrip;
        }
        pipeline[2] = renderer->CreatePipelineState(pipelineDesc);

        {
            pipelineDesc.blend.targets[0].blendEnabled  = false;
            pipelineDesc.blend.targets[0].colorMask     = 0x0;
        }
        pipeline[3] = renderer->CreatePipelineState(pipelineDesc);

        for (auto p : pipeline)
        {
            if (auto report = p->GetReport())
            {
                if (report->HasErrors())
                    throw std::runtime_error(report->GetText());
            }
        }

        // Create command buffer
        auto commandQueue = renderer->GetCommandQueue();
        auto commands = renderer->CreateCommandBuffer();

        // Scene parameters
        std::size_t pipelineIndex = 0;

        const auto& resolution = swapChain->GetResolution();
        auto w = resolution.width / 2;
        auto h = resolution.height / 2;
        auto x = static_cast<std::int32_t>(w);
        auto y = static_cast<std::int32_t>(h);

        // Main loop
        while (window.ProcessEvents() && !input.KeyDown(LLGL::Key::Escape))
        {
            // User input
            if (input.KeyDownRepeated(LLGL::Key::Tab))
            {
                if (input.KeyPressed(LLGL::Key::Shift))
                {
                    if (pipelineIndex == 0)
                        pipelineIndex = numPipelines - 1;
                    else
                        pipelineIndex = (pipelineIndex - 1);
                }
                else
                    pipelineIndex = (pipelineIndex + 1) % numPipelines;
            }

            // Update scene
            static float angle;
            angle += 0.1f;

            auto dx = static_cast<int>(std::sin(angle) * 10.0f);
            auto dy = static_cast<int>(std::cos(angle) * 10.0f);

            LLGL::Viewport viewports[4] =
            {
                { { 0, 0 }, { w, h } },
                { { x, 0 }, { w, h } },
                { { x + dx, y + dy }, { w, h } },
                { { 0, y }, { w, h } },
            };

            // Render scene
            commands->Begin();
            {
                commands->SetVertexBuffer(*vertexBuffer);
                commands->BeginRenderPass(*swapChain);
                {
                    commands->Clear(LLGL::ClearFlags::Color);
                    for (int i = 0; i < 4; ++i)
                    {
                        commands->SetViewport(viewports[i]);
                        commands->SetPipelineState(*pipeline[i]);
                        commands->Draw(4, 0);
                    }
                }
                commands->EndRenderPass();
            }
            commands->End();
            commandQueue->Submit(*commands);

            swapChain->Present();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        #ifdef _WIN32
        system("pause");
        #endif
    }

    return 0;
}
