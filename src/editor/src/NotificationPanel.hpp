#pragma once

#include "IEditorPanel.hpp"
#include "Query.hpp"
#include "UIUtils.hpp"

namespace Hush
{

	class NotificationPanel : public IEditorPanel
	{
	public:
		void OnRender(float deltaTime) override;

		void Init(Scene *activeScene) noexcept override;

	private:
		Query<ToastNotification> m_toastQuery;
		Scene *m_scene;
	};

} // namespace Hush
