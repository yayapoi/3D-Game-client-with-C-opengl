#include "scene/components/ui/ButtonComponent.h"
#include "scene/components/ui/CanvasComponent.h"
#include "scene/GameObject.h"

namespace eng
{
    void ButtonComponent::LoadProperties(const nlohmann::json& json)
    {
        if (json.contains("rect"))
        {
            auto& rectObj = json["rect"];
            SetRect(glm::vec2(
                rectObj.value("x", 1.0f),
                rectObj.value("y", 1.0f)
            ));
        }

        if (json.contains("color"))
        {
            auto& colorObj = json["color"];
            SetColor(glm::vec4(
                colorObj.value("r", 1.0f),
                colorObj.value("g", 1.0f),
                colorObj.value("b", 1.0f),
                colorObj.value("a", 1.0f)
            ));
        }

        if (json.contains("hovered"))
        {
            auto& colorObj = json["hovered"];
            SetHoveredColor(glm::vec4(
                colorObj.value("r", 1.0f),
                colorObj.value("g", 1.0f),
                colorObj.value("b", 1.0f),
                colorObj.value("a", 1.0f)
            ));
        }

        if (json.contains("pressed"))
        {
            auto& colorObj = json["pressed"];
            SetPressedColor(glm::vec4(
                colorObj.value("r", 1.0f),
                colorObj.value("g", 1.0f),
                colorObj.value("b", 1.0f),
                colorObj.value("a", 1.0f)
            ));
        }
    }

    void ButtonComponent::Render(CanvasComponent* canvas)
    {
        if (!canvas)
        {
            return;
        }

        auto pos = m_owner->GetWorldPosition2D();
        pos.x -= m_rect.x * m_pivot.x;
        pos.y -= m_rect.y * m_pivot.y;

        canvas->DrawRect(
            pos,
            pos + m_rect,
            *m_currentColor
        );
    }

    bool ButtonComponent::HitTest(const glm::vec2& pos) const
    {
        auto ownerPos = m_owner->GetWorldPosition2D();
        float x1 = ownerPos.x - m_rect.x * m_pivot.x;
        float y1 = ownerPos.y - m_rect.y * m_pivot.y;
        float x2 = x1 + m_rect.x;
        float y2 = y1 + m_rect.y;

        return (x1 <= pos.x && x2 >= pos.x && y1 <= pos.y && y2 >= pos.y);
    }

    void ButtonComponent::OnPointerEnter()
    {
        m_currentColor = &m_hoveredColor;
    }

    void ButtonComponent::OnPointerExit()
    {
        m_currentColor = &m_color;
    }

    void ButtonComponent::OnPointerUp()
    {
        m_currentColor = &m_hoveredColor;
    }

    void ButtonComponent::OnPointerDown()
    {
        m_currentColor = &m_pressedColor;
    }

    void ButtonComponent::OnClick()
    {
        if (onClick)
        {
            onClick();
        }
    }

    void ButtonComponent::SetRect(const glm::vec2& rect)
    {
        m_rect = rect;
    }

    const glm::vec2& ButtonComponent::GetRect() const
    {
        return m_rect;
    }

    void ButtonComponent::SetColor(const glm::vec4& color)
    {
        m_color = color;
    }

    const glm::vec4& ButtonComponent::GetColor() const
    {
        return m_color;
    }

    void ButtonComponent::SetHoveredColor(const glm::vec4& color)
    {
        m_hoveredColor = color;
    }

    const glm::vec4& ButtonComponent::GetHoveredColor() const
    {
        return m_hoveredColor;
    }

    void ButtonComponent::SetPressedColor(const glm::vec4& color)
    {
        m_pressedColor = color;
    }

    const glm::vec4& ButtonComponent::GetPressedColor() const
    {
        return m_pressedColor;
    }
}