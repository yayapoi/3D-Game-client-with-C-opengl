#include "Player.h"
#include <GLFW/glfw3.h>

void Player::Init()
{
    if (auto bullet = FindChildByName("bullet_33"))
    {
        bullet->SetActive(false);
    }

    if (auto fire = FindChildByName("BOOM_35"))
    {
        fire->SetActive(false);
    }

    if (auto gun = FindChildByName("Gun"))
    {
        m_animationComponent = gun->GetComponent<eng::AnimationComponent>();
    }

    m_audioComponent = GetComponent<eng::AudioComponent>();
    m_playerControllerComponent = GetComponent<eng::PlayerControllerComponent>();
}

void Player::Update(float deltaTime)
{
    eng::GameObject::Update(deltaTime);

    auto& input = eng::Engine::GetInstance().GetInputManager();
    if (input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        if (m_animationComponent && !m_animationComponent->IsPlaying())
        {
            m_animationComponent->Play("shoot", false);

            if (m_audioComponent)
            {
                if (m_audioComponent->IsPlaying("shoot"))
                {
                    m_audioComponent->Stop("shoot");
                }
                m_audioComponent->Play("shoot");
            }
        }
    }

    if (input.IsKeyPressed(GLFW_KEY_SPACE))
    {
        if (m_audioComponent && !m_audioComponent->IsPlaying("jump"))
        {
            m_audioComponent->Play("jump");
        }
    }

    bool walking =
        input.IsKeyPressed(GLFW_KEY_W) ||
        input.IsKeyPressed(GLFW_KEY_A) ||
        input.IsKeyPressed(GLFW_KEY_S) ||
        input.IsKeyPressed(GLFW_KEY_D);

    if (walking && m_playerControllerComponent && m_playerControllerComponent->OnGround())
    {
        if (m_audioComponent && !m_audioComponent->IsPlaying("step"))
        {
            m_audioComponent->Play("step", true);
        }
    }
    else
    {
        if (m_audioComponent && m_audioComponent->IsPlaying("step"))
        {
            m_audioComponent->Stop("step");
        }
    }
}