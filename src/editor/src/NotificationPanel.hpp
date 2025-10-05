#pragma once

#include "IEditorPanel.hpp"
#include "Query.hpp"
#include "UIUtils.hpp"

namespace Hush {
	
	class NotificationPanel : public IEditorPanel {
	public:
		void OnRender() override;

		void Init(Scene *activeScene) noexcept override;

	private:
		Query<ToastNotification> m_toastQuery;
	};
	
}

