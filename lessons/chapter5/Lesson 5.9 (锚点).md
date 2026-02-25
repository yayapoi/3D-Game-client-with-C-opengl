# 锚点 + 枢轴 +`RectTransformComponent` - 完整UI布局传递

我们已经有了交互式UI（按钮、文本）。现在，我们将添加一个**核心布局概念**，用于真实引擎：**anchors**。简而言之：

* **Pivot** - 元素自己的矩形内的本地原点*（用于绘制/定位该元素）。
* **Anchor** - 父UI元素的**children**的原点（其中子元素的定位从父元素中开始）。

因此，如果`A`是父级，`B`是子级：

* `A`有一个**pivot**（用于将`A`放在屏幕上）。
* `A`具有**anchor**（由后代使用）。
* `B`相对于**`A`的**anchor**有自己的**pivot**（用于放置`B`）**。

这为我们提供了在父级大小/位置更改时的响应行为。

---

## 1）添加`RectTransformComponent`

**文件：**  
`scene/components/ui/RectTransformComponent.h`  
`scene/components/ui/RectTransformComponent.cpp`

**私有字段：**

```cpp
glm::vec2 m_size   {0.0f, 0.0f};   // 此UI框的宽度、高度
glm::vec2 m_anchor {0.0f, 0.0f}; // 父级（子原点）内的标准化[0..1]点
glm::vec2 m_pivot  {0.0f, 0.0f}; // 在自身内规范化[0..1]原点
```

**公共API：**

```cpp
// getters / setters
const glm::vec2& GetSize()   const;
const glm::vec2& GetAnchor() const;
const glm::vec2& GetPivot()  const;

void SetSize  (const glm::vec2& s);
void SetAnchor(const glm::vec2& a);
void SetPivot (const glm::vec2& p);

// JSON loader
void LoadProperties(const nlohmann::json& jsonObject) override
{
    if (jsonObject.contains("size"))
    {
        auto& sizeObj = jsonObject["size"];
        SetSize(glm::vec2(
            sizeObj.value("x", 1.0f),
            sizeObj.value("y", 1.0f)
        ));
    }
    if (jsonObject.contains("anchor"))
    {
        auto& anchorObj = jsonObject["anchor"];
        SetAnchor(glm::vec2(
            anchorObj.value("x", 0.0f),
            anchorObj.value("y", 0.0f)
        ));
    }
    if (jsonObject.contains("pivot"))
    {
        auto& pivotObj = jsonObject["pivot"];
        SetPivot(glm::vec2(
            pivotObj.value("x", 0.0f),
            pivotObj.value("y", 0.0f)
        ));
    }
}
// Reads: "size": { "x":..., "y":... }, "anchor": { "x":..., "y":... }, "pivot": { "x":..., "y":... }

//屏幕空间解析器（关键方法）
glm::vec2 GetScreenPosition(); // 返回此对象的锚点对齐的*screen*位置
```

**实现说明：**
```cpp
glm::vec2 RectTransformComponent::GetScreenPosition() 
{
    auto* parent = GetOwner()->GetParent();
    // 如果没有父级，或者父级没有RectTransform，
    // 我们认为我们已经退出UI树（例如，点击画布边界）
    if (!parent || !parent->GetComponent<RectTransformComponent>()) 
    {
        return GetOwner()->GetPosition2D();
    }

    auto rect = parent->GetComponent<RectTransformComponent>();
    // 屏幕空间中的父锚点位置：
    //parentScreen+（parentAnchor-parentPivot）*parentSize
    glm::vec2 parentAnchorPos =
        rect->GetScreenPosition()
      + (rect->GetAnchor() - rect->GetPivot()) * rect->GetSize();

    // Final = local 2D pos + resolved parent anchor position
    return GetOwner()->GetPosition2D() + parentAnchorPos;
}
```

**构建集成：**

* 添加到 **CMakeList**（sources + include）。
* 添加到 **Scene::RegisterTypes**：`RectTransformComponent::Register();`
* 在需要时包括头文件。

---

## 2）将枢轴/锚从旧类中移出

我们**从以下位置删除所有枢轴/锚定字段**：

* `UIElementComponent`（仅保留虚拟交互方法）。
* `ButtonComponent`（大小/rect现在将存在于RectTransform中）。
* `TextComponent`（支持透视的放置将查询RectTransform）。

> RectTransform现在是**用于UI大小调整/布局的唯一真实来源**。

---

## 3）更新`ButtonComponent`

### 3.1 命中测试

旧代码使用了所有者位置 + 局部`m_rect`+`m_pivit`。替换为RectTransform：

```cpp
bool ButtonComponent::HitTest(const glm::vec2& pos) 
{
    auto rt = GetOwner()->GetComponent<RectTransformComponent>();
    if (!rt) 
    {
        return false;
    }

    glm::vec2 ownerPos = rt->GetScreenPosition();
    // 左下角:
    glm::vec2 p1 = ownerPos - rt->GetSize() * rt->GetPivot();
    // 右上角:
    glm::vec2 p2 = p1 + rt->GetSize();

    return (p1.x <= pos.x && p2.x >= pos.x &&
            p1.y <= pos.y && p2.y >= pos.y);
}
```

### 3.2渲染

我们不再使用`m_rect`；我们使用RectTransform：

```cpp
void ButtonComponent::Render(CanvasComponent* canvas) 
{
    if (!canvas) 
    {
        return;
    }

    auto rt = GetOwner()->GetComponent<RectTransformComponent>();
    if (!rt) 
    {
        return;
    }

    glm::vec2 ownerPos = rt->GetScreenPosition();
    ownerPos -= rt->GetSize() * rt->GetPivot();
    glm::vec2 p1 = ownerPos;
    glm::vec2 p2 = ownerPos + rt->GetSize();

    //m_currentColor是指向当前状态颜色的指针（正常/悬停/按下）
    canvas->drawRect(p1, p2, *m_currentColor);
}
```

### 3.3 加载特性清理

移除 `rect{ x, y }` 从`ButtonComponent:：LoadProperties` - 大小现在由`RectTransformComponent`读取。  
并删除`SetRect()/GetRect()`方法。

---

## 4）更新`TextComponent`

我们有一个基于枢轴计算起始笔位置的助手；用RectTransform替换内部枢轴：

```cpp
glm::vec2 TextComponent::GetPivotPos() 
{
    auto rt = GetOwner()->GetComponent<RectTransformComponent>();
    glm::vec2 pos = rt ? rt->GetScreenPosition() : GetOwner()->GetWorldPosition2D();

    // 度量呈现文本的边界（与之前相同）
    glm::vec2 rect(0.0f); // rect.x = total advance width, rect.y = max glyph height
    for (char c : m_text) 
    {
        const auto& desc = m_font->GetGlyphDescription(c);
        rect.x += static_cast<float>(desc.advance);
        rect.y = std::max(rect.y, static_cast<float>(desc.height));
    }

    if (rt) 
    {
        pos -= rect * rt->GetPivot(); // 在文本框内通过枢轴移位
    }

    // 对齐到整数像素（避免文本模糊）
    pos.x = std::round(pos.x);
    pos.y = std::round(pos.y);
    return pos;
}
```

## 5）每帧更新`CanvasComponent`大小

在开始递归UI渲染之前，请确保画布知道屏幕大小：

```cpp
void CanvasComponent::Update(float deltaTime)
{
    if (!m_active) 
    {
        return;
    }

    if (auto rt = GetOwner()->GetComponent<RectTransformComponent>()) 
    {
        auto& graphics = Engine::GetInstance().GetGraphicsAPI();
        const auto& viewport = graphics.GetViewport();
        rt->SetSize(glm::vec2(static_cast<float>(viewport.width),
                               static_cast<float>(viewport.height) ));
    }

    // ... 然后开始渲染（）；渲染（）；Flush（）；（就像你已经做的那样）
}
```

画布是“根UI层”，因此将其RectTransform大小设置为**当前视口**每个帧都会保持一切响应。（它很简单，不是最理想的，但非常适合演示这个概念。）

---

## 6）画布描述（JSON）

打开`scene.sc`并正确设置RectTransforms。

### 6.1 主画布

```json
{
  "name": "MainCanvas",
  "components": [
    { "type": "CanvasComponent",
      "active": true
    },
    { "type": "RectTransformComponent",
      "anchor": { "x": 0.5, "y": 0.5 },      // 其子级的画布锚点位于中心
      "pivot":  { "x": 0.0, "y": 0.0 }       // 画布自己的轴（LL角）
    }
  ],
  "children": [ /* buttons below */ ]
}
```

> 加载时忽略`size`（画布从视口的每一帧设置它）。

### 6.2播放按钮（居中，上移）

```json
{
  "name": "PlayButton",
  "components": [
    { "type": "ButtonComponent" },
    { "type": "RectTransformComponent",
      "size":   { "x": 150, "y": 50 },
      "anchor": { "x": 0.5, "y": 0.5 },
      "pivot":  { "x": 0.5, "y": 0.5 }
    }
  ],
  "position": { "x": 0, "y": 50, "z": 0 },   // 相对于中心上移
  "children": [
    {
      "name": "Text",
      "components": [
        { "type": "TextComponent",
          "text": "Play",
          "font": { "path": "fonts/arial.ttf", "size": 24 }
        },
        { "type": "RectTransformComponent",
          "anchor": { "x": 0.5, "y": 0.5 },
          "pivot":  { "x": 0.5, "y": 0.5 }
        }
      ]
    }
  ]
}
```

### 6.3退出按钮（居中，向下移动）

```json
{
  "name": "QuitButton",
  "components": [
    { "type": "ButtonComponent" },
    { "type": "RectTransformComponent",
      "size":   { "x": 150, "y": 50 },
      "anchor": { "x": 0.5, "y": 0.5 },
      "pivot":  { "x": 0.5, "y": 0.5 }
    }
  ],
  "position": { "x": 0, "y": -50, "z": 0 },  // 相对于中心向下移动
  "children": [
    {
      "name": "Text",
      "components": [
        { "type": "TextComponent",
          "text": "Quit",
          "font": { "path": "fonts/arial.ttf", "size": 24 }
        },
        { "type": "RectTransformComponent",
          "anchor": { "x": 0.5, "y": 0.5 },
          "pivot":  { "x": 0.5, "y": 0.5 }
        }
      ]
    }
  ]
}
```

> 删除`ButtonComponent`JSON中的所有旧`rect`字段 - 大小现在属于`RectTransformComponent`。

---

## 7）结果和行为

* **Play**和**退出**都保持**居中**，并且具有垂直偏移，无论窗口大小如何。
* 调整窗口大小会更新画布大小 → 父锚点/枢轴转换 → 子节点们保持相对位置。
* 命中测试使用RectTransform框；渲染也使用RectTransform。
* 现在，您拥有**Anchor + Pivot**工作流，该工作流镜像了主要引擎。

---

**恭喜你！**您刚刚实现了一个真正的UI布局主干：**RectTransform with Anchors&Pivots**、分层分辨率和调整大小时的响应行为。这是游戏引擎中适当UI构建背后的“基本规则手册”。