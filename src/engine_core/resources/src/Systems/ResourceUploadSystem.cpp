/*! \file ResourceUploadSystem.cpp
	\author Hush Engine
	\date 2026-02-17
	\brief ResourceUploadSystem implementation — uploads dirty mesh/texture
		   resources to the GPU via a render-graph Transfer pass.
*/

#include "ResourceUploadSystem.hpp"
#include "Assertions.hpp"
#include "Profiling.hpp"
#include "Components/GpuUploadComponent.hpp"
#include "Components/MeshReference.hpp"
#include "Components/RenderGraphBuilderComponent.hpp"
#include "Components/TextureComponent.hpp"
#include "Entity.hpp"
#include "Logger.hpp"
#include "RHI/GraphicsResources.hpp"
#include "RHI/GraphicsTypes.hpp"
#include "RHI/ICommandList.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "Scene.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

static constexpr uint64_t ROW_BYTE_ALIGNMENT = 256;

static constexpr uint16_t RENDER_GRAPH_SYSTEM_ORDER = 0;

Hush::Renderer::ResourceUploadSystem::ResourceUploadSystem(Hush::Scene &scene, RenderGraph::RenderDevice *renderDevice,
														   uint64_t stagingBufferSize)
	: ISystem(scene),
	  m_renderDevice(renderDevice),
	  m_graphicsDevice(renderDevice->GetGraphicsDevice()),
	  m_stagingBufferSize(stagingBufferSize),
	  m_halfQuota(stagingBufferSize / 2),
	  m_stagingBuffer(nullptr)
{
	HUSH_ASSERT(m_renderDevice != nullptr, "ResourceUploadSystem requires a valid RenderDevice!");
	HUSH_ASSERT(m_graphicsDevice != nullptr, "ResourceUploadSystem requires a valid IGraphicsDevice!");

	SetOrder(RENDER_GRAPH_SYSTEM_ORDER);
}

void Hush::Renderer::ResourceUploadSystem::Init()
{
	m_stagingBuffer = m_graphicsDevice->CreateBuffer(Graphics::BufferDescriptor{
		.size = m_stagingBufferSize,
		.usage = Graphics::EBufferUsage::CopySource,
		.memoryAccess = Graphics::EMemoryAccess::CPUWrite,
		.debugName = "ResourceUpload_StagingBuffer",
	});

	m_meshUploadQuery = GetScene().CreateQuery<GpuUploadComponent, MeshReference>();
	m_textureUploadQuery = GetScene().CreateQuery<GpuUploadComponent, Ref<TextureComponent>>();

	HUSH_ASSERT(m_stagingBuffer != nullptr, "Failed to create the staging buffer for ResourceUploadSystem!");

	Hush::Entity builderEntity = GetScene().CreateEntityWithName("ResourceUploadGraphBuilder");
	auto &builder = builderEntity.AddComponent<RenderGraph::RenderGraphBuilderComponent>();

	auto &scene = GetScene();

	scene.AddComponentObserver<Ref<TextureComponent>>(
		EComponentObserverType::Add, [this](Hush::Entity::EntityId entity, [[maybe_unused]]
																		   Ref<TextureComponent> *texComp) {
			LogFormat(Hush::ELogLevel::Trace,
					  "[ResourceUploadSystem] Detected new texture on entity %d, marking for upload.", entity);
			Hush::Entity entityRef = Hush::Entity(&GetScene(), entity);

			GpuUploadComponent uploadComponent = {
				.cpuOffset = 0,
				.uploadMode = GpuUploadComponent::EUploadMode::Full,
			};
			entityRef.EmplaceComponent<GpuUploadComponent>(uploadComponent);
		});

	builder.builderFunc = [this](Hush::RenderGraph::RenderGraph &graph) { this->BuildUploadPass(graph); };

	builder.frameUpdateFunc = [this](Hush::RenderGraph::RenderGraph &graph) { this->UpdateUploadPassImports(graph); };
}

void Hush::Renderer::ResourceUploadSystem::OnShutdown()
{
	m_meshStaging = {};
	m_textureStaging = {};

	if (m_stagingBuffer)
	{
		m_stagingBuffer->Unmap();
		m_stagingBuffer.reset();
	}
}

void Hush::Renderer::ResourceUploadSystem::OnUpdate([[maybe_unused]] float delta)
{
}

void Hush::Renderer::ResourceUploadSystem::OnFixedUpdate([[maybe_unused]] float delta)
{
}

void Hush::Renderer::ResourceUploadSystem::OnPreRender()
{
	ZoneScoped;

	// Staging-buffer mapping must happen before the frame's BeginFrame().
	// On Emscripten/Chromium, WebGPUBuffer::Map yields to the browser via
	// emscripten_sleep, and any task yield between
	// GPUCanvasContext.getCurrentTexture() and queue.submit() destroys
	// the canvas texture backing — producing a "Destroyed texture used
	// in a submit" validation error. Running in OnUpdate (which executes
	// before any system's OnPreRender) keeps the yield outside that
	// window.
	void *mapped = nullptr;
	{
		ZoneScopedN("MapStagingBuffer");
		mapped = m_stagingBuffer->Map(m_renderDevice->GetGraphicsDevice());
	}
	HUSH_ASSERT(mapped != nullptr, "Failed to map the staging buffer for CPU writes!");

	// First half → meshes
	m_meshStaging.cpuPtr = mapped;
	m_meshStaging.offset = 0;
	m_meshStaging.capacity = m_halfQuota;

	// Second half → textures
	m_textureStaging.cpuPtr = static_cast<char *>(mapped) + m_halfQuota;
	m_textureStaging.offset = 0;
	m_textureStaging.capacity = m_halfQuota;

	// Reset per-frame bookkeeping.
	m_bytesUploadedLastFrame = 0;
	m_meshStaging.Reset();
	m_textureStaging.Reset();
	m_pendingBufferCopies.clear();
	m_pendingTextureCopies.clear();

	// CPU-only staging: memcpy dirty data into the mapped buffer and
	// record pending copy descriptors.  No GPU calls happen here.
	{
		ZoneScopedN("StageDirtyMeshes");
		StageDirtyMeshes();
	}
	{
		ZoneScopedN("StageDirtyTextures");
		StageDirtyTextures();
	}

	m_bytesUploadedLastFrame = m_meshStaging.offset + m_textureStaging.offset;
}

void Hush::Renderer::ResourceUploadSystem::OnRender()
{
	// GPU work is handled by the render-graph Transfer pass.
}

void Hush::Renderer::ResourceUploadSystem::OnPostRender()
{
	// Remove the GpuUploadComponent from every entity that completed its
	// upload this frame so the query no longer picks them up next frame.
	for (auto &entity : m_entitiesToMarkAsUploaded)
	{
		GetScene().EntityFromIdUnchecked(entity).RemoveComponent<GpuUploadComponent>();
	}
	m_entitiesToMarkAsUploaded.clear();
}

void Hush::Renderer::ResourceUploadSystem::BuildUploadPass(RenderGraph::RenderGraph &graph)
{
	graph.AddPass<UploadPassData>(
		Hush::RenderGraph::EPassType::Transfer, "ResourceUploadPass",

		[this](Hush::RenderGraph::RenderGraph::BuildContext &ctx, UploadPassData &data) {
			// Import the staging buffer so the graph can track its state.
			data.stagingBufferResource = ctx.Import<Hush::Graphics::ImportedBufferResource>(
				"UploadStagingBuffer", Hush::Graphics::ImportedBufferResource{.buffer = this->m_stagingBuffer.get()},
				Hush::Graphics::EResourceState::CopySource);

			// Create a tiny transient resource that acts as a sync token.
			// Any pass that needs uploaded resources should Read this ID
			// in its own build callback to establish a dependency edge.
			data.syncToken = ctx.Create<Hush::Graphics::DummyResource>(
				RenderGraph::RenderGraph::RESOURCE_UPLOAD_SYNC_TOKEN_NAME, {});

			// Never cull this pass — even if nobody reads the sync token
			// yet, we still want the copies to happen.
			ctx.SetCullingMode(Hush::RenderGraph::RenderPassNode::EPassCullingMode::NeverCull);

			// Store the sync token ID so external code can depend on it.
			this->m_stagingBufferResourceId = data.stagingBufferResource;
			this->m_uploadSyncResourceId = data.syncToken;
		},

		[this]([[maybe_unused]] UploadPassData &data, Hush::Graphics::ICommandList *cmdList,
			   [[maybe_unused]]
			   const Hush::RenderGraph::ResourceManager &resourceManager) {
			auto *copyCmd =
				static_cast<Hush::Graphics::ICopyCommandList *>(cmdList); // NOLINT(*-pro-type-static-cast-downcast)

			Hush::Graphics::IGraphicsBuffer *staging = this->GetStagingBuffer();

			// Issue all buffer-to-buffer copies (mesh vertex/index data).
			for (const auto &copy : this->GetPendingBufferCopies())
			{
				copyCmd->CopyBuffer(staging, copy.stagingOffset, copy.destination, copy.destOffset, copy.size);
			}

			// Issue all buffer-to-texture copies (texture pixel data).
			for (const auto &texCopy : this->GetPendingTextureCopies())
			{
				copyCmd->CopyBufferToTexture(staging, texCopy.stagingOffset, texCopy.destination, texCopy.dstX,
											 texCopy.dstY, texCopy.dstZ, texCopy.width, texCopy.height, texCopy.depth,
											 texCopy.rowPitch);
			}

			// We are done with this, we must unmap the buffer
			m_stagingBuffer->Unmap();
		});
}

void Hush::Renderer::ResourceUploadSystem::UpdateUploadPassImports(RenderGraph::RenderGraph &graph)
{
	// Fast path: the graph topology hasn't changed, but the staging buffer
	// pointer might have been recreated (unlikely since we keep it alive).
	// Update the imported resource in-place so the executor sees the
	// current native handle.
	graph.UpdateImport<Graphics::ImportedBufferResource>(
		m_stagingBufferResourceId, Graphics::ImportedBufferResource{.buffer = m_stagingBuffer.get()});
}
void Hush::Renderer::ResourceUploadSystem::StageDirtyMeshes()
{
	size_t uploadCount = 0;
	bool shouldContinue = true;

	for (auto it = m_meshUploadQuery.begin();
		 it != m_meshUploadQuery.end() && uploadCount < MAX_RESOURCE_UPLOADS_PER_FRAME && shouldContinue; ++it)
	{
		auto [uploadStateSpan, meshRefSpan] = *it;

		for (uint32_t i = 0; i < it.Size(); ++i)
		{
			Hush::Entity entity = it.GetEntity(i);
			[[maybe_unused]]
			auto &uploadState = uploadStateSpan[i];
			auto &meshRef = meshRefSpan[i];

			// Skip entries with no mesh data — mark them as done so the
			// component is removed and we stop revisiting them.
			auto &mesh = meshRef.GetMesh();
			if (mesh.IsNull() || mesh->GetVertexBuffer().empty())
			{
				m_entitiesToMarkAsUploaded.push_back(entity.GetId());
				continue;
			}

			const auto &vertices = mesh->GetVertexBuffer();
			const auto &indices = mesh->GetIndexBuffer();

			const uint64_t vertexBytes = vertices.size() * sizeof(vertices[0]);
			const uint64_t indexBytes = indices.size() * sizeof(indices[0]);
			const uint64_t totalBytes = vertexBytes + indexBytes;

			// If the staging region is completely exhausted, stop processing
			// further entries this frame.
			if (m_meshStaging.Remaining() == 0)
			{
				shouldContinue = false;
				break;
			}

			// Mesh uploads are always full-mode: both buffers must fit in
			// the staging region in a single frame.
			if (totalBytes > m_meshStaging.Remaining())
			{
				// Not enough room this frame — defer to next frame.
				continue;
			}
			// ── Vertex data ──────────────────────────────────────────────
			char *dst = static_cast<char *>(m_meshStaging.cpuPtr) + m_meshStaging.offset;
			std::memcpy(dst, vertices.data(), vertexBytes);

			// The mesh staging region starts at byte 0 of the staging buffer.
			const uint64_t vertexStagingOffset = m_meshStaging.offset;

			// Create or reuse the GPU vertex buffer.
			if (meshRef.GetGpuVertexBuffer() == nullptr || meshRef.GetGpuVertexBuffer()->GetSize() < vertexBytes)
			{
				meshRef.SetGpuVertexBuffer(m_graphicsDevice->CreateBuffer(Graphics::BufferDescriptor{
					.size = vertexBytes,
					.usage = Graphics::EBufferUsage::Vertex | Graphics::EBufferUsage::CopyDestination,
					.memoryAccess = Graphics::EMemoryAccess::CPUNone,
					.debugName = "MeshVertexBuffer",
				}));
			}

			// Record a pending copy (staging → GPU vertex buffer).
			m_pendingBufferCopies.push_back(PendingBufferCopy{
				.stagingOffset = vertexStagingOffset,
				.destination = meshRef.GetGpuVertexBuffer(),
				.destOffset = 0,
				.size = vertexBytes,
			});

			m_meshStaging.offset += vertexBytes;

			// ── Index data ───────────────────────────────────────────────
			dst = static_cast<char *>(m_meshStaging.cpuPtr) + m_meshStaging.offset;
			std::memcpy(dst, indices.data(), indexBytes);

			const uint64_t indexStagingOffset = m_meshStaging.offset;

			// Create or reuse the GPU index buffer.
			if (meshRef.GetGpuIndexBuffer() == nullptr || meshRef.GetGpuIndexBuffer()->GetSize() < indexBytes)
			{
				meshRef.SetGpuIndexBuffer(m_graphicsDevice->CreateBuffer(Graphics::BufferDescriptor{
					.size = indexBytes,
					.usage = Graphics::EBufferUsage::Index | Graphics::EBufferUsage::CopyDestination,
					.memoryAccess = Graphics::EMemoryAccess::CPUNone,
					.debugName = "MeshIndexBuffer",
				}));
			}

			// Record a pending copy (staging → GPU index buffer).
			m_pendingBufferCopies.push_back(PendingBufferCopy{
				.stagingOffset = indexStagingOffset,
				.destination = meshRef.GetGpuIndexBuffer(),
				.destOffset = 0,
				.size = indexBytes,
			});

			m_meshStaging.offset += indexBytes;

			// Both buffers are fully staged — mark for component removal.
			m_entitiesToMarkAsUploaded.push_back(entity.GetId());

			// TODO: This code is somewhat temporal, since the textures should be made available by HushCooker on
			// material instancing, we'll figure that out when we merge that feature Upload all material textures for
			// this mesh (full mode, not streaming).
			for (auto &[materialPtr, textureMap] : meshRef.GetMaterialTextureRefs())
			{
				for (auto &[binding, texRef] : textureMap)
				{
					if (!texRef->IsCpuImageValid())
					{
						continue;
					}
					if (texRef->GetGpuTexture() != nullptr)
					{
						continue;
					}

					Image *cpuImage = texRef->GetCpuImage();

					auto gpuTexture = m_graphicsDevice->CreateTexture(Graphics::TextureDescriptor{
						.width = cpuImage->GetWidth(),
						.height = cpuImage->GetHeight(),
						.depth = cpuImage->GetDepth(),
						.mipLevels = 1,
						.arrayLayers = 1,
						.sampleCount = 1,
						.format = cpuImage->GetFormat(),
						.usage = Graphics::ETextureUsage::Sampled | Graphics::ETextureUsage::CopyDestination,
						.debugName = "MeshMaterialTexture",
					});

					uint64_t cpuOffset = 0;
					ETextureUploadNextStep step = UploadTexture({
						.cpuImage = cpuImage,
						.cpuOffsetRef = &cpuOffset,
						.gpuTexture = gpuTexture.get(),
						.isStreaming = false,
					});

					if (step == ETextureUploadNextStep::Finished)
					{
						Graphics::IGraphicsTexture *rawPtr = gpuTexture.get();
						texRef->SetGpuTexture(std::move(gpuTexture));
						const_cast<Graphics::Material3D *>(materialPtr)->SetTexture(binding, rawPtr);
					}
					else if (step == ETextureUploadNextStep::Stop)
					{
						shouldContinue = false;
						break;
					}
					// Continue: staging was full, gpuTexture is dropped,
					// texture stays without GPU resource — retry next frame.
				}
				if (!shouldContinue)
				{
					break;
				}
			}

			++uploadCount;
		}

		if (uploadCount >= MAX_RESOURCE_UPLOADS_PER_FRAME || m_meshStaging.Remaining() == 0)
		{
			break;
		}
	}
}

Hush::Renderer::ResourceUploadSystem::ETextureUploadNextStep Hush::Renderer::ResourceUploadSystem::UploadTexture(
	const TextureUploadCommand &command)
{
	Image *cpuImage = command.cpuImage;

	std::span<const std::byte> cpuImageData = cpuImage->GetTextureData();
	const uint64_t totalSize = cpuImageData.size_bytes();
	const uint64_t remaining = totalSize - *command.cpuOffsetRef;

	const bool isStreaming = command.isStreaming;

	uint64_t bytesToUpload = 0;

	const uint32_t width = cpuImage->GetWidth();
	const uint32_t height = cpuImage->GetHeight();

	// Compute the tight (unpadded) and aligned row pitches.
	// Graphics APIs (e.g. WebGPU) require bytesPerRow to be a
	// multiple of ROW_BYTE_ALIGNMENT (256).
	const uint64_t bytesPerRow =
		static_cast<uint64_t>(width) * static_cast<uint64_t>(Graphics::GetBytesPerPixel(cpuImage->GetFormat()));
	const uint64_t alignedBytesPerRow =
		(bytesPerRow > 0) ? (bytesPerRow + ROW_BYTE_ALIGNMENT - 1) & ~(ROW_BYTE_ALIGNMENT - 1) : 0;

	// Align the current staging write cursor so that the buffer
	// offset for this texture satisfies the row-alignment constraint.
	m_textureStaging.offset = (m_textureStaging.offset + ROW_BYTE_ALIGNMENT - 1) & ~(ROW_BYTE_ALIGNMENT - 1);
	const uint64_t alignedStagingRemaining = m_textureStaging.Remaining();

	if (alignedStagingRemaining == 0)
	{
		// Staging region exhausted after alignment — stop this frame.
		return ETextureUploadNextStep::Stop;
	}

	// Number of source rows still to upload.
	const uint32_t remainingRows = (bytesPerRow > 0) ? static_cast<uint32_t>(remaining / bytesPerRow) : height;

	if (!isStreaming)
	{
		// Full mode: all remaining rows must fit at once (aligned).
		const uint64_t alignedRequired = static_cast<uint64_t>(remainingRows) * alignedBytesPerRow;
		if (alignedRequired > alignedStagingRemaining)
		{
			// Not enough staging room for a full upload — defer to
			// next frame rather than partially uploading in Full mode.
			return ETextureUploadNextStep::Continue;
		}
		bytesToUpload = remaining;
	}
	else
	{
		// Streaming mode: stage as many complete rows as we can.
		// Enforce a minimum of one texture row to avoid tiny stalls.
		if (alignedStagingRemaining < alignedBytesPerRow && bytesPerRow <= remaining)
		{
			// Not even a single aligned row fits — skip this entry this frame.
			return ETextureUploadNextStep::Continue;
		}

		const uint32_t rowsThatFit = (alignedBytesPerRow > 0)
										 ? static_cast<uint32_t>(alignedStagingRemaining / alignedBytesPerRow)
										 : remainingRows;
		const uint32_t rowsToStage = std::min(rowsThatFit, remainingRows);
		bytesToUpload = static_cast<uint64_t>(rowsToStage) * bytesPerRow;
	}

	// Compute row counts for the copy region.
	const uint32_t startRow = (bytesPerRow > 0) ? static_cast<uint32_t>(*command.cpuOffsetRef / bytesPerRow) : 0;
	const uint32_t rowCount = (bytesPerRow > 0) ? static_cast<uint32_t>(bytesToUpload / bytesPerRow) : height;
	const uint64_t alignedStagingSize = static_cast<uint64_t>(rowCount) * alignedBytesPerRow;

	// Copy CPU data → texture staging region (CPU-only memcpy).
	// Each row is written at an aligned stride so the GPU copy
	// command sees correctly pitched data.
	char *dst = static_cast<char *>(m_textureStaging.cpuPtr) + m_textureStaging.offset;
	const char *src = reinterpret_cast<const char *>(cpuImageData.data()) + *command.cpuOffsetRef;

	if (bytesPerRow == alignedBytesPerRow)
	{
		// Row pitch already satisfies alignment — single memcpy.
		std::memcpy(dst, src, bytesToUpload);
	}
	else
	{
		// Copy each row individually, leaving alignment padding
		// between rows in the staging buffer.
		for (uint32_t row = 0; row < rowCount; ++row)
		{
			std::memcpy(dst + (row * alignedBytesPerRow), src + (row * bytesPerRow), bytesPerRow);
		}
	}

	// The texture staging region starts at m_halfQuota within the
	// overall staging buffer.
	const uint64_t bufferOffset = m_halfQuota + m_textureStaging.offset;

	// Record a pending copy (staging → GPU texture).
	m_pendingTextureCopies.push_back(PendingTextureCopy{
		.stagingOffset = bufferOffset,
		.destination = command.gpuTexture,
		.dstX = 0,
		.dstY = startRow,
		.dstZ = 0,
		.width = width,
		.height = rowCount,
		.depth = 1,
		.rowPitch = static_cast<uint32_t>(alignedBytesPerRow),
	});

	m_textureStaging.offset += alignedStagingSize;
	*(command.cpuOffsetRef) += bytesToUpload;

	return ETextureUploadNextStep::Finished;
}

void Hush::Renderer::ResourceUploadSystem::StageDirtyTextures()
{
	size_t uploadCount = 0;

	bool shouldContinue = true;

	for (auto it = m_textureUploadQuery.begin();
		 it != m_textureUploadQuery.end() && uploadCount < MAX_RESOURCE_UPLOADS_PER_FRAME && shouldContinue; ++it)
	{
		auto [uploadStateSpan, textureComponentSpan] = *it;
		for (uint32_t i = 0; i < it.Size(); ++i)
		{
			Hush::Entity entity = it.GetEntity(i);
			GpuUploadComponent &uploadState = uploadStateSpan[i];
			auto &textureComponent = textureComponentSpan[i];

			// Skip non-dirty or data-less entries.
			if (!textureComponent->IsCpuImageValid())
			{
				m_entitiesToMarkAsUploaded.push_back(entity.GetId());
				continue;
			}

			Image *cpuImage = textureComponent->GetCpuImage();

			std::span<const std::byte> cpuImageData = cpuImage->GetTextureData();
			const uint64_t totalSize = cpuImageData.size_bytes();
			const uint64_t remaining = totalSize - uploadState.cpuOffset;

			if (remaining == 0)
			{
				// Fully uploaded already — just clear the flag.
				// This case should never happen but we handle it defensively just in case.
				m_entitiesToMarkAsUploaded.push_back(entity.GetId());
				continue;
			}

			// Ensure the GPU texture exists.
			if (textureComponent->GetGpuTexture() == nullptr)
			{
				// This texture doesn't have a GPU resource yet — create it now since we know we have valid CPU data to
				// upload.
				auto gpuTexture = m_graphicsDevice->CreateTexture(Graphics::TextureDescriptor{
					.width = cpuImage->GetWidth(),
					.height = cpuImage->GetHeight(),
					.depth = cpuImage->GetDepth(),
					.mipLevels = 1, // TODO: Support mip levels in the future.
					.arrayLayers = 1,
					.sampleCount = 1,
					.format = textureComponent->GetCpuImage()->GetFormat(),
					.usage = Graphics::ETextureUsage::Sampled | Graphics::ETextureUsage::CopyDestination,
					.debugName = "UploadedTexture",
				});

				textureComponent->SetGpuTexture(std::move(gpuTexture));
			}

			// We have a valid CPU image and a GPU texture — we can proceed
			// with staging the upload.
			//
			// Determine how much we can stage this frame.

			ETextureUploadNextStep nextStep =
				UploadTexture({.cpuImage = cpuImage,
							   .cpuOffsetRef = &(uploadState.cpuOffset),
							   .gpuTexture = textureComponent->GetGpuTexture(),
							   .isStreaming = uploadState.uploadMode == GpuUploadComponent::EUploadMode::Streaming});

			if (nextStep == ETextureUploadNextStep::Continue)
			{
				continue;
			}
			if (nextStep == ETextureUploadNextStep::Stop)
			{
				shouldContinue = false;
				break;
			}

			// Check whether the upload is now complete.
			if (uploadState.cpuOffset >= totalSize)
			{
				// This resource is fully uploaded — mark as clean.
				m_entitiesToMarkAsUploaded.push_back(entity.GetId());
			}

			++uploadCount;
		}

		if (uploadCount >= MAX_RESOURCE_UPLOADS_PER_FRAME || m_textureStaging.Remaining() == 0)
		{
			break;
		}
	}
}
