/*! \file GpuUploadTag.hpp
	\author Hush Engine
	\date 2026-02-21
	\brief ECS tag component used to mark entities that require GPU uploads.
*/

#pragma once

#include <cstdint>

namespace Hush::Renderer
{
	/// @brief ECS component used to mark entities that require GPU uploads.
	///
	/// This is a simple empty struct that serves as a marker.  Entities with
	/// this component will be processed by the ResourceUploadSystem, which will
	/// perform the necessary GPU uploads and then remove this tag when done.
	///
	/// @note Only TextureComponent and MeshReference entities should
	/// be tagged with this component, as the ResourceUploadSystem assumes these
	/// types when processing uploads.
	///
	/// Tagging other entities is not supported as it won't do anything.
	/// You can implement your own system to handle uploads for other resource types.
	/// But it is not recommended to do so.
	struct GpuUploadComponent
	{
		enum class EUploadMode
		{
			Full,	  ///< Upload the entire texture in one operation (default).
			Streaming ///< Upload the texture in smaller chunks over multiple frames to stay within the per-frame upload
					  ///< quota.
		};

		uint64_t cpuOffset = 0; ///< Byte offset into the staging buffer where this entity's data is located.

		EUploadMode uploadMode = EUploadMode::Full; ///< Upload mode for this entity's resource.
	};
} // namespace Hush::Renderer
