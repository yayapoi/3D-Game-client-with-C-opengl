## 实现UI输入逻辑（交互式按钮）

好了，现在让我们继续实现**实际的交互逻辑**。  
我们将使UI响应用户输入 - 单击、悬停和其他鼠标交互。

---

### 步骤1 - 准备地基

我们首先声明：

```cpp
UIElementComponent* hit = nullptr;
```

该指针稍后将保存对我们的光标当前悬停在上面的任何UI元素的引用。

接下来，我们需要**收集当前活动画布中存在的所有UI元素**，然后**迭代它们**，使用它们的`HitTest`函数检查每个元素是否被鼠标光标击中。

让我们实现一个助手函数：

```cpp
std::vector<UIElementComponent*> CollectUI(CanvasComponent* canvas);
```

在里面，我们将写道：

```cpp
std::vector<UIElementComponent*> result;
GameObject* canvasObject = canvas->GetOwner();
const auto& children = canvasObject->GetChildren();

for (const auto& child : children)
{
    if (auto component = child->GetComponent<UIElementComponent>())
    {
        // 下面我们将处理递归
    }
}
```

---

### 步骤2 - 元素的递归集合

为了使遍历递归，我们将在`CanvasComponent`**:

```cpp
void CollectUI(UIElementComponent* element, std::vector<UIElementComponent*>& out);
```

实现：

```cpp
void CanvasComponent::CollectUI(UIElementComponent* element, std::vector<UIElementComponent*>& out)
{
    out.push_back(element);

    const auto& children = element->GetOwner()->GetChildren();
    for (const auto& child : children)
    {
        if (auto component = child->GetComponent<UIElementComponent>())
        {
            CollectUI(component, out);
        }
    }
}
```

因此，该函数只是递归遍历每个子对象，并收集所有UI组件。

回到我们的`UIInputSystem`，现在可以调用：

```cpp
for (const auto& child : children)
{
    auto comp = child->GetComponent<UIElementComponent>();
    if (comp)
    {
        canvas->CollectUI(comp, result);
    }
}

return result;
```

这为我们提供了当前活动画布中所有UI元素的**完整递归列表**。

---

### 步骤3 - 跟踪悬停和按下的元素

在`UIInputSystem`中，添加两个新字段：

```cpp
UIElementComponent* m_hovered = nullptr;
UIElementComponent* m_pressed = nullptr;
```

* `m_hovered` - 当前在光标下的元素
* `m_pressed` - 上次单击的元素

现在，在`Update `函数中，我们可以开始处理输入：

```cpp
auto uiElements = CollectUI(m_activeCanvasComponent);
```

然后，我们将迭代所有元素并检查命中情况：

```cpp
for (auto element : uiElements)
{
    if (element->HitTest(mousePos))
    {
        hit = element;
        break;
    }
}
```

一旦我们找到了鼠标下的第一个元素，我们就停止处理其余的元素。

---

### 步骤4 - 管理悬停状态

现在，如果`hit`元素与`m_hovered`中存储的元素不同，则需要通知新旧元素：

```cpp
if (m_hovered != hit)
{
    if (m_hovered)
    {
        m_hovered->OnPointerExit();
    }

    m_hovered = hit;

    if (m_hovered)
    {
        m_hovered->OnPointerEnter();
    }
    m_pressed = nullptr;
}
```

因此，现在系统知道鼠标**何时进入**，**何时离开**小部件。

---

### 步骤5 - 鼠标按钮交互

接下来，我们将处理按下和释放鼠标左键的操作。

```cpp
if (!m_pressed)
{
    // No active click right now
    if (mousePressed && m_hovered)
    {
        m_pressed = m_hovered;
        m_pressed->OnPointerDown();
    }
}
```

当我们释放按钮时：

```cpp
if (mouseReleased)
{
    if (m_pressed)
    {
        m_pressed->OnPointerUp();

        if (m_pressed == m_hovered)
        {
            m_pressed->OnClick();
        }
    }

    m_pressed = nullptr;
}
```

漂亮 -- 这涵盖了**完整的鼠标交互生命周期**：  
`OnPointerEnter（鼠标进入）→ OnPointerDown 鼠标按下→ OnPointer 抬起鼠标→ OnClick（单击）→ OnPointerExit`。

在开始实际测试之前，我们需要具有启用/禁用游标的功能。转到`Engine`类并添加方法：

```cpp
void SetCursorEnabled(bool enabled)
{
    glfwSetInputMode(m_window, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}
```
在`Engine::Init()`中，设置所有回调后，删除此行：

```cpp
glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
```

所以我们有我们的光标回来了
---

### 步骤6 - 注册和测试我们的按钮

不要忘记在**场景**初始化代码中注册`ButtonComponent`：

```cpp
ButtonComponent::Register();
```

现在让我们测试所有内容。转到`Game.cpp`  
创建`Canvas`后，请编写以下内容：

```cpp
auto& uiInput = Engine::GetInstance().GetUIInputSystem();
uiInput.SetActive(true);
uiInput.SetCanvasComponent(canvasComponent);
```

这将启用输入系统并分配当前活动的画布。

---

### 步骤7 - 创建测试按钮

现在，我们将创建一个简单的按钮，上面有文本标签（使用前面实现的`TextComponent`）：

```cpp
auto button = m_scene->CreateObject("Button", canvas);
button->SetPosition2D(glm::vec2(300.0f, 300.0f));

auto buttonComponent = new eng::ButtonComponent();
buttonComponent->SetRect(glm::vec2(150, 50));
buttonComponent->SetColor(glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
button->AddComponent(buttonComponent);
```

运行应用程序。

现在，在屏幕上，您应该看到在矩形区域上渲染的文本 - 这是您的按钮。

如果将鼠标悬停在其上，颜色将发生变化（`OnPointerEnter`状态）。  
如果单击它，它将进一步变暗（`OnPointerDown`和`OnClick`状态）。

这意味着按钮组件是完全交互式的！

---

### 第8步 - 最终注释

当然，不要忘记为`ButtonComponent`实现 **`LoadProperties`** 方法，  
就像我们之前对`TextComponent`所做的那样，它可以从JSON读取其属性：

```cpp
void ButtonComponent::LoadProperties(const nlohmann::json& jsonObject)
{
    if (jsonObject.contains("rect"))
    {
        auto& rectObj = jsonObject["rect"];
        SetRect(glm::vec2(
            rectObj.value("x", 1.0f),
            rectObj.value("y", 1.0f)
        ));
    }
    if (jsonObject.contains("color"))
    {
        auto& colorObj = jsonObject["color"];
        SetColor(glm::vec4(
            colorObj.value("r", 1.0f),
            colorObj.value("g", 1.0f),
            colorObj.value("b", 1.0f),
            colorObj.value("a", 1.0f)
        ));
    }
    if (jsonObject.contains("hovered"))
    {
        auto& colorObj = jsonObject["hovered"];
        SetHoveredColor(glm::vec4(
            colorObj.value("r", 0.5f),
            colorObj.value("g", 0.5f),
            colorObj.value("b", 0.5f),
            colorObj.value("a", 0.5f)
        ));
    }
    if (jsonObject.contains("pressed"))
    {
        auto& colorObj = jsonObject["pressed"];
        SetPressedColor(glm::vec4(
            colorObj.value("r", 0.2f),
            colorObj.value("g", 0.2f),
            colorObj.value("b", 0.2f),
            colorObj.value("a", 0.2f)
        ));
    }
}
```

---

### 总结

我们刚刚构建了**基本的交互式UI系统**：

* 递归遍历画布中的所有UI组件，
* 检测鼠标位置和点击测试，
* 处理悬停、按下、释放和单击事件，
* 对颜色变化的视觉反应。

**恭喜你！**  
现在，您已经正式将 **用户输入** 和 **交互** 添加到自定义UI系统中 --   
对于任何引擎来说都是一个巨大的里程碑。