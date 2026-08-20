/*! \file ResourceUploadSystem.hpp
	\author Hush Engine
	\date 2026-02-17
	\brief ECS system that uploads dirty mesh and texture resources to the GPU
		   via a render-graph Transfer pass, avoiding CPU–GPU sync stalls.
*/

#pragma once

#include "Components/GpuUploadComponent.hpp"
#include "Components/MeshReference.hpp"
#include "Components/TextureComponent.hpp"
#include "Query.hpp"
#include "ISystem.hpp"
#include "RHI/IGraphicsBuffer.hpp"
#include "RHI/IGraphicsDevice.hpp"
#include "RHI/IGraphicsTexture.hpp"
#include "RenderGraph/RenderDevice.hpp"
#include "RenderGraph/RenderGraph.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace Hush::Renderer
{
	class ResourceUploadSystem : public ISystem
	{
	public:
		// ------------------------------------------------------------------
		// Staging-buffer layout constants
		// ------------------------------------------------------------------

		/// @brief Total size of the CPU-mapped intermediate staging buffer
		///        used to ferry data from CPU memory to the GPU.
		///        Default: 128 MiB.
		static constexpr uint64_t DEFAULT_STAGING_BUFFER_SIZE = 128ULL * 1024ULL * 1024ULL;

		/// @brief The first half of the staging buffer (64 MiB) is reserved
		///        for mesh vertex/index uploads.
		static constexpr uint64_t DEFAULT_MESH_STAGING_SIZE = DEFAULT_STAGING_BUFFER_SIZE / 2;

		/// @brief The second half of the staging buffer (64 MiB) is reserved
		///        for texture uploads.
		static constexpr uint64_t DEFAULT_TEXTURE_STAGING_SIZE = DEFAULT_STAGING_BUFFER_SIZE / 2;

		/// @brief Maximum number of individual resource uploads processed in
		///        a single frame.  Acts as an upper bound to prevent
		///        pathological iteration over a very large number of dirty
		///        entities.
		static constexpr size_t MAX_RESOURCE_UPLOADS_PER_FRAME = 64;

		/// @brief Describes a single buffer-to-buffer copy that will be
		///        recorded into the Transfer pass command list.
		struct PendingBufferCopy
		{
			uint64_t stagingOffset;					///< Byte offset into the staging buffer.
			Graphics::IGraphicsBuffer *destination; ///< Target GPU buffer (vertex or index).
			uint64_t destOffset;					///< Byte offset into the destination.
			uint64_t size;							///< Number of bytes to copy.
		};

		/// @brief Describes a single buffer-to-texture copy that will be
		///        recorded into the Transfer pass command list.
		struct PendingTextureCopy
		{
			uint64_t stagingOffset;					 ///< Byte offset into the staging buffer.
			Graphics::IGraphicsTexture *destination; ///< Target GPU texture.
			uint32_t dstX;							 ///< Destination X offset in texels.
			uint32_t dstY;							 ///< Destination Y offset in texels.
			uint32_t dstZ;							 ///< Destination Z offset (layer / depth).
			uint32_t width;							 ///< Width of the region to copy (texels).
			uint32_t height;						 ///< Height of the region to copy (texels).
			uint32_t depth;							 ///< Depth of the region to copy.
			uint32_t rowPitch;						 ///< Aligned bytes-per-row in the staging buffer.
		};

		/// @brief Pass-local data stored inside the render-graph Transfer pass.
		struct UploadPassData
		{
			RenderGraph::ResourceId stagingBufferResource{}; ///< Imported staging buffer (Read).
			RenderGraph::ResourceId syncToken{};			 ///< Transient sync token (Write).
		};

		// ------------------------------------------------------------------
		// Construction
		// ------------------------------------------------------------------

		/// @brief Construct the system with a scene, render device, and
		///        optional staging buffer size.
		///
		/// @param scene             The scene this system belongs to.
		/// @param renderDevice      Non-owning pointer to the render device
		///                          that owns the render graph and executor.
		///                          Must outlive this system.
		/// @param stagingBufferSize Total size in bytes of the CPU-mapped
		///                          staging buffer.  Half is used for meshes,
		///                          the other half for textures.
		///                          Defaults to 128 MiB.
		ResourceUploadSystem(Scene &scene, RenderGraph::RenderDevice *renderDevice,
							 uint64_t stagingBufferSize = DEFAULT_STAGING_BUFFER_SIZE);

		~ResourceUploadSystem() override = default;

		ResourceUploadSystem(const ResourceUploadSystem &) = delete;
		ResourceUploadSystem &operator=(const ResourceUploadSystem &) = delete;
		ResourceUploadSystem(ResourceUploadSystem &&) = delete;
		ResourceUploadSystem &operator=(ResourceUploadSystem &&) = delete;

		// ------------------------------------------------------------------
		// ISystem lifecycle
		// ------------------------------------------------------------------

		void Init() override;
		void OnShutdown() override;
		void OnUpdate(float delta) override;
		void OnFixedUpdate(float delta) override;

		/// @brief CPU-only staging pass.
		///
		/// Iterates all entities that carry a GpuMeshUploadState or
		/// GpuTextureUploadState component.  For each dirty resource the
		/// CPU data is memcpy'd into the staging buffer and a pending copy
		/// descriptor is appended.
		///
		/// No GPU calls are made here — the actual copies are deferred to
		/// the render-graph Transfer pass.
		void OnPreRender() override;

		void OnRender() override;
		void OnPostRender() override;

		[[nodiscard]]
		std::string_view GetName() const override
		{
			return "ResourceUploadSystem";
		}

		// ------------------------------------------------------------------
		// Configuration / queries
		// ------------------------------------------------------------------

		/// @brief Get the total staging buffer size (in bytes).
		[[nodiscard]]
		uint64_t GetStagingBufferSize() const noexcept
		{
			return m_stagingBufferSize;
		}

		/// @brief Get the per-half upload quota (in bytes).
		[[nodiscard]]
		uint64_t GetHalfQuota() const noexcept
		{
			return m_halfQuota;
		}

		/// @brief Get the number of bytes staged during the most recent
		///        OnPreRender() invocation.
		[[nodiscard]]
		uint64_t GetBytesUploadedLastFrame() const noexcept
		{
			return m_bytesUploadedLastFrame;
		}

		/// @brief Return the render-graph ResourceId of the upload sync
		///        token.
		///
		/// Rendering passes that consume uploaded mesh/texture data should
		/// call `ctx.Read(id)` on this resource in their build callback to
		/// create a dependency edge that ensures the Transfer pass completes
		/// before they execute.
		///
		/// @note  The returned ID is only valid after the render graph has
		///        been built for the current frame (i.e. after the
		///        RenderGraphSystem invokes the builder callbacks).
		[[nodiscard]]
		RenderGraph::ResourceId GetUploadSyncResourceId() const noexcept
		{
			return m_uploadSyncResourceId;
		}

		/// @brief Return true if there are pending copies to issue this
		///        frame (i.e. the Transfer pass has actual work to do).
		[[nodiscard]]
		bool HasPendingUploads() const noexcept
		{
			return !m_pendingBufferCopies.empty() || !m_pendingTextureCopies.empty();
		}

		/// @brief Read-only access to the pending buffer copies for the
		///        current frame.  Used by the Transfer pass execute callback.
		[[nodiscard]]
		const std::vector<PendingBufferCopy> &GetPendingBufferCopies() const noexcept
		{
			return m_pendingBufferCopies;
		}

		/// @brief Read-only access to the pending texture copies for the
		///        current frame.  Used by the Transfer pass execute callback.
		[[nodiscard]]
		const std::vector<PendingTextureCopy> &GetPendingTextureCopies() const noexcept
		{
			return m_pendingTextureCopies;
		}

		/// @brief Get the raw staging buffer pointer (for the Transfer pass
		///        execute callback to issue CopyBuffer commands against).
		[[nodiscard]]
		Graphics::IGraphicsBuffer *GetStagingBuffer() const noexcept
		{
			return m_stagingBuffer.get();
		}

	private:
		/// @brief Lightweight view into a contiguous region of CPU-mapped
		///        staging memory.
		struct StagingRegion
		{
			void *cpuPtr = nullptr;
			uint64_t offset = 0;
			uint64_t capacity = 0;

			[[nodiscard]]
			uint64_t Remaining() const noexcept
			{
				return (capacity > offset) ? (capacity - offset) : 0;
			}

			void Reset() noexcept
			{
				offset = 0;
			}
		};

		enum class ETextureUploadNextStep
		{
			Finished = 0,
			Continue,
			Stop
		};

		struct TextureUploadCommand
		{
			Image *cpuImage;
			uint64_t *cpuOffsetRef;
			Graphics::IGraphicsTexture *gpuTexture;
			bool isStreaming;
		};

		/// @brief Stage dirty mesh data into the mesh half of the staging
		///        buffer and record PendingBufferCopy descriptors.
		void StageDirtyMeshes();

		/// @brief Stage dirty texture data into the texture half of the
		///        staging buffer and record PendingTextureCopy descriptors.
		void StageDirtyTextures();

		ETextureUploadNextStep UploadTexture(const TextureUploadCommand &command);

		// ------------------------------------------------------------------
		// Render-graph integration
		// ------------------------------------------------------------------

		/// @brief Called from the RenderGraphBuilderComponent's builderFunc.
		///        Adds the Transfer pass to the render graph.
		void BuildUploadPass(RenderGraph::RenderGraph &graph);

		/// @brief Called from the RenderGraphBuilderComponent's
		///        frameUpdateFunc (fast path).  Updates the imported staging
		///        buffer resource without rebuilding the whole graph.
		void UpdateUploadPassImports(RenderGraph::RenderGraph &graph);

		// ------------------------------------------------------------------
		// Members
		// ------------------------------------------------------------------

		/// Non-owning pointer to the render device (graph + executor + device).
		RenderGraph::RenderDevice *m_renderDevice = nullptr;

		/// Convenience shortcut: m_renderDevice->GetGraphicsDevice().
		Graphics::IGraphicsDevice *m_graphicsDevice = nullptr;

		/// Total staging buffer size (bytes).
		uint64_t m_stagingBufferSize = DEFAULT_STAGING_BUFFER_SIZE;

		/// Per-half upload budget (bytes).
		uint64_t m_halfQuota = DEFAULT_STAGING_BUFFER_SIZE / 2;

		/// Bytes staged during the most recent OnPreRender() call.
		uint64_t m_bytesUploadedLastFrame = 0;

		/// The single CPU-mapped intermediate buffer (CopySource | CPUWrite).
		/// First half = mesh data, second half = texture data.
		std::unique_ptr<Graphics::IGraphicsBuffer> m_stagingBuffer;

		/// Mesh staging sub-region (first half).
		StagingRegion m_meshStaging;

		/// Texture staging sub-region (second half).
		StagingRegion m_textureStaging;

		// ------------------------------------------------------------------
		// Pending copy lists (populated in OnPreRender, consumed by Transfer pass)
		// ------------------------------------------------------------------

		std::vector<PendingBufferCopy> m_pendingBufferCopies;
		std::vector<PendingTextureCopy> m_pendingTextureCopies;

		// ------------------------------------------------------------------
		// Render-graph resource IDs (valid after graph build each frame)
		// ------------------------------------------------------------------

		/// Resource ID of the imported staging buffer in the render graph.
		RenderGraph::ResourceId m_stagingBufferResourceId{};

		/// Resource ID of the transient upload-sync token.
		/// Other passes Read this to depend on the Transfer pass.
		RenderGraph::ResourceId m_uploadSyncResourceId{};

		// ------------------------------------------------------------------
		// ECS queries
		// ------------------------------------------------------------------

		Hush::Query<GpuUploadComponent, MeshReference> m_meshUploadQuery;
		Hush::Query<GpuUploadComponent, Ref<TextureComponent>> m_textureUploadQuery;

		std::vector<Entity::EntityId>
			m_entitiesToMarkAsUploaded; // Temporary list of entities to mark as clean after staging, to avoid mutating
										// components while iterating
	};

} // namespace Hush::Renderer
