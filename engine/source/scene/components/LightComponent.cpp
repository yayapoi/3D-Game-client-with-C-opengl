#include "scene/components/LightComponent.h"

namespace eng
{
    void LightComponent::Update(float deltaTime)
    {
    }

    void LightComponent::SetColor(const glm::vec3& color)
    {
        m_color = color;
    }

    const glm::vec3& LightComponent::GetColor() const
    {
        return m_color;
    }
}