# 玩家控制器：转弯、物理和角色控制器

现在，让我们让我们的 **玩家控制器** 使用起来更舒适。

## 1）用鼠标连续旋转

打开`PlayerControllerComponent::Update()`。  
目前，我们仅在按住鼠标左键时旋转。这不是现代射击游戏的工作方式。让我们取消按住 —— 现在旋转将始终跟随鼠标。

如果我们运行游戏，我们会注意到另一个问题：一旦光标离开屏幕，旋转就会停止。让我们解决这个问题。

在`Engine::Init()`中，设置完所有回调函数后，添加：

```cpp
glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
```

这将禁用操作系统光标并将鼠标移动锁定到我们的窗口。太好了！

## 2）InputManager中的鼠标移动标志

我们将改进`InputManager`，以便轻松检测鼠标是否移动了此帧。  
转到`InputManager.h`

添加字段：

```cpp
bool m_mousePositionChanged = false;
```

两种方法：

```cpp
void SetMousePositionChanged(bool changed);
bool IsMousePositionChanged() const;
```

### 在`cursorPositionCallback`中的`Engine.cpp`中更新

在回调结束时：

```cpp
glm::vec2 currentPos{ static_cast<float>(xpos), static_cast<float>(ypos) };
mInputManager.SetMousePositionCurrent(currentPos);
mInputManager.SetMousePositionChanged(true);
```

因此，每次鼠标移动时，都会设置标志。

### 在`Engine::Run()`中重置

在每一帧的末尾：

```cpp
m_inputManager.SetMousePositionChanged(false);
```

这给了我们一个“鼠标在这一帧移动了”事件。

## 3）改进玩家旋转

在`PlayerControllerComponent`中，我们不再检查鼠标按钮，而是：

```cpp
if (inputManager.IsMousePositionChanged()) 
{
    //应用旋转
}
```

### 旋转逻辑

* 围绕 **Y轴** 旋转以进行偏航（左转/右转）。
* 围绕 **X轴** 旋转以获得俯仰（向上/向下看）。
* 夹紧间距以避免翻转相机。

```cpp
// Yaw
float yDeltaAngle = -deltaX * m_sensitivity * deltaTime;
m_yRot += yDeltaAngle;
glm::quat yRot = glm::angleAxis(glm::radians(m_yRot), glm::vec3(0, 1, 0));

// Pitch
float xDeltaAngle = -deltaY * m_sensitivity * deltaTime;
m_xRot += xDeltaAngle;
m_xRot = std::clamp(m_xRot, -89.0f, 89.0f);
glm::quat xRot = glm::angleAxis(glm::radians(m_xRot), glm::vec3(1, 0, 0));

// Final rotation
rotation = glm::normalize(yRot * xRot);
m_owner->SetRotation(rotation);
```

现在我们平稳地旋转，始终在FPS限制范围内。

## 4）与物理相结合

由于我们已经有了一个物理世界， **玩家** 与之交互是有意义的。我们将使用 **运动体** ，而不是简单的变换。

Bullet提供了一个现成的助手：
**`btKinematicCharacterController`**。

我们将把它纳入我们自己的类。

## 5）KinematicCharacterController包装

创建`physics/KinematicCharacterController.h/.cpp`。

### Header

```cpp
#pragma once
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class btPairCachingGhostObject;
class btKinematicCharacterController;

class KinematicCharacterController 
{
public:
    KinematicCharacterController(float radius, float height);
    ~KinematicCharacterController();

    glm::vec3 GetPosition() const;
    glm::quat GetRotation() const;

    void Walk(const glm::vec3& direction);
    void Jump(const glm::vec3& direction);
    bool OnGround() const;

private:
    float m_height = 1.2f;
    float m_radius = 0.4f;

    std::unique_ptr<btPairCachingGhostObject> m_ghost;
    std::unique_ptr<btKinematicCharacterController> m_controller;
};
```

### 实现

```cpp
#include "KinematicCharacterController.h"
#include "Engine.h"

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

KinematicCharacterController::KinematicCharacterController(float radius, float height)
    : m_height(height), m_radius(radius)
{
    auto world = Engine::GetInstance().GetPhysicsManager().GetWorld();

    // Capsule collider (standard for characters)
    auto capsule = new btCapsuleShape(m_radius, m_height);

    m_ghost = std::make_unique<btPairCachingGhostObject>();
    btTransform start;
    start.setIdentity();
    start.setOrigin(btVector3(0.0f, 2.0f, 0.0f)); // spawn above ground
    m_ghost->setWorldTransform(start);
    m_ghost->setCollisionShape(capsule);
    m_ghost->setCollisionFlags(m_ghost->getCollisionFlags() | btCollisionObject::CF_CHARACTER_OBJECT);

    world->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

    const btScalar stepHeight = 0.35f;
    m_controller = std::make_unique<btKinematicCharacterController>(m_ghost.get(), capsule, stepHeight);

    m_controller->setMaxSlope(btRadians(50.0f));
    m_controller->setGravity(world->getGravity());

    world->addCollisionObject(m_ghost.get(),
        btBroadphaseProxy::CharacterFilter,
        btBroadphaseProxy::AllFilter & ~btBroadphaseProxy::SensorTrigger);
    world->addAction(m_controller.get());
}

KinematicCharacterController::~KinematicCharacterController() 
{
    auto world = Engine::GetInstance().GetPhysicsManager().GetWorld();
    if (m_controller) 
    {
        world->removeAction(m_controller.get());
    }
    if (m_ghost)     
    {
        world->removeCollisionObject(m_ghost.get());
    } 
}

glm::vec3 KinematicCharacterController::GetPosition() const 
{
    const auto& pos = m_ghost->getWorldTransform().getOrigin();
    // Offset upwards: camera is not at capsule center
    const glm::vec3 offset(0.0f, m_height * 0.5f + m_radius, 0.0f);
    return glm::vec3(pos.x(), pos.y(), pos.z()) + offset;
}

glm::quat KinematicCharacterController::GetRotation() const 
{
    const auto& rot = m_ghost->getWorldTransform().getRotation();
    return glm::quat(rot.w(), rot.x(), rot.y(), rot.z());
}

void KinematicCharacterController::Walk(const glm::vec3& dir) 
{
    m_controller->setWalkDirection(btVector3(dir.x, dir.y, dir.z));
}

void KinematicCharacterController::Jump(const glm::vec3& dir) 
{
    if (m_controller->onGround()) 
    {
        m_controller->jump(btVector3(dir.x, dir.y, dir.z));
    }
}

bool KinematicCharacterController::OnGround() const 
{
    return m_controller->onGround();
}
```

## 6）连接到PlayerController

添加到`PlayerControllerComponent`：

```cpp
std::unique_ptr<KinematicCharacterController> m_kinematicController;
```

```cpp
void PlayerControllerComponent::Init()
{
    m_kinematicController = std::make_unique<KinematicCharacterController>(0.4f, 1.2f);
}
```

处理旋转后，构建`front`和`right`向量。然后处理输入：

```cpp
glm::vec3 move(0.0f);
if (input.IsKeyPressed(GLFW_KEY_A)) 
{
    move -= right;
}
else if (input.IsKeyPressed(GLFW_KEY_D)) 
{
    move += right;
}
if (input.IsKeyPressed(GLFW_KEY_W)) 
{
    move += front;
}
else if (input.IsKeyPressed(GLFW_KEY_S)) 
{
    move -= front;
}

if (glm::dot(move, move) > 0)
{
    move = glm::normalize(move);
}
m_kinematicController->Walk(move * moveSpeed * deltaTime);

if (input.IsKeyPressed(GLFW_KEY_SPACE)) 
{
    m_kinematicController->Jump(glm::vec3(0.0f, 5.0f, 0.0f));
}

// 将场景对象与物理控制器同步
m_owner->SetPosition(m_kinematicController->GetPosition());
```

现在我们的玩家用 WASD 行走，用鼠标旋转，用空格跳跃。

## 7）专用玩家类

让我们把所有东西包装成一个“玩家”游戏对象。

创建`Player.h/.cpp`：

**Player.h**

```cpp
#pragma once
#include <eng.h>

class AnimationComponent;

class Player : public GameObject 
{
public:
    void Init();
    void Update(float dt) override;

private:
    AnimationComponent* m_animationComponent = nullptr;
};
```

**Player.cpp**

```cpp
#include "Player.h"
#include <GLFW/glfw3.h>

void Player::Init() 
{
    AddComponent(new eng::CameraComponent());
    SetPosition(glm::vec3(0.0f, 0.0f, 2.0f));
    AddComponent(new eng::PlayerControllerComponent());

    auto gun = eng::GameObject::LoadGLTF("models/sten_gunmachine_carbine/scene.gltf");
    gun->SetParent(this);
    gun->SetPosition(glm::vec3(0.75f, -0.5f, -0.75f));
    gun->SetScale(glm::vec3(-1.0f, 1.0f, 1.0f));

    if (auto anim = gun->GetComponent<eng::AnimationComponent>())
    {
        if (auto bullet = gun->FindChildByName("bullet_33"))
        {
            bullet->SetActive(false);
        }

        if (auto fire = gun->FindChildByName("BOOM_35"))
        {
            fire->SetActive(false);
        }

        anim->Play("shoot", false);
    }
    m_animationComponent = gun->GetComponent<eng::AnimationComponent>();
}

void Player::Update(float dt) 
{
    GameObject::Update(dt);

    auto& input = Engine::GetInstance().GetInputManager();
    if (input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) 
    {
        if (m_animationComponent && !m_animationComponent->IsPlaying()) 
        {
            m_animationComponent->Play("shoot", false);
        }
    }
}
```

同时添加`AnimationComponent::IsPlaying()`函数。

最后，在`Game.cpp`中删除创建相机和装弹枪的代码（它们现在位于`Player`中）

```cpp
auto player = m_scene->CreateObject<Player>("Player");
player->Init();
m_scene->SetMainCamera(player);
```

现在，玩家封装了相机、控制和武器动画。

## 8）运行游戏

* 光标锁定并隐藏
* 流畅的FPS风格旋转
* 玩家胶囊体与地面和箱子碰撞
* 玩家使用 WASD 四处走动，不穿过对象
* 使用空格键跳
* 鼠标左键点击射击动画

**好样的！**我们现在有一个真正的FPS风格的角色控制器，具有物理支持。这是朝着真正可玩原型迈出的一大步。
