### 实现核心类

让我们开始实现这些类。首先，打开 `Engine.h` 并定义 `Engine` 类的基本结构。由于引擎位于其自己的模块中，我们将它置于其自己的命名空间中——例如 `namespace eng`。

#### `Engine.h`

定义一个名为`Engine`的类，包含以下三个基本方法：

```cpp
bool Init();
void Run();
void Destroy();
```

#### `Engine.cpp`

现在转到`Engine.cpp`，添加`namespace eng`，并提供空实现：

```cpp
bool Engine::Init() { return false; }
void Engine::Run() {}
void Engine::Destroy() {}
```

---

### 定义应用程序接口

接下来，让我们在`Application.h`中定义`Application`类的基本结构。将所有内容都封装在`namespace eng`中。

如您所知，应用程序应包含三个核心方法：`Init`、`Update` 和 `Destroy`。由于这些方法将在派生类中实现，我们将它们设置为 **纯虚函数** ：

```cpp
class Application 
{
public:
    virtual bool Init() = 0;
    virtual void Update(float deltaTime) = 0; // deltaTime in seconds
    virtual void Destroy() = 0;

    void SetNeedsToBeClosed(bool value);
    bool NeedsToBeClosed() const;

protected:
    bool m_needsToBeClosed = false;
};
```

#### `Application.cpp`

现在实现非虚方法：

```cpp
void Application::SetNeedsToBeClosed(bool value) 
{
    m_needsToBeClosed = value;
}

bool Application::NeedsToBeClosed() const 
{
    return m_needsToBeClosed;
}
```

---

### 将引擎连接到应用程序

回到`Engine.h`，我们需要`Engine`来 **拥有并管理** 一个`Application`实例。

1. 在声明`Engine`之前，添加一个`Application`类的 **提前声明**。
2. 为`std::unique_ptr`包含`<memory>`头文件。

在`Engine`类中，添加：

```cpp
void SetApplication(Application* app);
Application* GetApplication();

private:
    std::unique_ptr<Application> m_application;
```

#### `Engine.cpp`

实现`getter`和`setter`方法：

```cpp
void Engine::SetApplication(Application* app) 
{
    m_application.reset(app);
}

Application* Engine::GetApplication() 
{
    return m_application.get();
}
```

---

### 实现引擎生命周期

#### `Engine::Init`

```cpp
bool Engine::Init() 
{
    if (!m_application)
    {
        return false;
    }
    return m_application->init();
}
```

#### `Engine::Destroy`

```cpp
void Engine::Destroy() 
{
    if (m_application) 
    {
        m_application->Destroy();
        m_application.reset();
    }
}
```

#### `Engine::Run`

这就是 **主游戏循环** 所在之处。

```cpp
void Engine::Run() 
{
    if (!m_application) 
    {
        return;
    }

    // Initialize time tracking
    m_lastTimePoint = std::chrono::high_resolution_clock::now();

    while (!m_application->NeedsToBeClosed()) 
    {
        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(now - m_lastTimePoint).count();
        m_lastTimePoint = now;

        m_application->Update(deltaTime);
    }
}
```

在`Engine.h`中，声明时间跟踪变量：

```cpp
#include <chrono>
std::chrono::time_point<std::chrono::high_resolution_clock> m_lastTimePoint;
```

---

现在，你的引擎和应用程序的基本结构和生命周期已经准备就绪。这为你在接下来的章节中扩展游戏引擎打下了坚实的基础。