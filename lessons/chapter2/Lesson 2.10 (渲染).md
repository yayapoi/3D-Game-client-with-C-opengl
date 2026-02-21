我们离实际渲染已经非常近了！让我们朝着终点线再迈进一步。

---

### **渲染概述**

大致思路如下：在 **更新阶段** ，我们将绘制请求累积到 **渲染队列** 中。一旦场景更新完毕，引擎进入 **渲染阶段** ，它将处理队列并发出所有必要的绘制调用。

让我们实施这个系统。

---

### Step 1: 创建渲染队列

在`render`文件夹中，创建两个文件：

* `RenderQueue.h`
* `RenderQueue.cpp`

将它们添加到`CMakeLists`中，并在`eng.h`中包含`RenderQueue.h`。

---

### Step 2: 定义RenderQueue类

在`RenderQueue`中，我们将存储 **绘制命令**。

为了表示每个绘图调用，定义一个名为`RenderCommand`的结构：

```cpp
struct RenderCommand 
{
    Mesh* mesh;
    Material* material;
};
```

现在，使用以下内容定义`RenderQueue`类：

* 一个名为`Submit()`的方法，用于将绘制命令加入队列：


```cpp
void Submit(const RenderCommand& command);
```

* 一个用来存放它们的容器：

```cpp
std::vector<RenderCommand> m_commands;
```

* 一种执行所有已排队绘图调用的方法：

```cpp
void Draw(GraphicsAPI& graphicsAPI);
```

---

### Step 3: 实现提交和绘图

在`Submit()`函数中：

```cpp
m_commands.push_back(command);
```

在 `Draw()` 中:

```cpp
for (const auto& command : m_commands) 
{
    graphicsAPI.BindMaterial(command.material);
    graphicsAPI.BindMesh(command.mesh);
    graphicsAPI.DrawMesh(command.mesh);
}
m_commands.clear();
```

---

### Step 4: 与引擎集成

打开`Engine.h`并添加一个`RenderQueue`成员：

```cpp
RenderQueue m_renderQueue;
```

另外，添加一个访问器：

```cpp
RenderQueue& GetRenderQueue();
```

---

### Step 5: 在引擎循环中渲染

打开`Engine::Run()`方法。在更新逻辑之后，且在`glfwSwapBuffers`之 **前** ，插入渲染调用。

但首先，我们需要清空屏幕并设置背景颜色。

---

### Step 6: 向GraphicsAPI添加缓冲区清除功能

在`GraphicsAPI.h`中，添加：

```cpp
void SetClearColor(float r, float g, float b, float a);
void ClearBuffers();
```

按照以下方式实现它们：

```cpp
void GraphicsAPI::SetClearColor(float r, float g, float b, float a) 
{
    glClearColor(r, g, b, a);
}

void GraphicsAPI::ClearBuffers() 
{
    glClear(GL_COLOR_BUFFER_BIT);
}
```

现在在`Engine::Run()`函数中，在绘图之前插入：

```cpp
m_graphicsAPI.SetClearColor(1.0f, 1.0f, 1.0f, 1.0f); // White background
m_graphicsAPI.ClearBuffers();

m_renderQueue.Draw(m_graphicsAPI);
```

---

### Step 7: 从游戏中提交渲染命令

打开`Game.cpp`文件，转到`Game::Update()`函数，并准备一个渲染命令：

```cpp
RenderCommand command;
command.mesh = m_mesh.get();
command.material = &m_material;

auto& renderQueue = eng::Engine::GetInstance().GetRenderQueue();
renderQueue.Submit(command);
```

---

### Step 8: 运行并验证

构建并运行应用程序。如果一切正常，您应该会在屏幕上看到渲染出的矩形，其顶点颜色经过平滑插值。

这证实了：

* 着色器正在工作。
* 几何形状定义正确。
* 渲染管道正在按预期运行。

---

### 摘要

你现在拥有：

* 一个 **渲染队列** ，用于组织绘图调用。
* 一种在更新过程中提交 **渲染命令** 的方法。
* 一个执行所有命令的干净渲染阶段。
* 具备清屏功能，且背景颜色可自定义。