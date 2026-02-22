现在，我们的相机可以显示场景的不同部分，但它是固定的。
我们想让它 **移动** 和 **旋转**，这样我们就可以在世界上飞行 -- 
就像游戏中的关卡编辑器或观众模式一样。

最简单的方法是制作一个**玩家控制器**组件。  
该组件将：

* 从键盘和鼠标读取输入
* 对其 **所有者** 对象应用移动和旋转
* 使用任何对象（包括带有“CameraComponent”的对象）

如果我们用相机将这个组件附着到物体上，
我们将有一个 **第一人称视角** 我们可以控制。

---

## Step 1: 创建PlayerController组件

在“scene/components”文件夹中：

* 创建`PlayerControllerComponent.h`
* 创建`PlayerControllerComponent.cpp`
* 将两者添加到 **CMakeLists.txt**
* 将它们包含在`eng.h`中

我们将使用前面的`COMPONENT`宏，以便引擎知道这个组件是什么。

---

### PlayerControllerComponent.h

```cpp
#pragma once
#include "scene/component.h"

namespace eng 
{

class PlayerControllerComponent : public Component 
{
    COMPONENT(PlayerControllerComponent) // 标识此组件类型

private:
    float m_sensitivity = 0.1f; // 鼠标旋转灵敏度
    float m_moveSpeed = 1.0f;   // 移动速度

public:
    void Update(float deltaTime) override;
};

} // namespace eng
```

*我们为控制器提供了两个重要设置：*

* **m_sensitivity**：移动鼠标时旋转的速度。
* **m_moveSpeed**：按下移动键时移动的速度。

---

## Step 2: 扩展鼠标输入的InputManager

我们的`InputManager`已经可以处理键盘按键了。
现在我们需要它来处理 **鼠标按钮** 和 **鼠标位置**。

为什么？

* **鼠标按钮**：因此，我们只能在按下左键（或我们选择的任何按钮）时旋转相机。
* **鼠标位置**：这样我们就可以测量鼠标自上一帧以来移动了多远 --
 这个距离将决定我们旋转的幅度。

---

### 在 `InputManager.h`

```cpp
std::array<bool, 16> m_mouseKeys = { false }; // 足够用于任何鼠标按钮

glm::vec2 m_mousePositionOld { 0.0f, 0.0f };      // 最后一帧位置
glm::vec2 m_mousePositionCurrent { 0.0f, 0.0f };  // 当前帧位置
```

我们还创建了 **get/set** 方法，就像键盘按键一样。

---

## Step 3: 为鼠标添加GLFW回调

我们需要两个回调函数：

1. **Mouse Button Callback** — 按下/松开鼠标按钮时调用
2. **Mouse Position Callback** — 每当光标移动时调用

---

### 鼠标按钮回调

```cpp
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) 
{
    auto& inputManager = Engine::getInstance().getInputManager();
    inputManager.SetMouseKey(button, action != GLFW_RELEASE);
}
```

---

### 鼠标位置回调

```cpp
void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) 
{
    auto& inputManager = Engine::GetInstance().GetInputManager();

    // 更新前将当前保存为旧
    inputManager.SetMousePositionOld(inputManager.GetMousePositionCurrent());

    // 设置新的当前位置
    glm::vec2 currentPos(static_cast<float>(xpos), static_cast<float>(ypos));
    inputManager.SetMousePositionCurrent(currentPos);
}
```

---

### 连接到 Engine::init()

```cpp
glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
glfwSetCursorPosCallback(m_window, cursorPositionCallback);
```

---

### 每帧重置旧鼠标位置

在 `Engine::run()`, 在 `glfwSwapBuffers`之后:

```cpp
inputManager.SetMousePositionOld(inputManager.GetMousePositionCurrent());
```

💬 *这很重要，所以当鼠标不移动时，我们不会一遍又一遍地得到相同的增量*

---

## Step 4: 清理旧动作代码

在`TestObject:：Update（）`中，删除任何直接改变位置的代码。
现在，移动将由`PlayerControllerComponent`处理。

---

## Step 5: 在PlayerController组件中查看鼠标

```cpp
void PlayerControllerComponent::Update(float deltaTime) 
{
    auto& input = Engine::getInstance().GetInputManager();
    auto rotation = m_owner->GetRotation();

    // 仅在按下鼠标左键时旋转
    if (input.IsMouseKeyPressed(GLFW_MOUSE_BUTTON_LEFT)) 
    {
        glm::vec2 oldPos = input.GetMousePositionOld();
        glm::vec2 curPos = input.GetMousePositionCurrent();

        float deltaX = curPos.x - oldPos.x;
        float deltaY = curPos.y - oldPos.y;

        // 偏航旋转（向左/向右看）—— 与鼠标移动方向相反
        rotation.y -= deltaX * mSensitivity * deltaTime;

        // 俯仰旋转（向上/向下看）
        rotation.x -= deltaY * mSensitivity * deltaTime;
    }

    m_owner->SetRotation(rotation);
```

*我们围绕Y轴旋转以进行水平鼠标移动（偏航）围绕X轴进行垂直移动（俯仰）。*

---

## Step 6: 使用键盘移动

为了相对于相机的旋转 **移动**，
我们首先根据旋转计算 **正向** 和 **右** 向量。

```cpp
    // 创建仅旋转矩阵
    glm::mat4 rotMat(1.0f);
    glm::vec3 rot = m_owner->getRotation();

    rotMat = glm::rotate(rotMat, rot.x, glm::vec3(1, 0, 0));
    rotMat = glm::rotate(rotMat, rot.y, glm::vec3(0, 1, 0));
    rotMat = glm::rotate(rotMat, rot.z, glm::vec3(0, 0, 1));

    glm::vec3 forward = glm::normalize(glm::vec3(rotMat * glm::vec4(0, 0, -1, 0)));
    glm::vec3 right   = glm::normalize(glm::vec3(rotMat * glm::vec4(1, 0, 0, 0)));

    auto position = m_owner->getPosition();

    if (input.IsKeyPressed(GLFW_KEY_W))
    {
        position += forward * mMoveSpeed * deltaTime;
    }
    else if (input.IsKeyPressed(GLFW_KEY_S)) 
    {
        position -= forward * mMoveSpeed * deltaTime;
    }

    if (input.IsKeyPressed(GLFW_KEY_D)) 
    {
        position += right * mMoveSpeed * deltaTime;
    }
    else if (input.IsKeyPressed(GLFW_KEY_A)) 
    {
        position -= right * mMoveSpeed * deltaTime;
    }

    m_owner->SetPosition(position);
}
```

*这里的关键是我们不是在固定的世界轴上移动 -- 我们相对于相机所面向的位置向前移动*

---

## Step 7: 把它们放在一起

在您的游戏设置中：

```cpp
camera->AddComponent(new PlayerControllerComponent());
```

---

## 最终结果

* 移动鼠标（按下左键）→ 照相机旋转
* 按 **W/S** → 相对于你看的地方向前/向后移动
* 按 **A/D** → 左/右扫看
* 运动和旋转是平滑的，因为它们与`deltaTime`一起缩放
* 速度和灵敏度可以在组件中调整

---

## 摘要

| Feature                | Why It Matters                                 |
| ---------------------- | ---------------------------------------------- |
| 鼠标增量跟踪            | 检测自上一帧以来鼠标移动了多远              |
| 鼠标按钮检查            | 让我们仅在按住按钮时旋转                    |
| 灵敏度设置              | 控制旋转响应                                 |
| 移动速度设置            | 控制我们移动的速度                                |
| 前/右矢量              | 相对于相机旋转进行移动                           |
| 增量时间使用            | 无论帧率如何，都能保持运动平稳                  |