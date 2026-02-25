#include "Game.h"
#include "TestObject.h"
#include "Player.h"
#include "Bullet.h"
#include "JumpPlatform.h"

#include <iostream>

void Game::RegisterTypes()
{
    Player::Register();
    Bullet::Register();
    JumpPlatform::Register();
}

bool Game::Init()
{
    //auto scene = eng::Scene::Load("scenes/scene.sc");
    //m_scene = scene;
    //eng::Engine::GetInstance().SetScene(scene.get());

    m_scene = std::make_shared<eng::Scene>();
    eng::Engine::GetInstance().SetScene(m_scene.get());

    auto sprite = m_scene->CreateObject("Sprite");
    auto spriteComponent = new eng::SpriteComponent();

    auto texture = eng::Texture::Load("textures/brick.png");
    spriteComponent->SetTexture(texture);

    sprite->AddComponent(spriteComponent);
    sprite->SetPosition2D(glm::vec2(500.0f, 500.0f));

    spriteComponent->SetSize(glm::vec2(200.0f, 100.0f));
    spriteComponent->SetUpperRightUV(glm::vec2(2.0f, 1.0f));
    sprite->SetRotation2D(glm::radians(45.0f));


    auto camera = m_scene->CreateObject("Camera");
    auto cameraComponent = new eng::CameraComponent();
    camera->AddComponent(cameraComponent);
    m_scene->SetMainCamera(camera);

    auto canvas = m_scene->CreateObject("Canvas");
    auto canvasComponent = new eng::CanvasComponent();
    canvas->AddComponent(canvasComponent);

    auto& uiInput = eng::Engine::GetInstance().GetUIInputSystem();
    uiInput.SetActive(true);
    uiInput.SetCanvas(canvasComponent);

    auto button = m_scene->CreateObject("Button", canvas);
    button->SetPosition2D(glm::vec2(300.0f, 300.0f));
    auto buttonComponent = new eng::ButtonComponent();
    buttonComponent->SetRect(glm::vec2(150.0f, 50.0f));
    buttonComponent->SetColor(glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
    button->AddComponent(buttonComponent);

    auto text = m_scene->CreateObject("Text", canvas);
    text->SetPosition2D(glm::vec2(300.0f, 300.0f));
    auto textComponent = new eng::TextComponent();
    text->AddComponent(textComponent);
    textComponent->SetText("Some Text");
    textComponent->SetFont("fonts/arial.ttf", 24);
    textComponent->SetColor(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    return true;
}

void Game::Update(float deltaTime)
{
    m_scene->Update(deltaTime);
}

void Game::Destroy()
{

}