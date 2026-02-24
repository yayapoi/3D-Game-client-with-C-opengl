#include "scene/components/ui/TextComponent.h"

namespace eng
{
    void TextComponent::Render(CanvasComponent* canvas)
    {

    }

    const std::string& TextComponent::GetText() const
    {
        return m_text;
    }

    void TextComponent::SetText(const std::string& text)
    {
        m_text = text;
    }
}