现在是时候回答一个重要问题了：  
**如果我们有许多行为完全不同的游戏对象怎么办？**

想象一下，一个物体在所有可能的方面都可能与另一个物体完全不同。现在，想象一下，我们的游戏中有 **成千上万** 个这样独特的物体。

我们该如何应对？

---

### The Answer: 组件

我们可以将独特的行为和特征分解为单独的 **实体** ，称为 **组件**。

因此，每个游戏对象都可以：

* 有孩子（其他对象）  
* 包含一组 **组件**，每个组件代表一个特定的行为或属性

示例：

* `MeshComponent` – 定义对象的外观及其构成
* `CarComponent` – 处理道路移动
* `AIComponent` – 控制NPC行为
* `PlayerComponent` – 处理玩家控制
* `CameraComponent` – 管理相机
* `LightComponent` – 控制照明

这种架构被称为 **基于组件的系统** ，它广泛应用于现代游戏引擎（如Unity和Unreal）。

---

## 逐步实施

### 1. 创建"Component"基类

在"Scene"文件夹中，创建两个文件：

* `Component.h`
* `Component.cpp`

将它们添加到`CMakeLists.txt`中，并将其包含在`eng.h`中。

在`Component.h`中：

```cpp
#pragma once

class GameObject; // Forward declaration

class Component 
{
public:
    virtual ~Component() = default;
    virtual void Update(float deltaTime) = 0;

    GameObject* GetOwner() const { return m_owner; }

protected:
    GameObject* m_owner = nullptr;

    friend class GameObject;
};
```

在`Component.cpp `中，如果需要，实现`GetOwner（）`（在头文件中已经很简单了）。

---

### 2. 为`GameObject`添加组件支持

在`GameObject.h`中，包含组件头：

```cpp
#include "scene/component.h"
```

添加一个容器来存储组件：

```cpp
std::vector<std::unique_ptr<Component>> m_components;
```

在`GameObject`的`Update（）`方法中，遍历所有组件：

```cpp
for (auto& component : m_components) 
{
    component->Update(deltaTime);
}
```

添加一个`AddComponent`方法：

```cpp
void AddComponent(Component* component) 
{
    m_components.emplace_back(component);
    component->m_owner = this;
}
```

---

### 3. 创建`MeshComponent`

在`engine/source/sescene`中，创建一个新文件夹：`components`。

在该文件夹中，创建：

* `MeshComponent.h`
* `MeshComponent.cpp`

将它们添加到`CMakeLists.txt `和`eng.h`中。

在`MeshComponent.h `中：

```cpp
#pragma once

#include "scene/Component.h"
#include <memory>

class Material;
class Mesh;

class MeshComponent : public Component 
{
public:
    MeshComponent(std::shared_ptr<Material> material, std::shared_ptr<Mesh> mesh);
    void Update(float deltaTime) override;

private:
    std::shared_ptr<Material> m_material;
    std::shared_ptr<Mesh> m_mesh;
};
```

在`MeshComponent.cpp`中:

```cpp
#include "scene/components/MeshComponent.h"
#include "render/Material.h"
#include "render/Mesh.h"
#include "render/RenderQueue.h"
#include "scene/GameObject.h"
#include "Engine.h"

MeshComponent::MeshComponent(std::shared_ptr<Material> material, std::shared_ptr<Mesh> mesh)
    : mMaterial(material), mMesh(mesh) {}

void MeshComponent::Update(float deltaTime) 
{
    if (!m_material || !m_mesh) 
    {
        return;
    }

    RenderCommand command;
    command.material = m_material.get();
    command.mesh = m_mesh.get();
    command.modelMatrix = GetOwner()->GetWorldTransform();

    auto& renderQueue = Engine::GetInstance().GetRenderQueue();
    renderQueue.Submit(command);
}
```

---

### 4. 重构`TestObject`以使用`MeshComponent`

现在进入你的`TestObject`类：

* 删除`m_material`和`m_mesh`成员变量
* 删除`Update（）`中手动创建的渲染命令

相反，在构造函数或`Init（）`中执行以下操作：

```cpp
auto material = std::make_shared<Material>();
auto mesh = std::make_shared<Mesh>();

AddComponent(new eng::MeshComponent(material, mesh));
```

---

### 5. 测试实施

编译并运行游戏。  
您将看到相同的结果：屏幕上出现一个矩形，对输入做出反应并像以前一样移动。

但现在，所有网格和材质渲染逻辑都被清晰地分离到“MeshComponent”中。

---

### 摘要

* 您已经了解了 **组件** 是什么，以及它如何帮助清晰地分离逻辑。
* 您添加了一个基类`Component`。
* 您更新了`GameObject`以支持多个组件。
* 您构建了一个处理渲染的`MeshComponent`。
* 您已经重构了测试对象以使用这个新系统。

> 这种结构为您在游戏中处理数千个独特的对象提供了强大、灵活和可扩展的基础。