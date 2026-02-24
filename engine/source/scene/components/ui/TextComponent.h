#pragma once

#include "scene/components/ui/UIElementComponent.h"

#include <string>

namespace eng
{
    class TextComponent : public UIElementComponent
    {
        COMPONENT_2(TextComponent, UIElementComponent)
    public:
        void Render(CanvasComponent* canvas) override;

        const std::string& GetText() const;
        void SetText(const std::string& text);

    private:
        std::string m_text;
    };
}