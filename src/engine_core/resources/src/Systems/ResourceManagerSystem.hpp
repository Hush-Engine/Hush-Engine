#pragma once

#include "ISystem.hpp"
#include "ResourceManager.hpp"

namespace Hush {
	class ResourceManagerSystem final : ISystem{
	public:
		ResourceManagerSystem(const ResourceManagerSystem &) = delete;
		ResourceManagerSystem(ResourceManagerSystem &&) = default;
		ResourceManagerSystem &operator=(const ResourceManagerSystem &) = delete;
		ResourceManagerSystem &operator=(ResourceManagerSystem &&) = default;

		void Init() override;

		void OnShutdown() override;

		void OnUpdate(float delta) override;

		void OnFixedUpdate(float delta) override;

		void OnRender() override;

		void OnPreRender() override;

		void OnPostRender() override;

		[[nodiscard]] std::string_view GetName() const override {
			return "ResourceManagerSystem";
		}
		
	private:
		ResourceManager* m_resourceManager;
	};
}

