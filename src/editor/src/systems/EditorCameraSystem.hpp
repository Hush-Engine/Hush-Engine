#pragma once
#include "ISystem.hpp"
#include "Shared/EditorCamera.hpp"

namespace Hush {
	class EditorCameraSystem final : public ISystem {
	public:
		EditorCameraSystem(const EditorCameraSystem &) = delete;
		EditorCameraSystem(EditorCameraSystem &&) = delete;
		EditorCameraSystem &operator=(const EditorCameraSystem &) = delete;
		EditorCameraSystem &operator=(EditorCameraSystem &&) = delete;

		~EditorCameraSystem() override = default;
		using ISystem::ISystem;

		/// Init() is called when the system is initialized.
		void Init() override;

		/// OnShutdown() is called when the system is shutting down.
		void OnShutdown() override;

		/// OnRender() is called when the system should render.
		/// @param delta Time since last frame
		void OnUpdate(float delta) override;

		/// OnFixedUpdate() is called when the system should update its state.
		/// @param delta Time since last fixed frame
		void OnFixedUpdate(float delta) override;

		/// OnRender() is called when the system should render.
		void OnRender() override;

		/// OnPreRender() is called before rendering.
		void OnPreRender() override;

		/// OnPostRender() is called after rendering.
		void OnPostRender() override;

		/// GetName() is used to get the name of the system
		/// @return Name of the system
		[[nodiscard]]
		std::string_view GetName() const override;
	private:
		
		float ApplyAccelerationCurve(float blend);
		
		EditorCamera* m_editorCamera{};
		
		float m_blendValue = 0.0F;
	};
}
