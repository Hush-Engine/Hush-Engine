#pragma once
#include "IEditorPanel.hpp"
#include <cstdint>
#include <string>

namespace Hush
{
	class StatsPanel final : public IEditorPanel
	{

	public:
		void Init(Scene *activeScene) noexcept override;

		void OnRender(float deltaTime) noexcept override;

		void SetDeltaTime(float delta);

		void SetDrawCallsCount(int32_t count);

		void SetDeviceName(const std::string &deviceName);

	private:
		float m_deltaTime;
		float m_framesPerSecond;
		int32_t m_drawCallCount;
		std::string m_deviceName;
	};
} // namespace Hush
