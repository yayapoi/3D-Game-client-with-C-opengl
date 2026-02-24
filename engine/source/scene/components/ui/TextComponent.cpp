#include "scene/components/ui/TextComponent.h"
#include "font/Font.h"
#include "Engine.h"

namespace eng
{
    void TextComponent::LoadProperties(const nlohmann::json& json)
    {
        const std::string text = json.value("text", "");
        SetText(text);

        if (json.contains("font"))
        {
            const auto& fontObj = json["font"];
            std::string path = fontObj.value("path", "");
            int fontSize = fontObj.value("size", 12);
            SetFont(path, fontSize);
        }

        if (json.contains("color"))
        {
            const auto& colorObj = json["color"];
            glm::vec4 color(
                colorObj.value("r", 1.0f),
                colorObj.value("g", 1.0f),
                colorObj.value("b", 1.0f),
                colorObj.value("a", 1.0f)
            );
            SetColor(color);
        }
    }

    void TextComponent::Render(CanvasComponent* canvas)
    {
        if (m_text.empty() || !m_font || !canvas)
        {
            return;
        }


    }

    const std::string& TextComponent::GetText() const
    {
        return m_text;
    }

    void TextComponent::SetText(const std::string& text)
    {
        m_text = text;
    }

    const glm::vec4& TextComponent::GetColor() const
    {
        return m_color;
    }

    void TextComponent::SetColor(const glm::vec4& color)
    {
        m_color = color;
    }

    const std::shared_ptr<Font>& TextComponent::GetFont() const
    {
        return m_font;
    }

    void TextComponent::SetFont(const std::shared_ptr<Font>& font)
    {
        m_font = font;
    }

    void TextComponent::SetFont(const std::string& path, int size)
    {
        m_font = Engine::GetInstance().GetFontManager().GetFont(path, size);
    }

    glm::vec2 TextComponent::GetPivotPos() const
    {
        auto pos = m_owner->GetWorldPosition2D();

        glm::vec2 rect(0.0f);
        for (const auto c : m_text)
        {
            const auto& d = m_font->GetGlyphDescription(c);
            rect.x += static_cast<float>(d.advance);
            rect.y = std::max(rect.y, static_cast<float>(d.height));
        }

        pos.x -= std::round(rect.x * m_pivot.x);
        pos.y -= std::round(rect.y * m_pivot.y);
        return pos;
    }
}