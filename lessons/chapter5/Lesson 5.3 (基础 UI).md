## 构建UI基础和类型层次结构系统

好的 -- 既然我们已经有了物理、渲染和2D/3D逻辑，是时候再次扩展我们的视野了。  
下一步自然是理解 **用户界面（UI）** 和 **用户输入系统** 。

UI元素是特殊的：它们本质上是2D的，但它们有很强的约束 —— 例如，它们 **不旋转** ，并且它们几乎总是 **矩形** 。  
因此，让我们首先同意一件事：引擎中的所有UI元素 **将从单个基础组件** 继承。

---

### 步骤1：创建基本UI组件

让我们从创建基类开始。

1. 转到文件夹：`scene/components/`。
2. 在其中，创建一个新文件夹：**ui** - 这将存储所有与ui相关的组件。
3. 在其中，创建两个新文件：

    * `UIElementComponent.h`
    * `UIElementComponent.cpp`

在这些文件中，声明：

```cpp
class UIElementComponent : public Component 
{
protected:
    glm::vec2 m_pivot = glm::vec2(0.5f);
};
```

这个轴心点的工作方式就像我们用于精灵的轴心点一样 —— 它定义了UI元素的定位点。

不要忘记将这些文件添加到 **CMake** 和 **引擎包含路径**。

好极了 - 现在我们有了所有UI元素的基础。

---

### 步骤2：画布-所有UI的根

由于UI元素是特殊的，因此它们都将位于场景层次结构中的 **专用子树** 下 —— 一个管理所有与UI相关的内容的根对象。

这是一种常见的设计模式。  
它允许我们独立管理多个UI屏幕（菜单、HUD等）- 根据需要激活、隐藏或禁用整个UI集。

因此，让我们创建一个**Canvas**组件 —— UI系统的根。

在同一“ui”文件夹中，再创建两个文件：

* `CanvasComponent.h`
* `CanvasComponent.cpp`

内部，声明：

```cpp
class CanvasComponent : public Component 
{
public:
    void Update(float deltaTime) override;
    void Render(UIElementComponent* element);
};
```

---

### 步骤3：画布逻辑-递归渲染

现在，在`Update（）`方法中，我们将使画布循环通过其所有子对象，并呈现包含`UIElementComponent`的任何子对象。

```cpp
void CanvasComponent::Update(float deltaTime) 
{
    const auto children = m_owner->GetChildren();
    for (const auto& child : children) {
        if (auto comp = child->GetComponent<UIElementComponent>()) 
        {
            Render(comp);
        }
    }
}
```

转到`GameObject`并实现`GetChildren（）`方法：

```cpp
const std::vector<std::unique_ptr<GameObject>>& GameObject::GetChildren() const
{
    return m_children;
}
```

现在，让我们实现`Render（）`：

```cpp
void CanvasComponent::Render(UIElementComponent* element) 
{
    if (!element) 
    {
        return;
    }

    // 呈现此元素
    element->Render(this);

    // 然后递归呈现也是UI元素的所有子元素
    const auto children = element->GetOwner()->GetChildren();
    for (const auto& child : children) 
    {
        if (auto comp = child->getComponent<UIElementComponent>()) 
        {
            Render(comp);
        }
    }
}
```

因此，`CanvasComponent`现在充当中央管理器 - 它遍历场景图，查找所有UI元素，并递归调用它们的`Render（）`方法。

---

### 步骤4：使用虚 render 函数扩展`UIElementComponent`

让我们回到`UIElementComponent`并添加**虚** render 函数，这样所有派生的UI组件都可以实现自己的绘图逻辑：

```cpp
virtual void Render(CanvasComponent* canvas) {}
```

现在，任何子类（按钮、文本、图像）都可以覆盖 render 来绘制自己。

---

### 步骤5：第一个真实UI元素-TextComponent

让我们实现第一个UI组件：**TextComponent**。

在`scene/components/ui/`中，创建：

* `TextComponent.h`
* `TextComponent.cpp`

不要忘记在**CMake**中注册它们并包含文件。

在头文件中，声明：

```cpp
class TextComponent : public UIElementComponent 
{
public:
    COMPONENT(TextComponent)

    void Render(CanvasComponent* canvas) override;

private:
    std::string m_text;

public:
    void SetText(const std::string& text) { m_text = text; }
    const std::string& GetText() const { return m_text; }
};
```

现在，`Render（）`方法可以保持为空 - 我们将在稍后实际开始绘制文本时填充它。

---

### 步骤6：扩展类型层次结构-父注册表系统

现在，让我们回答一个重要的问题：  
当画布迭代场景对象时，它调用`GetComponent<UIElementComponent>()`。  
但我们的`TextComponent`是 **从`UIElementComponent'派生的**。  
系统如何自动识别`TextComponent`**是**`UIElementComponent`的一种？  

为了解决这个问题，我们需要在`ComponentFactory`中有一个**父类型层次结构**。

---

### 步骤7：实现父映射

我们将使用映射存储类型之间的父子关系。

在`ComponentFactory`中，添加：

```cpp
std::unordered_map<size_t, std::vector<size_t>> m_parentMap;
```

* **Key:** 组件的类型ID。
* **Value:** 表示其直接父级的类型ID的向量。

这样，我们可以向上遍历继承链，以查看组件是否从另一个派生。

---

### 步骤8：扩展注册

注册新组件类型时，默认情况下它是`Component`的子级。  
但有时，我们希望显式指定**不同的父**（如`UIElementComponent`）。

因此，让我们扩展注册系统。

添加第二个模板化注册方法：

```cpp
template<typename T, typename ParentT>
void RegisterObject() 
{
    m_creators.emplace(name, std::make_unique<ComponentCreator<T>>());
    m_parentMap[T::TypeId()].push_back(Component::StaticTypeId<ParentT>());
}
```

还将父映射中的注册添加到基本的`RegisterObject`函数：
```cpp
template<typename T>
void RegisterObject() 
{
    m_creators.emplace(name, std::make_unique<ComponentCreator<T>>());
    m_parentMap[T::TypeId()].push_back(Component::StaticTypeId<Component>());
}
```


现在，我们可以使用**特定的父**注册组件。

---

### 步骤9：新宏：`COMPONENT_2`

让我们为指定父类型的组件引入一个新宏：

```cpp
#define COMPONENT_2(ComponentClass, ParentComponentClass) \
public: \
    static size_t TypeId() { return eng::Component::StaticTypeId<ComponentClass>(); } \
    size_t GetTypeId() const override { return TypeId(); } \
    static void Register() { eng::ComponentFactory::GetInstance().RegisterComponent<ComponentClass, ParentComponentClass>(std::string(#ComponentClass)); }
```

这与常规的`COMPONENT`宏类似，但将类显式链接到其父级。

现在，在`TextComponent`中，将旧宏替换为以下宏：

```cpp
COMPONENT_2(TextComponent, UIElementComponent)
```

---

还使`Component::Update()`函数从纯虚拟实现变为默认实现：

```cpp
void Component::Update(float deltaTime)
{
}
```

---

### 步骤10:递归父项检查 - `HasParent`

在`ComponentFactory`中，添加：

```cpp
bool HasParent(size_t objectType, size_t parentType);
```

实现：

```cpp
bool ComponentFactory::HasParent(size_t objectType, size_t parentType) 
{
    auto record = m_parentMap.find(objectType);
    if (record == m_parentMap.end()) 
    {
        return false;
    }

    auto& parents = record->second;
    if (std::find(parents.begin(), parents.end(), parentType) != parents.end())
    {
        return true;
    }

    for (auto p : parents) 
    {
        if (HasParent(p, parentType)) 
        {
            return true;
        }
    }

    return false;
}
```

该函数递归地爬升继承树，直到找到（或找不到）请求的父级。

---

### 步骤11：将层次结构集成到`GameObject:：GetComponent`

最后，在`GameObject`的`GetComponent<T>()`方法中，更新检查：

```cpp
if (component->GetTypeId() == typeId ||
    ComponentFactory::GetInstance().HasParent(component->getTypeId(), typeId)) 
    {
    return static_cast<T*>(component.get());
}
```

现在`GetComponent<UIElementComponent>()`将正确返回`TextComponent`。

---

### 步骤12：注册所有内容

在`Scene:：RegisterTypes（）`中，注册新组件：

```cpp
CanvasComponent::RegisterType();
UIElementComponent::RegisterType();
TextComponent::RegisterType();
```

---

### 步骤13：了解结果

让我们总结一下这个系统现在是如何工作的。

假设您创建了一个`TextComponent`并将其附加到`GameObject`。

* 它注册为`UIElementComponent`的子类。
* 当`CanvasComponent`迭代子级时，它会查找`UIElementComponent'。
* `ComponentFactory`层次结构告诉它`TextComponent`**是从**`UIElementComponent`派生的。
* 画布在其上调用`Render()`。

这是完整的逻辑链，它一直递归工作到基本的“Component”。

---

### 最后的想法

现在，我们已经为UI系统构建了整个**基础**：

* 我们了解UI元素是什么，以及它们如何适应场景层次。
* 我们创建了一个“CanvasComponent”来管理它们。
* 我们实现了一个支持继承检查的灵活类型层次结构。
* 我们准备了第一个UI元素 - `TextComponent`。

在 **下一课** 中，我们将最终开始实现屏幕上文本的实际呈现逻辑。

请继续关注 - 它将被视觉化！