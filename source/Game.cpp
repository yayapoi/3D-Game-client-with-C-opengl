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
    auto scene = eng::Scene::Load("scenes/scene.sc");
    m_scene = scene;
    eng::Engine::GetInstance().SetScene(scene.get());

    return true;
}

void Game::Update(float deltaTime)
{
    m_scene->Update(deltaTime);
}

void Game::Destroy()
{

}