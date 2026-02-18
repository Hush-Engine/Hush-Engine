/*! \file RenderGraphExample.cpp
    \brief Comprehensive example showing how to use the RenderGraph with construct-once, reuse-many pattern
    \author Alan Ramirez Herrera
    \date 2025-11-17
*/

#include "RenderGraph/RenderG.hpp"
#include <iostream>
#include <memory>

using namespace Hush::Exp::RenderGraph;

// ============================================================================
// Example Resource Types
// ============================================================================

/// Example texture resource that satisfies ResourceConcept
struct Texture
{
    struct Descriptor
    {
        uint32_t width = 1920;
        uint32_t height = 1080;
        uint32_t format = 0; // 0=RGBA8, 1=Depth, 2=RGBA16F, etc.
        std::string name;
    };

    void* gpuHandle = nullptr;

    void CreateResource(const Descriptor& desc, void* ctx)
    {
        std::cout << "  [GPU] Creating texture '" << desc.name
                  << "' (" << desc.width << "x" << desc.height << ")\n";
        // Actual GPU resource creation would happen here
        gpuHandle = reinterpret_cast<void*>(0xDEADBEEF);
    }

    void DestroyResource(const Descriptor& desc, void* ctx)
    {
        std::cout << "  [GPU] Destroying texture '" << desc.name << "'\n";
        gpuHandle = nullptr;
    }
};

/// Example buffer resource
struct Buffer
{
    struct Descriptor
    {
        uint64_t size = 0;
        uint32_t usage = 0; // Bitflags: 1=Uniform, 2=Storage, 4=Vertex, etc.
        std::string name;
    };

    void* gpuHandle = nullptr;

    void CreateResource(const Descriptor& desc, void* ctx)
    {
        std::cout << "  [GPU] Creating buffer '" << desc.name
                  << "' (" << desc.size << " bytes)\n";
        gpuHandle = reinterpret_cast<void*>(0xCAFEBABE);
    }

    void DestroyResource(const Descriptor& desc, void* ctx)
    {
        std::cout << "  [GPU] Destroying buffer '" << desc.name << "'\n";
        gpuHandle = nullptr;
    }
};

// ============================================================================
// Example Render Context (simplified)
// ============================================================================

struct MockRenderContext
{
    uint32_t frameNumber = 0;

    void BeginFrame()
    {
        std::cout << "\n>>> Beginning Frame " << frameNumber << " <<<\n";
    }

    void EndFrame()
    {
        std::cout << ">>> Ending Frame " << frameNumber << " <<<\n\n";
        frameNumber++;
    }
};

// ============================================================================
// Example Renderer with Construct-Once, Reuse-Many Pattern
// ============================================================================

class DeferredRenderer
{
public:
    DeferredRenderer()
    {
        std::cout << "=== Building Render Graph (ONE TIME ONLY) ===\n\n";
        BuildRenderGraph();
        std::cout << "=== Compiling Render Graph (ONE TIME ONLY) ===\n\n";
        m_renderGraph.Compile();
        std::cout << "=== Render Graph Ready for Reuse ===\n\n";
    }

    void RenderFrame()
    {
        m_renderContext.BeginFrame();

        // Execute the pre-compiled graph - NO RECOMPILATION!
        m_renderGraph.Execute(&m_renderContext);

        m_renderContext.EndFrame();
    }

    void RebuildIfNeeded()
    {
        if (m_renderGraph.IsDirty())
        {
            std::cout << "=== Recompiling Render Graph ===\n";
            m_renderGraph.Compile();
        }
    }

private:
    void BuildRenderGraph()
    {
        // ====================================================================
        // Pass 1: Shadow Map Generation (Graphics Queue)
        // ====================================================================
        struct ShadowPassData
        {
            ResourceId shadowMap;
            ResourceId lightViewBuffer;
        };

        m_renderGraph.AddPass<ShadowPassData>(
            EPassType::Graphics,
            "Shadow Map Pass",
            // Build callback - declares resources and dependencies
            [](RenderGraph::BuildContext& ctx, ShadowPassData& data)
            {
                std::cout << "  [Build] Shadow Map Pass\n";

                // Create shadow map texture
                data.shadowMap = ctx.Create<Texture>("Shadow Map", Texture::Descriptor{
                    .width = 2048,
                    .height = 2048,
                    .format = 1, // Depth format
                    .name = "ShadowMap"
                });

                // Create light view/projection buffer
                data.lightViewBuffer = ctx.Create<Buffer>("Light View Buffer", Buffer::Descriptor{
                    .size = 256,
                    .usage = 1, // Uniform buffer
                    .name = "LightViewBuffer"
                });
            },
            // Execute callback - performs actual rendering
            [](ShadowPassData& data, void* ctx)
            {
                auto* renderCtx = static_cast<MockRenderContext*>(ctx);
                std::cout << "  [Execute] Shadow Map Pass (Frame " << renderCtx->frameNumber << ")\n";
                std::cout << "    - Rendering scene from light's perspective\n";
                std::cout << "    - Output: Shadow depth map\n";
            }
        );

        // ====================================================================
        // Pass 2: GBuffer Generation (Graphics Queue)
        // ====================================================================
        struct GBufferPassData
        {
            ResourceId albedoRT;
            ResourceId normalRT;
            ResourceId depthRT;
            ResourceId materialRT;
        };

        m_renderGraph.AddPass<GBufferPassData>(
            EPassType::Graphics,
            "GBuffer Pass",
            [](RenderGraph::BuildContext& ctx, GBufferPassData& data)
            {
                std::cout << "  [Build] GBuffer Pass\n";

                data.albedoRT = ctx.Create<Texture>("Albedo", Texture::Descriptor{
                    .width = 1920,
                    .height = 1080,
                    .format = 0, // RGBA8
                    .name = "GBuffer_Albedo"
                });

                data.normalRT = ctx.Create<Texture>("Normals", Texture::Descriptor{
                    .width = 1920,
                    .height = 1080,
                    .format = 2, // RGBA16F
                    .name = "GBuffer_Normals"
                });

                data.depthRT = ctx.Create<Texture>("Depth", Texture::Descriptor{
                    .width = 1920,
                    .height = 1080,
                    .format = 1, // Depth32F
                    .name = "GBuffer_Depth"
                });

                data.materialRT = ctx.Create<Texture>("Material", Texture::Descriptor{
                    .width = 1920,
                    .height = 1080,
                    .format = 0, // RGBA8
                    .name = "GBuffer_Material"
                });

                // This pass never gets culled
                ctx.SetCullingMode(RenderPassNode::EPassCullingMode::NeverCull);
            },
            [](GBufferPassData& data, void* ctx)
            {
                auto* renderCtx = static_cast<MockRenderContext*>(ctx);
                std::cout << "  [Execute] GBuffer Pass (Frame " << renderCtx->frameNumber << ")\n";
                std::cout << "    - Rendering geometry to multiple render targets\n";
                std::cout << "    - Outputs: Albedo, Normals, Depth, Material\n";
            }
        );

        // ====================================================================
        // Pass 3: SSAO (Compute Queue - runs in parallel with other work!)
        // ====================================================================
        struct SSAOPassData
        {
            ResourceId depthInput;
            ResourceId normalInput;
            ResourceId ssaoOutput;
            ResourceId ssaoNoiseBuffer;
        };

        m_renderGraph.AddPass<SSAOPassData>(
            EPassType::Compute,
            "SSAO Pass",
            [](RenderGraph::BuildContext& ctx, SSAOPassData& data)
            {
                std::cout << "  [Build] SSAO Pass\n";

                // Read from GBuffer
                data.depthInput = ctx.Read(ResourceId{3}); // GBuffer depth
                data.normalInput = ctx.Read(ResourceId{2}); // GBuffer normals

                data.ssaoOutput = ctx.Create<Texture>("SSAO", Texture::Descriptor{
                    .width = 1920,
                    .height = 1080,
                    .format = 0,
                    .name = "SSAO_Output"
                });

                data.ssaoNoiseBuffer = ctx.Create<Buffer>("SSAO Kernel", Buffer::Descriptor{
                    .size = 1024,
                    .usage = 2, // Storage buffer
                    .name = "SSAO_Kernel"
                });
            },
            [](SSAOPassData& data, void* ctx)
            {
                auto* renderCtx = static_cast<MockRenderContext*>(ctx);
                std::cout << "  [Execute] SSAO Pass [COMPUTE QUEUE] (Frame " << renderCtx->frameNumber << ")\n";
                std::cout << "    - Computing ambient occlusion\n";
                std::cout << "    - NOTE: This runs in parallel with graphics work!\n";
            }
        );

        // ====================================================================
        // Pass 4: Deferred Lighting (Graphics Queue)
        // ====================================================================
        struct LightingPassData
        {
            ResourceId shadowMapInput;
            ResourceId albedoInput;
            ResourceId normalInput;
            ResourceId depthInput;
            ResourceId materialInput;
            ResourceId ssaoInput;
            ResourceId litSceneOutput;
        };

        m_renderGraph.AddPass<LightingPassData>(
            EPassType::Graphics,
            "Deferred Lighting Pass",
            [](RenderGraph::BuildContext& ctx, LightingPassData& data)
            {
                std::cout << "  [Build] Deferred Lighting Pass\n";

                // Read from previous passes
                data.shadowMapInput = ctx.Read(ResourceId{1}); // Shadow map
                data.albedoInput = ctx.Read(ResourceId{2});    // GBuffer albedo
                data.normalInput = ctx.Read(ResourceId{3});    // GBuffer normals
                data.depthInput = ctx.Read(ResourceId{4});     // GBuffer depth
                data.materialInput = ctx.Read(ResourceId{5});  // GBuffer material
                data.ssaoInput = ctx.Read(ResourceId{7});      // SSAO result

                data.litSceneOutput = ctx.Create<Texture>("Lit Scene", Texture::Descriptor{
                    .width = 1920,
                    .height = 1080,
                    .format = 2, // RGBA16F for HDR
                    .name = "LitScene"
                });
            },
            [](LightingPassData& data, void* ctx)
            {
                auto* renderCtx = static_cast<MockRenderContext*>(ctx);
                std::cout << "  [Execute] Deferred Lighting Pass (Frame " << renderCtx->frameNumber << ")\n";
                std::cout << "    - Computing lighting using GBuffer\n";
                std::cout << "    - Applying shadows and SSAO\n";
                std::cout << "    - Cross-queue sync with compute handled automatically!\n";
            }
        );

        // ====================================================================
        // Pass 5: Bloom Extraction (Compute Queue)
        // ====================================================================
        struct BloomExtractData
        {
            ResourceId hdrInput;
            ResourceId brightPixelsOutput;
        };

        m_renderGraph.AddPass<BloomExtractData>(
            EPassType::Compute,
            "Bloom Extract Pass",
            [](RenderGraph::BuildContext& ctx, BloomExtractData& data)
            {
                std::cout << "  [Build] Bloom Extract Pass\n";

                data.hdrInput = ctx.Read(ResourceId{9}); // Lit scene

                data.brightPixelsOutput = ctx.Create<Texture>("Bright Pixels", Texture::Descriptor{
                    .width = 960,
                    .height = 540,
                    .format = 2, // RGBA16F
                    .name = "BrightPixels"
                });
            },
            [](BloomExtractData& data, void* ctx)
            {
                auto* renderCtx = static_cast<MockRenderContext*>(ctx);
                std::cout << "  [Execute] Bloom Extract Pass [COMPUTE] (Frame " << renderCtx->frameNumber << ")\n";
                std::cout << "    - Extracting bright pixels for bloom\n";
            }
        );

        // ====================================================================
        // Pass 6: Post-Processing & Tone Mapping (Graphics Queue)
        // ====================================================================
        struct PostProcessData
        {
            ResourceId hdrInput;
            ResourceId bloomInput;
            ResourceId finalOutput;
        };

        m_renderGraph.AddPass<PostProcessData>(
            EPassType::Graphics,
            "Post-Process & Tone Mapping",
            [](RenderGraph::BuildContext& ctx, PostProcessData& data)
            {
                std::cout << "  [Build] Post-Process Pass\n";

                data.hdrInput = ctx.Read(ResourceId{9});   // Lit scene
                data.bloomInput = ctx.Read(ResourceId{10}); // Bloom

                data.finalOutput = ctx.Create<Texture>("Final Image", Texture::Descriptor{
                    .width = 1920,
                    .height = 1080,
                    .format = 0, // RGBA8 LDR
                    .name = "FinalImage"
                });
            },
            [](PostProcessData& data, void* ctx)
            {
                auto* renderCtx = static_cast<MockRenderContext*>(ctx);
                std::cout << "  [Execute] Post-Process Pass (Frame " << renderCtx->frameNumber << ")\n";
                std::cout << "    - Applying bloom\n";
                std::cout << "    - Tone mapping HDR -> LDR\n";
                std::cout << "    - Final output ready for presentation!\n";
            }
        );
    }

private:
    RenderGraph m_renderGraph;
    MockRenderContext m_renderContext;
};

// ============================================================================
// Main Entry Point
// ============================================================================

int main()
{
    // Create renderer - graph is built and compiled ONCE during construction
    DeferredRenderer renderer;

    // Render multiple frames - graph is reused without recompilation!
    for (int frame = 0; frame < 3; ++frame)
    {
        renderer.RenderFrame();
    }

    return 0;
}
