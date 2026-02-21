### 使用引擎创建窗口

现在，让我们使用我们的引擎来创建一个 **窗口** —— 这是游戏（或任何其他应用程序）中所有动作发生的主要画布。

你可能还记得，我们之前使用 **GLFW** 库来创建窗口。所以现在，我们要将创建窗口的逻辑直接集成到我们的引擎中。

---

### Step 1: 将GLFW代码移入`Engine::Init`函数中

打开`Engine::Init()`函数。在检查应用程序实例是否有效（`if (!m_application)`）之后，插入之前编写的GLFW窗口创建代码：

1. `glfwInit()`
2. `glfwWindowHint()` 以指定 OpenGL 版本和配置文件
3. `glfwCreateWindow()` 指定窗口的宽度和高度
4. `glfwMakeContextCurrent()`
5. `glewInit()` （或类似函数以初始化 GL 函数指针）

为了实现动态分辨率，请修改 `Engine::Init()` 函数，使其接受两个参数：`int width`和`int height`。

```cpp
bool Engine::Init(int width, int height)
```

---

### Step 2: 将窗口存储在引擎中

之前，该窗口是存储在一个局部变量中的。但在引擎内部，我们希望将其 **作为类成员存储** ，以便在整个系统中访问。

执行以下操作：

* 在`Engine.h`文件中，在`eng`命名空间之前，对GLFW结构体进行前向声明：

```cpp
struct GLFWwindow;
```

* 在`Engine`类中，声明一个私有成员：

```cpp
GLFWwindow* m_window = nullptr;
```

* 在`Engine::Init`函数中，将`glfwCreateWindow`函数的结果赋值给`m_window`。

---

### Step 3: 在 `Engine::Destroy` 中进行清理

在调用`m_application->Destroy()`并重置应用程序实例后，清理GLFW：

```cpp
glfwTerminate();
m_window = nullptr;
```

---

### Step 4: 更新主游戏循环（`Engine::Run`）

现在我们来更新主循环，以便正确处理窗口事件：

```cpp
void Engine::Run() 
{
    if (!m_application || !m_window) 
    {
        return;
    }

    m_lastTimePoint = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(m_window) && !m_application->needsToBeClosed()) 
    {
        glfwPollEvents(); // Handle OS/window input

        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(now - m_lastTimePoint).count();
        m_lastTimePoint = now;

        m_application->Update(deltaTime); // Game logic update

        glfwSwapBuffers(m_window); // Placeholder for rendering
    }
}
```

### 主循环的分解：

* **`glfwPollEvents()`** — 处理输入 / 事件。
* **`Update(deltaTime)`** — application/game 逻辑。
* **`glfwSwapBuffers()`** — 将处理渲染（尽管我们尚未添加渲染功能）。

---

### Step 5: 更新`main.cpp`以提供分辨率

在你的`main.cpp`文件中，更新对`engine.Init()`的调用，以传递所需的屏幕分辨率，例如1280×720：

```cpp
if (engine.Init(1280, 720)) 
{
    engine.Run();
}
```

然后构建并运行你的项目。

---

### 结果

应用程序启动，一个窗口打开，你的引擎循环正在运行 —— 已准备好继续构建。

**恭喜你 —— 你刚刚为渲染和实时更新打下了基础！**