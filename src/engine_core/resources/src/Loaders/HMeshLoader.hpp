#pragma once

#include "Components/MeshReference.hpp"
#include "Loaders/CrossLoaderDefinitions.hpp"
#include <span>

namespace Hush::HMeshLoader
{
	bool LoadMeshFromBinary(std::span<const std::byte> data, MeshReference *outRef,
							const RenderingContext *renderingCtx);
}
