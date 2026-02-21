### 通过`InputManager`处理用户输入

让我们在引擎中实现 **用户输入** 支持。为此，我们将创建一个名为`InputManager`的新组件。

---

### Step 1: 文件和文件夹设置

1. 前往 `engine/source/` 目录，并新建一个名为 `input` 的文件夹。

2. 在此文件夹中，创建两个文件：

   * `InputManager.h`
   * `InputManager.cpp`

3. 打开`eng.h`并添加`#include "input/InputManager.h"`，以便输入系统对引擎用户可见。

4. 在引擎的`CMakeLists.txt`文件中，将`InputManager.h`和`InputManager.cpp`都包含在构建目标中。

---

### Step 2: 实现`InputManager`类

这个类将仅由 **引擎** 本身创建和拥有。为了确保这一点，我们将采取以下措施：

* 将所有构造函数设为私有：默认构造函数、复制构造函数、移动构造函数。
* 将`Engine`声明为`友元类`，这样只有`Engine`可以构造和访问`InputManager`。

#### 示例 (`InputManager.h`):

```cpp
#pragma once
#include <array>

namespace eng 
{

    class InputManager 
    {
        friend class Engine;

    private:
        InputManager() = default;
        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;
        InputManager(InputManager&&) = delete;
        InputManager& operator=(InputManager&&) = delete;

    public:
        void SetKeyPressed(int key, bool pressed);
        bool IsKeyPressed(int key) const;

    private:
        std::array<bool, 256> mKeys = { false };
    };

}
```

#### `InputManager.cpp`:

```cpp
#include "InputManager.h"

namespace eng 
{

    void InputManager::SetKeyPressed(int key, bool pressed) 
    {
        if (key < 0 || key >= static_cast<int>(mKeys.size()))
        { 
            return; 
        }
        mKeys[key] = pressed;
    }

    bool InputManager::IsKeyPressed(int key) const 
    {
        if (key < 0 || key >= static_cast<int>(mKeys.size())) 
        {
            return false;
        }
        return mKeys[key];
    }

}
```

---

### Step 3: 与`Engine`集成

在`Engine.h`中，添加：

```cpp
#include "input/InputManager.h"

private:
    InputManager m_inputManager;

public:
    InputManager& GetInputManager();
```

在 `Engine.cpp` 中:

```cpp
InputManager& Engine::GetInputManager() {
    return m_inputManager;
}
```

---

### Step 4: 使`Engine`全局可访问

为了在游戏代码中的任何位置使用输入，我们将使`Engine`成为一个 **单例** 。修改`Engine`类：

```cpp
class Engine 
{
public:
    static Engine& GetInstance();

private:
    Engine() = default;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;
    //...
};
```

`Engine.cpp`中的实现:

```cpp
Engine& Engine::GetInstance() 
{
    static Engine instance;
    return instance;
}
```

现在，你可以通过`Engine::GetInstance()`全局访问引擎。

更新`main.cpp`以使用单例模式，而不是手动创建引擎：

```cpp
eng::Engine& engine = eng::Engine::GetInstance();
```

---

### Step 5: 注册GLFW按键回调函数

在`Engine.cpp`文件中，`Engine::init()`函数内部，创建窗口之后，添加以下代码：

```cpp
void keyCallback(GLFWwindow*, int key, int, int action, int) 
{
    auto& inputManager = eng::Engine::GetInstance().GetInputManager();
    if (action == GLFW_PRESS)
    {
        inputManager.SetKeyPressed(key, true);
    }
    else if (action == GLFW_RELEASE)
    {
        inputManager.SetKeyPressed(key, false);
    
}
```

这个lambda表达式直接更新了`InputManager`中的键状态。

---

### Step 6: 测试输入处理

在`Game.cpp`文件中的`Game::Update`函数内，你现在可以这样检查输入：

```cpp
#include <GLFW/glfw3.h>
#include <iostream>
#include <eng.h>

void Game::Update(float deltaTime) 
{
    auto& input = eng::Engine::GetInstance().GetInputManager();

    if (input.IsKeyPressed(GLFW_KEY_A)) 
    {
        std::cout << "A button pressed" << std::endl;
    }
}
```

构建并运行你的游戏。当你按下 **A** 键时，你应该会在控制台中看到一条消息。

---

### 回顾

你现在拥有：

* 一个用于跟踪按键的全局`InputManager` 
* 一个基于单例模式的`Engine`类，用于全局访问 
* 通过GLFW回调实现正确的按键状态处理 
* 在游戏更新循环中进行输入查询