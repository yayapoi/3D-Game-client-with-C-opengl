# 加载整个场景（第1部分）

我们将做一些有趣的事情：**学习一次加载整个场景**。  
我们已经知道如何加载单个对象，并且我们有很多资源，所以让我们将我们所学到的关于资源加载的一切结合起来，并将其作为一个大场景加载。  
我们的主要目标是复制（或非常接近）我们之前手工构建的场景。

---

在我们开始之前，让我们回到`PhysicsComponent`。正如您所记得的，我们在基于物理更新对象时使用了局部位置和旋转。  
让我们解决这个问题。转到`GameObject`并添加以下函数：

```cpp
void GameObject::SetWorldPosition(const glm::vec3& pos)
{
    if (m_parent)
    {
        glm::mat4 parentWorld = m_parent->GetWorldTransform();
        glm::mat4 invParentWorld = glm::inverse(parentWorld);
        glm::vec4 localPos = invParentWorld * glm::vec4(newWorldPos, 1.0f);
        SetPosition(glm::vec3(localPos) / localPos.w);
    }
    else
    {
        SetPosition(newWorldPos);
    }
}

glm::quat GameObject::GetWorldRotation()
{
    if (m_parent)
    {
        return m_parent->GetWorldRotation() * m_rotation;
    }
    else
    {
        return m_rotation;
    }
}

void GameObject::SetWorldRotation(const glm::quat& rot)
{
    if (m_parent)
    {
        glm::quat parentWorldRot = m_parent->GetWorldRotation();
        glm::quat invParentWorldRot = glm::inverse(parentWorldRot);
        glm::quat newLocalRot = invParentWorldRot * rot;
        SetRotation(newLocalRot);
    }
    else
    {
        SetRotation(worldRot);
    }
}
```

然后转到`PhysicsComponent:：Init（）`并写：

```cpp
const auto pos = m_owner->GetWorldPosition();
const auto rot = m_owner->GetWorldRotation();
```

然后转到`PhysicsComponent:：Update（）`并写：

```cpp
m_owner->SetWorldPosition(m_rigidBody->GetPosition());
m_owner->SetWorldRotation(m_rigidBody->GetRotation());
```
---

## 1）按常规方式启动：在`Scene`中添加`Load`功能

打开`Scene`类并添加：

```cpp
// Scene.h
class Scene 
{
public:
    static std::shared_ptr<Scene> Load(const std::string& path);

private:
    //将从JSON加载一个对象并将其附加到“parent”（或根，如果parent==nullptr）
    void LoadObject(const nlohmann::json& objectJson, GameObject* parent);
};
```

现在，按照我们之前加载程序的方式实现它：

1. **读取文件内容**：

```cpp
// Scene.cpp
#include <nlohmann/json.hpp>
using nlohmann::json;

std::shared_ptr<Scene> Scene::Load(const std::string& path) 
{
    const std::string contents = Engine::GetInstance().GetFileSystem().LoadAssetFileText(path);
    if (contents.empty())
    {
        return nullptr;
    }
```

2. **解析JSON**：

```cpp
    json j = json::parse(contents);
    if (j.empty())
    {
        return nullptr;
    }
```

3. **创建场景实例**：

```cpp
    auto result = std::make_shared<Scene>();
```

我们已经准备好使用JSON了。现在，在继续之前，让我们讨论一下**场景格式**。

---

## 2）场景格式（JSON）

我们将以 **JSON** 存储场景，因为这已经是我们在引擎中的统一资源格式。

* 转到`assets/`，创建子文件夹`scenes/`。
* 在那里放一个文件，例如`scene.sc`。
* 根JSON对象将具有：

    * `name` -- 可选的场景名称，以防万一。
    * `objects` —— 场景对象数组（直接存在于场景中的顶级对象）。

最小格式：

```json
{
  "name": "MyScene",
  "objects": []
}
```

让我们逐渐开始描述它。目前，`objects`是一个数组。我们会回来填的。

返回`Scene::Load`并读取根字段：

```cpp
    const std::string sceneName = j.value("name", "NoName");

    if (j.contains("objects") && j["objects"].is_array()) 
    {
        const auto& objects = j["objects"];
        // 这些是顶级项目（没有父项）
        for (const auto& obj : objects) 
        {
            result->LoadObject(obj, nullptr);
        }
    }

    return result;
}
```

---

## 3）`LoadObject`：从JSON加载单个对象

提前声明：

```cpp
void LoadObject(const nlohmann::json& objectJson, GameObject* parent);
```

现在实现它。我们将逐步深入到对象描述本身。

**Name.** 每个对象都应该有一个`name`。  
在我们的JSON中，对于第一个对象，让我们放一个悬挂在空间中的随机立方体：

```json
{
  "name": "Object_0"
}
```

实现：

```cpp
void Scene::LoadObject(const nlohmann::json& objectJson, GameObject* parent) 
{
    const std::string name = objectJson.value("name", "Object");

    GameObject* gameObject = nullptr;
```

我们最终将支持 **三种对象类型**：

1. 我们通过场景创建的 **标准** 对象（`GameObject`）。
2. **从GLTF文件加载的对象**。
3. 从`GameObject`派生的 **自定义** 对象。

我们需要区分它们。最简单的方法是在JSON中添加一个`type`字段。  
如果`type`不存在，我们将其视为默认的`GameObject`。

```cpp
    if (objectJson.contains("type")) 
    {
        const std::string type = objectJson.value("type", "");
        // Non-default path (GLTF or custom) — we’ll implement this later.
        // Example placeholder:
        // gameObject = CreateObjectFromType(type, name, parent);
    } 
    else 
    {
        // Default object
        gameObject = CreateObject(name, parent);
    }

    if (!gameObject)
    {
        return; // could not create object — bail out and continue
    }
```

接下来，加载所有游戏对象共有的**变换**：**位置**、**旋转**、**缩放**。

我们将通过以下方式扩展JSON：

* `position` holding `{ x, y, z }`
* `rotation` holding `{ x, y, z, w }`
* `scale` holding `{ x, y, z }`

```cpp
    // Position
    if (objectJson.contains("position") && objectJson["position"].is_object()) 
    {
        const auto& p = objectJson["position"];
        glm::vec3 pos(
            p.value("x", 0.0f),
            p.value("y", 0.0f),
            p.value("z", 0.0f)
        );
        gameObject->SetPosition(pos);
    }

    // Rotation (quaternion)
    if (objectJson.contains("rotation") && objectJson["rotation"].is_object()) 
    {
        const auto& r = objectJson["rotation"];
        glm::quat rot(
            r.value("w", 1.0f), // glm::quat(w, x, y, z)
            r.value("x", 0.0f),
            r.value("y", 0.0f),
            r.value("z", 0.0f)
        );
        gameObject->SetRotation(rot);
    }

    // Scale
    if (objectJson.contains("scale") && objectJson["scale"].is_object()) 
    {
        const auto& s = objectJson["scale"];
        glm::vec3 scl(
            s.value("x", 1.0f),
            s.value("y", 1.0f),
            s.value("z", 1.0f)
        );
        gameObject->SetScale(scl);
    }
```

最后，组件。

---

## 4）JSON中的组件和迭代

每个对象都可以有组件。在JSON文件中添加一个字段`components`，这是一个数组。  
对于我们的第一个简单盒子，它只有一个组件：**`MeshComponent`**。

JSON示例：

```json
{
  "name": "Object_0",
  "position": { "x": 10, "y": 0, "z": 0 },
  "rotation": { "x": 0, "y": 0, "z": 0, "w": 1 },
  "scale":    { "x": 1, "y": 1, "z": 1 },
  "components": [
    {
      "type": "MeshComponent"
        // 材料/网格字段将在LoadProperties（）中读取
    }
  ]
}
```

回到“LoadObject”，迭代并加载：

```cpp
    if (objectJson.contains("components") && objectJson["components"].is_array()) 
    {
        const auto& comps = objectJson["components"];
        for (const auto& compJson : comps) 
        {
            const std::string type = compJson.value("type", "");
            // create compoent by type
        }
    }
}
```

为了实现这一点，我们必须“教导”系统存在哪些组件类型以及如何创建它们。  
这就把我们带到了一个小小的**工厂**。

---

## 5）组件工厂（创建者类+注册表）

我们将建立一个小型工厂，可以根据组件的 **类型名称** 创建组件。

1. 一种具有单个纯虚拟方法的基础创建者：

```cpp
// Component.h
struct ComponentCreatorBase 
{
    virtual ~ComponentCreatorBase() = default;
    virtual Component* CreateComponent() = 0;
};
```

2. 混合类型的模板创建器：

```cpp
template <typename T>
struct ComponentCreator : ComponentCreatorBase 
{
    Component* CreateComponent() override 
    { 
        return new T(); 
    }
};
```

3. 单例工厂本身：

```cpp
class ComponentFactory 
{
public:
    static ComponentFactory& GetInstance() 
    {
        static ComponentFactory instance;
        return instance;
    }

    template <typename T>
    void RegisterObject(const std::string& name) 
    {
        m_creators.emplace(name, std::make_unique<ComponentCreator<T>>());
    }

    Component* CreateComponent(const std::string& typeName) 
    {
        auto it = m_creators.find(typeName);
        if (it == m_creators.end())
            return nullptr;
        return it->second->CreateComponent();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<ComponentCreatorBase>> m_creators;
};
```
---

## 6）使组件注册变得容易

为每种组件类型添加**静态注册函数**。您可以重用一个小助手宏：

```cpp
//在类定义后的每个组件标头中：
#define COMPONENT(ComponentClass) \
public: \
    static size_t TypeId() { return Component::StaticTypeId<ComponentClass>(); } \
    size_t GetTypeId() const override { return TypeId(); } \
    static void Register() { eng::ComponentFactory::GetInstance().RegisterObject<ComponentClass>(std::string(#ComponentClass)); }
```

其思想是：`Register（）`是一个 **静态** 方法，在初始化过程中被调用 **一** 次；当您看到字符串`“MeshComponent”`时，它会告诉工厂，构造`MeshComponent`。

---

## 7）组件的默认构造函数+设置器

因为我们的`ComponentCreator<T>`使用**默认构造函数**构造组件，所以所有注册的组件类型都必须有一个。

* `MeshComponent`：以前在其构造函数中需要一种材质和一个网格。  
    给它`MeshComponent() = default;`并添加setter：

    ```cpp
    void SetMaterial(std::shared_ptr<Material> m);
    void SetMesh(std::shared_ptr<Mesh> mesh);
    ```

* `PhysicsComponent`：之前取了一个`Rigidbody`。  
    给它一个默认构造函数和一个setter：

    ```cpp
    void SetRigidbody(const std::shared_ptr<Rigidbody>& body);
    ```

浏览你的组件列表，确保每个组件都有一个默认构造函数。

---

## 8）从JSON加载每个组件的属性

一些组件用于通过其构造函数接收参数。我们将通过在基类中添加一个**虚拟**负载钩子来补偿：

```cpp
// Component.h
class Component 
{
public:
    virtual ~Component() = default;
    virtual void LoadProperties(const nlohmann::json& j) { /*默认值：不执行任何操作*/}
};
```

请确保在需要的地方`#include <nlohmann/json.hpp>`。

如果你的游戏/CMake包含引擎的第三方JSON头文件，请添加：

```
# In the game's CMakeLists.txt
include_directories(engine/thirdparty/json)
```

现在，当我们通过工厂创建组件时，我们立即调用`component->LoadProperties(compJson)`来读取网格/材料/刚体等字段。

---

## 9）在哪里调用注册（引擎与客户端）

我们需要一个在启动时运行**一次**的地方来注册所有组件类型：

* 在**引擎端**，添加`static void Scene::RegisterTypes();`并从`Engine:：Init（）`调用它。
* 在**客户端/应用程序端**，让应用程序通过重写虚拟方法添加自己的**拥有的**类型。

```cpp
// Application.h
class Application 
{
public:
    virtual ~Application() = default;
    virtual void RegisterTypes() {} // 客户端可以重写以注册自定义类型
};
```

在`Engine:：Init（）`中（在您确认`m_application`存在之后）：

```cpp
Scene::RegisterTypes();        // 注册所有内置引擎组件
if (m_application)
    mApplication->RegisterTypes();  // 让客户端注册其自定义组件
```

现在实现`Scene:：RegisterTypes（）`以包含每个内置组件并调用其`Register（）`：

```cpp
//Scene.cpp（或专门注册TU）
#include "scene/components/AnimationComponent.h"
#include "scene/components/CameraComponent.h"
#include "scene/components/LightComponent.h"
#include "scene/components/MeshComponent.h"
#include "scene/components/PhysicsComponent.h"
#include "scene/components/PlayerControllerComponent.h"

void Scene::RegisterTypes() 
{
    AnimationComponent::Register();
    CameraComponent::Register();
    LightComponent::Register();
    MeshComponent::Register();
    PhysicsComponent::Register();
    PlayerControllerComponent::Register();
}
```

这确保了工厂知道如何在场景加载期间按**名称**创建所有内置组件。

---

## 10）回到对象加载：按`type`创建组件

现在注册已经就绪，`LoadObject`中的组件加载循环开始工作：

```cpp
if (objectJson.contains("components") && objectJson["components"].is_array()) 
{
    for (const auto& compJson : objectJson["components"]) 
    {
        const std::string type = compJson.value("type", "");
        Component* component = ComponentFactory::GetInstance().CreateComponent(type);
        if (component) 
        {
            component->LoadProperties(compJson);     // per-component JSON
            gameObject->AddComponent(component);
        }
    }
}
```

`MeshComponent:：LoadProperties（）`将读取`"mesh"`和`"material"`，并调用`SetMesh(...)`, `SetMaterial(...)`。

（指定了`"type"`的对象，例如GLTF或`GameObject`的自定义子类，将在稍后实现该分支时处理。）

---

## 12）总结（我们在这一部分取得的成就）

* 添加了`Scene:：Load（path）`，用于读取JSON文件并构建整个场景。
* 为场景定义了**稳定的JSON格式**：“名称”、“对象”（每个对象都有“位置”、“旋转”、“缩放”、“组件”）。
* 实现了`LoadObject(json,parent)`，创建默认的`GameObject`（非默认类型延迟），应用转换并加载组件。
* 使用创建者和注册表构建了一个 **ComponentFactory**，以按名称 **实例化组件**。
* 确保组件具有 **默认构造函数** + 用于先前构造函数注入数据的设置器。
* 添加了`Component:：LoadProperties（json）`来加载每个组件的数据。
* 引入了 **引擎端**(`Scene::RegisterTypes`)和**客户端**(`Application::RegisterTypes`)注册，两者都是从`Engine::Init()`调用的。

现在就这样。**待续…**（GLTF/自定义对象创建将在下一部分讨论。）
