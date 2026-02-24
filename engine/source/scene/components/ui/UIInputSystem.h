#pragma once

namespace eng
{
    class CanvasComponent;

    class UIInputSystem
    {
    public:
        void SetActive(bool active);
        bool IsActive() const;
        void SetCanvas(CanvasComponent* canvas);
        void Update(float deltaTime);

    private:
        bool m_active = false;
        CanvasComponent* m_activeCanvas = nullptr;
    };
}