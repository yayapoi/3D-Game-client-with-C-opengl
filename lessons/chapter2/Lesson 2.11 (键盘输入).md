现在，让我们在引擎内部实现 **键盘输入** 处理。

---

### 背景

在上一章中，我们通过直接键盘输入来控制物体的移动 —— 使物体在水平和垂直方向上移动。现在我们将采用同样的方法，但这次是使用我们引擎的输入和材质API。

---

### Step 1: 更新ShaderProgram - 为`vec2`添加`SetUniform`

首先，打开`ShaderProgram`并添加一个 **重载** 的`SetUniform()`方法，该方法接受 **两个`float`参数**：

```cpp
void SetUniform(const std::string& name, float v0, float v1);
```

在实现过程中，调用：

```cpp
glUniform2f(location, v0, v1);
```

这处理的是`vec2`类型的 uniforms。

---

### Step 2: 更新材质 – 存储并传递`vec2`类型的 uniforms 变量

打开`Material`类，然后：

1. 为`vec2`参数添加一个新的容器：

```cpp
std::unordered_map<std::string, std::pair<float, float>> m_float2Params;
```

2. 添加一个重载的`SetParam()`方法：

```cpp
void SetParam(const std::string& name, float v0, float v1);
```

在其实施过程中：

```cpp
m_float2Params[name] = {v0, v1};
```

3. 更新`Material`中的`Bind()`方法：

在设置完所有浮点型 uniforms 后，遍历`m_float2Params`：

```cpp
for (const auto& param : m_float2Params) 
{
    m_ShaderProgram->SetUniform(param.first, param.second.first, param.second.second);
}
```

现在，你的材质系统支持`vec2`类型的 uniforms。

---

### Step 3: 在游戏中添加偏移变量

在`Game.h`中，声明：

```cpp
float m_offsetX = 0.0f;
float m_offsetY = 0.0f;
```

---

### Step 4: 修改顶点着色器以使用偏移量

更新您的 **顶点着色器源代码**（位于`Game.cpp`中），以包含新的 uniform：

```glsl
uniform vec2 uOffset;

void main() 
{
    gl_Position = vec4(position.x + uOffset.x, position.y + uOffset.y, position.z, 1.0);
}
```

---

### Step 5: 在`Game::Update()`中处理按键输入

在`Game::Update()`函数中，使用引擎的输入管理器来检查按键操作：

```cpp
auto& inputManager = eng::Engine::GetInstance().GetInputManager();
if (inputManager.IsKeyPressed(GLFW_KEY_A)) 
{
    m_offsetX -= 0.01f;
} 
else if (inputManager.IsKeyPressed(GLFW_KEY_D)) 
{
    m_offsetX += 0.01f;
}

if (inputManager.IsKeyPressed(GLFW_KEY_W)) 
{
    m_offsetY += 0.01f;
} 
else if (inputManager.IsKeyPressed(GLFW_KEY_S)) 
{
    m_offsetY -= 0.01f;
}
```

---

### Step 6: 使用新偏移量更新材料

仍在`Game::Update()`函数中，处理完输入后：

```cpp
m_material.SetParam("uOffset", m_offsetX, m_offsetY);
```

---

### Step 7: 运行与测试

构建并运行你的应用程序。现在，你应该能够使用以下方式移动渲染对象：

* **W / S** 垂直移动 
* **A / D** 水平移动

如果物体按预期移动 —— **恭喜**！

---

### 最终说明

这一次，你不仅仅是移动一个硬编码的形状。你：

* 使用了你的 **自定义着色器系统**
* 通过 **材质 API** 传递了统一参数
* 使用 **InputManager** 处理输入
* 使用了一个带有可重用组件的合适的 **渲染管道**

曾经是一个单一的线性脚本，现在却变成了一个结构化且可扩展的迷你游戏引擎。

**做得好——这才是真正的引擎架构在发挥作用。**👏