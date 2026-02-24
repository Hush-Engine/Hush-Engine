/*! \file MaterialInstance.hpp
	\author Kyn21kx
	\date 2026-02-20
	\brief Material instance implementation.
*/

#pragma once

#include "Shared/MaterialPass.hpp"
#include "IPipeline.hpp"
#include "IBindGroup.hpp"

namespace Hush::Graphics
{
	/// @brief Material instance struct that holds the underlying graphics API resources for a material, as well as the
	/// material pass type it belongs to.
	struct GraphicsApiMaterialInstance
	{
		/// @brief The bind group containing the material's bound resources (e.g. textures, uniform buffers).
		IBindGroup *bindGroup;
		/// @brief The graphics pipeline associated with this material instance, which encapsulates the shader stages
		/// and fixed-function state for rendering.
		IPipeline *pipeline;

		/// @brief The material pass type (e.g. opaque, transparent) that this instance belongs to, which determines how
		/// it is rendered in the pipeline.
		EMaterialPass passType;
	};
} // namespace Hush::Graphics
