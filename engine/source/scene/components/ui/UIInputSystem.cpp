#include "scene/components/ui/UIInputSystem.h"
#include "scene/components/ui/CanvasComponent.h"
#include "Engine.h"

#include <GLFW/glfw3.h>

namespace eng
{
    void UIInputSystem::SetActive(bool active)
    {
        m_active = active;
    }

    bool UIInputSystem::IsActive() const
    {
        return m_active;
    }

    void UIInputSystem::SetCanvas(CanvasComponent* canvas)
    {
        m_activeCanvas = canvas;
    }

    void UIInputSystem::Update(float deltaTime)
    {
        if (!m_active || !m_activeCanvas)
        {
            return;
        }

        auto& input = Engine::GetInstance().GetInputManager();
        bool mouseDown = input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
        bool mousePressed = input.WasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
        bool mouseReleased = input.WasMouseButtonReleased(GLFW_MOUSE_BUTTON_LEFT);

        auto mousePos = input.GetMousePositionCurrent();
        mousePos.y = Engine::GetInstance().GetGraphicsAPI().GetViewport().height - mousePos.y;
    }

}