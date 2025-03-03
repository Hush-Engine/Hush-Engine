#pragma once

#include "IEditorPanel.hpp"
#include <glm/vec3.hpp>

namespace Hush
{
    class DebugTooltip final : public IEditorPanel
    {
    public:
        static inline DebugTooltip *s_debugTooltip = nullptr;
        void OnRender() noexcept override;

        const glm::vec3 &GetScale() const;

        const glm::vec3 &GetRotation() const;

        const glm::vec3 &GetTranslation() const;

    private:
        glm::vec3 m_scale{0.5f};
        glm::vec3 m_rotation;
        glm::vec3 m_translation;
    };

} // namespace Hush
