## 课程：向UI添加交互性 - 实现按钮和输入处理

因此，我们已经有了一个**文本组件**来处理文本渲染。  
但UI不仅仅是绘制文本或显示图像 —— 它还涉及**对用户输入的反应**。  
因此，为了演示这个概念，让我们创建另一个组件来完成这一任务。

而且，正如您可能已经猜到（或已经知道）的那样，最简单和最基本的交互组件是**Button**。

### 步骤1 - 扩展用于交互的基本UI元素

让我们从扩展基类`UIElementComponent`开始，因为交互性不应该局限于按钮 —— 其他UI元素（如滑块、复选框等）稍后也可能需要用户输入。

打开基类**UIElementComponent**并添加以下方法：

```cpp
virtual bool HitTest(const glm::vec2& pos) const;
virtual void OnPointerEnter();
virtual void OnPointerExit();
virtual void OnPointerUp();
virtual void OnPointerDown();
virtual void OnClick();
```

`HitTest`方法确定鼠标光标当前是否在元素的边界内，换句话说，该元素是否被“命中”。  
其余的方法（`OnPointerEnter`、`OnPointerExit`等）定义对不同光标和单击事件的反应。

---

### 步骤2 - 创建按钮组件

现在，在文件夹`scene/components/ui`中，创建两个新文件：

```
ButtonComponent.h  
ButtonComponent.cpp
```

声明类：

```cpp
class ButtonComponent : public UIElementComponent
```

使用宏：

```cpp
COMPONENT_2(ButtonComponent, UIElementComponent)
```

该按钮将具有两个主要字段：

```cpp
glm::vec2 m_rect;      // 按钮的大小（宽度和高度）
glm::vec4 m_color = glm::vec4(1.0f); // 默认颜色
```

添加getter和setter：

```cpp
void SetRect(const glm::vec2& rect);
const glm::vec2& GetRect() const;
void SetColor(const glm::vec4& color);
const glm::vec4& GetColor() const;
```

---

### 步骤3 - 实现虚拟方法

现在，让我们实现从`UIElementComponent`继承的所有虚拟方法：  
`HitTest`，`Render`，`OnPointerEnter`，`OnPointerExit`，`OnPointerUp`，`OnePointerDown`，`OnClick`。

#### `hitTest`

我们需要确定鼠标位置是否位于该按钮的矩形内。

```cpp
bool ButtonComponent::HitTest(const glm::vec2& pos) const 
{
    auto ownerPos = m_owner->GetWorldPosition2D();

    float x1 = ownerPos.x - m_rect.x * m_pivot.x;
    float y1 = ownerPos.y - m_rect.y * m_pivot.y;
    float x2 = x1 + m_rect.x;
    float y2 = y1 + m_rect.y;

    return (x1 <= pos.x && x2 >= pos.x && y1 <= pos.y && y2 >= pos.y);
}
```

---

### 步骤4 - 呈现按钮

接下来，让我们画一个按钮。  
内部`ButtonComponent:：Render（）`：

```cpp
auto pos = m_owner->GetWorldPosition2D();
pos.x -= m_rect.x * m_pivot.x;
pos.y -= m_rect.y * m_pivot.y;

canvas->DrawRect(
    pos,
    pos + m_rect,
    m_color
);
```

但要使其工作，我们需要在`CanvasComponent`中使用新的助手函数：

```cpp
void DrawRect(const glm::vec2& lowerLeft, const glm::vec2& upperRight, const glm::vec4& color)
{
    uint32_t base = static_cast<uint32_t>(m_vertices.size() / 8);

    m_vertices.insert(m_vertices.end(), {
        p2.x, p2.y, color.r, color.g, color.b, color.a, 1.0f, 1.0f,
        p1.x, p2.y, color.r, color.g, color.b, color.a, 0.0f, 1.0f,
        p1.x, p1.y, color.r, color.g, color.b, color.a, 0.0f, 0.0f,
        p2.x, p1.y, color.r, color.g, color.b, color.a, 1.0f, 0.0f,
        });
    m_indices.insert(m_indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });

    UpdateBatches(nullptr);
}
```

此方法类似于前面的纹理`DrawRect`，  
默认情况下设置UV坐标除外，纹理指针将为`nullptr`。  
在它的末尾，我们将调用`UpdateBatches（nullptr）`。

---

### 步骤5 - 添加悬停和按下状态

现在，让我们处理鼠标交互 —— 当光标进入、离开或按下按钮时。

我们将向`ButtonComponent`添加更多字段：

```cpp
glm::vec4 m_hoveredColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
glm::vec4 m_pressedColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
const glm::vec4* m_currentColor = &m_color;
```

然后，在`Render（）`中，我们使用：

```cpp
canvas->DrawRect(pos, pos + m_rect, *m_currentColor);
```

为悬停和按下的颜色添加setters/getters。  
现在，我们可以实现交互性：

```cpp
void ButtonComponent::OnPointerEnter() { m_currentColor = &m_hoveredColor; }
void ButtonComponent::OnPointerExit()  { m_currentColor = &m_color; }
void ButtonComponent::OnPointerUp()    { m_currentColor = &m_hoveredColor; }
void ButtonComponent::OnPointerDown()  { m_currentColor = &m_pressedColor; }
```

对于单击事件：

```cpp
std::function<void()> onClick;

void ButtonComponent::OnClick() 
{
    if (onClick) 
    {
        onClick();
    }
}
```

---

### 步骤6 - 创建UI输入系统

现在，我们需要一个系统来管理用户输入并将事件分发到UI元素。

创建两个新文件：

```
UIInputSystem.h  
UIInputSystem.cpp
```

声明：

```cpp
class UIInputSystem 
{
public:
    void SetActive(bool active);
    bool IsActive() const;
    void SetCanvas(CanvasComponent* canvas);
    void Update(float deltaTime);

private:
    bool m_active = false;
    CanvasComponent* m_activeCanvas = nullptr;
};
```

将其作为成员添加到引擎（在`engine`类中）：

```cpp
UIInputSystem m_uiInputSystem;
```

并使用getter `GetUIInputSystem（）`将其公开。

在`Engine:：Run（）`中，在`m_physicsManager.Update(deltaTime);`之后的每一帧更新它：

```cpp
if (m_uiInputSystem.IsActive())
{
    m_uiInputSystem.Update(deltaTime);
}
```

---

### 步骤7 - 连接InputManager和鼠标事件

`UIInputSystem`必须了解鼠标单击和移动。
让我们为此增强我们的`InputManager`。

添加：

```cpp
std::array<bool, 16> m_mouseKeyPressed = { false };
std::array<bool, 16> m_mouseKeyReleased = { false };
```

然后实现：

```cpp
void SetMouseButtonWasPressed(int button, bool pressed);
bool WasMouseButtonPressed(int button) const;
void SetMouseButtonWasReleased(int button, bool released);
bool WasMouseButtonReleased(int button) const;
```

此外：

```cpp
void ClearStates() 
{
    SetMousePositionChanged(false);
    for (auto& k : m_mouseKeyPressed) 
    {
        WasMouseButtonPressed(k, false);
    } 
    for (auto& k : m_mouseKeyReleased)
    {
        WasMouseButtonReleased(k, false);
    }
}
```

调用`m_inputManager.ClearStates()`而不是`m_inputManager.SetMousePositionChanged(false)`在`Engine:：Run（）`中的每个帧末尾。

现在更新 GLFW 回调：

```cpp
void mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    auto& inputManager = eng::Engine::GetInstance().GetInputManager();
    if (action == GLFW_PRESS)
    {
        inputManager.SetMouseButtonPressed(button, true);
        inputManager.SetMouseButtonWasPressed(button, true);
    }
    else if (action == GLFW_RELEASE)
    {
        inputManager.SetMouseButtonPressed(button, false);
        inputManager.SetMouseButtonWasReleased(button, true);
    }
}
```

---

### 步骤8 - 更新UI输入系统

最后，在`UIInputSystem:：Update（）`中：

```cpp
if (!m_active || !m_activeCanvas)
{
    return;
} 

auto& input = Engine::GetInstance().GetInputManager();
bool mouseDown     = input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
bool mousePressed  = input.WasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
bool mouseReleased = input.WasMouseButtonReleased(GLFW_MOUSE_BUTTON_LEFT);

auto mousePos = input.GetMousePositionCurrent();
mousePos.y = Engine::GetInstance().GetGraphicsAPI().GetViewport().height - mousePos.y;
```

现在，我们已经准备好了完整的设置 - 我们的输入系统处于活动状态，  
基本UI元素支持交互性，  
并且按钮组件现在可以在视觉上和逻辑上响应用户操作。

在 **下一课** 中，我们将处理该系统如何实际检测光标下的元素，
调度指针事件，并触发类似`onClick`的回调。

不要关掉 —— 接下来就是有趣的部分了！