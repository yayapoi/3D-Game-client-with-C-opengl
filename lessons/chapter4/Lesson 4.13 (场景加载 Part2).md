# 加载整个场景（第2部分）

在上一部分中，我们停止了在“Component”中添加“LoadProperties”方法。让我们从那里继续。

---

## 1）回到`scene.sc`：描述 MeshComponent

我们回到`scene.sc`和我们的基本对象。按照约定，我们正在加载一个`MeshComponent`。  
所以我们写：

```json
"type": "MeshComponent"
```

接下来，我们需要描述它的**网格**和**材料**。

* 对于材质：我们已经知道如何加载它，所以只需指定一个路径，例如：

```json
"material": "materials/brick.mat"
```

* 对于网格：我们添加一个`mesh`字段，它本身就是一个对象。  
    在内部，它将包含一个`type`（目前只有`"box"`），盒子网格由**extents**（XYZ值）描述。

例子：

```json
"mesh": {
  "type": "box",
  "x": 2,
  "y": 1,
  "z": 5
}
```

JSON中的`MeshComponent`条目看起来像这样：

```json
{
  "type": "MeshComponent",
  "material": "materials/brick.mat",
  "mesh": {
    "type": "box",
    "x": 2,
    "y": 1,
    "z": 5
  }
}
```

---

## 2）在`MeshComponent`中实现`LoadProperties `

打开`MeshComponent.h `并添加：

```cpp
void LoadProperties(const nlohmann::json& j) override;
```

实现：

```cpp
void MeshComponent::LoadProperties(const nlohmann::json& j) 
{
    // Material
    if (j.contains("material")) 
    {
        const std::string matPath = j.value("material", "");
        auto material = Material::Load(matPath);
        if (material)
        {
            SetMaterial(material);
        }
    }

    // Mesh
    if (j.contains("mesh")) 
    {
        const auto& m = j["mesh"];
        const std::string type = m.value("type", "box");
        if (type == "box") 
        {
            glm::vec3 extents(
                m.value("x", 1.0f),
                m.value("y", 1.0f),
                m.value("z", 1.0f)
            );
            auto mesh = Mesh::CreateBox(extents);
            SetMesh(mesh);
        }
    }
}
```

很好——现在我们的`MeshComponent`知道如何加载自己的数据。

---

## 3）返回`Scene::LoadObject`：对象级属性和子对象

记住：对象也可能有**自定义属性**，就像组件一样。  
所以，让我们为`GameObject`添加一个类似的机制。

1. 在`GameObject`中添加：

```cpp
virtual void LoadProperties(const nlohmann::json& j) { }
virtual void Init() { }
```

默认情况下，这些对象什么也不做，但用户定义的对象可以覆盖它们。

2. 在`Scene::LoadObject`中，在读取变换之后和加载组件之前：

```cpp
gameObject->LoadProperties(objectJson);
```

这保证了：

* 创建对象
* 读其属性
* 然后读取组件

3. 读取组件后，检查**子节点**：

```cpp
if (objectJson.contains("children") && objectJson["children"].is_array()) 
{
    for (const auto& childJson : objectJson["children"]) 
    {
        LoadObject(childJson, gameObject); // recursive
    }
}
```

4.最后，在所有子节点读完之后：

```cpp
gameObject->Init();
```

这允许对象在层次结构和组件准备就绪后执行自定义初始化。

---

## 4）下一种类的对象：玩家

现在，让我们向场景中添加一个新对象 —— **main player**。

JSON格式：

```json
{
  "name": "MainPlayer",
  "type": "Player",
  "position": { "x": 0, "y": 2, "z": 7 },
  "components": [
    { "type": "CameraComponent" },
    { "type": "PlayerControllerComponent" }
  ],
  "children": [
    {
      "name": "Gun",
      "type": "gltf",
      "path": "models/sten_gun_machine_carbine/scene.gltf",
      "position": { "x": 0.75, "y": -0.5, "z": -0.75 },
      "scale":    { "x": -1, "y": 1, "z": 1 }
    }
  ]
}
```

---

## 5）创建自定义对象（玩家）

回到“LoadObject”，回想一下：

```cpp
if (objectJson.contains("type")) 
{
    std::string type = objectJson.value("type", "");
    if (type == "gltf") 
    {
        // GLTF分支（见下文）
    } 
    else 
    {
        // 自定义用户定义对象
    }
}
```

我们需要支持用户定义的对象类型，如“Player”。  
我们将以与组件相同的方式解决它：使用**工厂**。

---

## 6）游戏对象工厂

1. 基础创建者：

```cpp
struct ObjectCreatorBase 
{
    virtual ~ObjectCreatorBase() = default;
    virtual GameObject* CreateGameObject() = 0;
};
```

2. 模板创建者：

```cpp
template <typename T>
struct ObjectCreator : ObjectCreatorBase 
{
    GameObject* CreateGameObject() override 
    { 
        return new T(); 
    }
};
```

3. 工厂单例：

```cpp
class GameObjectFactory 
{
public:
    static GameObjectFactory& GetInstance() 
    {
        static GameObjectFactory instance;
        return instance;
    }

    template <typename T>
    void RegisterObject(const std::string& name) 
    {
        m_creators.emplace(name, std::make_unique<ObjectCreator<T>>());
    }

    GameObject* CreateGameObject(const std::string& typeName) 
    {
        auto it = m_creators.find(typeName);
        if (it == m_creators.end())
            return nullptr;
        return it->second->CreateGameObject();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<ObjectCreatorBase>> m_creators;
};
```

4. 方便的宏：

```cpp
#define GAMEOBJECT(ObjectClass) \
public: \
    static void Register() { eng::GameObjectFactory::GetInstance().RegisterObject<ObjectClass>(std::string(#ObjectClass)); }
```

现在在`Player.h`中：

```cpp
class Player : public eng::GameObject 
{
    GAMEOBJECT(Player)
public:
    Player() = default;
    // ...
};
```

---

## 7）注册自定义对象

客户端游戏必须重写`Application:：RegisterTypes（）`以注册自定义游戏对象：

```cpp
void Game::RegisterTypes() override 
{
    Player::Register();
}
```

在引擎端注册类型后，引擎将在“engine:：Init（）”期间调用此函数。

---

## 8）按类型创建对象的场景方法

在`Scene`中，添加：

```cpp
GameObject* CreateObject(const std::string& type,
                         const std::string& name,
                         GameObject* parent = nullptr) 
{
    GameObject* obj = GameObjectFactory::GetInstance().CreateGameObject(type);
    if (obj) 
    {
        obj->SetName(name);
        obj->m_scene = this;
        obj->SetParent(parent);
    }
    return obj;
}
```

现在，在`LoadObject`分支中：

```cpp
gameObject = CreateObject(type, name, parent);
```

我们可以创建自定义用户类型，如`"Player"`。

---

## 9）GLTF对象

子节点类似于`"Gun"`这种属于`"gltf"`的类型。
我们需要从文件路径加载它们。

在`LoadObject`中：

```cpp
else if (type == "gltf") 
{
    std::string path = objectJson.value("path", "");
    gameObject = GameObject::LoadGLTF(path, this); // pass scene
    if (gameObject) 
    {
        gameObject->SetParent(parent);
        gameObject->SetName(name);
    }
}
```

注意：`GameObject:：LoadGLTF`应获得`Scene*`参数，以便加载的节点可以附加到它。  
如果场景指针为空，则返回`nullptr`。

在`LoadGLTF`中，调用`scene->CreateObject(...)`以确保正确的父对象/场景链接，而不是手动从引擎获取场景。

---

## 10）使用“Init”进行负载后调整

记住我们为什么添加`Init（）`。例如，`Player:：Init（）`可以：

* 隐藏不必要的节点，
* 抓住它的`AnimationComponent`，
* 设置玩家特定状态。

这发生在加载对象及其所有子对象 **之后**。

---

我们还需要将父对象的位置传递给`KinematicCharacterController`。将`const glm::vec3& position`参数添加到
`KinematicCharacterController`，将位置传递给变换器。然后在`PlayerControllerComponent:：Init（）`中传递`m_owner->GetWorldPosition`
给构造者。

---

到目前为止，我们创建了**Player**对象。玩家与物理链接上，但现在它只会**掉进虚空**，因为没有地面可以降落。  
让我们通过创建一个**地面对象**来解决这个问题。

---

## 11）JSON格式的地面对象

在`scene.cs`中，添加：

```json
{
  "name": "Ground",
  "position": { "x": 0, "y": -5, "z": 0 },
  "components": [
    {
      "type": "MeshComponent",
      "mesh": {
        "type": "box",
        "x": 20,
        "y": 2,
        "z": 20
      },
      "material": "materials/brick.mat"
    },
    {
      "type": "PhysicsComponent",
      "collider": {
        "type": "box",
        "x": 20,
        "y": 2,
        "z": 20
      },
      "body": {
        "mass": 0,
        "friction": 0.5,
        "type": "static"
      }
    }
  ]
}
```

* 网格是一个**20×2×20的盒子**，足够大，可以作为一个平台。
* 物理组件有：

    * 相同尺寸的箱式碰撞体，
    * 静态刚体（质量=0，摩擦力=0.5）。

---

## 12）实现`PhysicsComponent::LoadProperties`

现在让我们让`PhysicsComponent`能够从JSON加载自己。

```cpp
void PhysicsComponent::LoadProperties(const nlohmann::json& j) 
{
    std::shared_ptr<Collider> collider;

    // Collider
    if (j.contains("collider")) 
    {
        const auto& col = j["collider"];
        std::string type = col.value("type", "");

        if (type == "box") 
        {
            glm::vec3 extents(
                col.value("x", 1.0f),
                col.value("y", 1.0f),
                col.value("z", 1.0f)
            );
            collider = std::make_shared<BoxCollider>(extents);
        }
        else if (type == "sphere") 
        {
            float radius = col.value("r", 1.0f);
            collider = std::make_shared<SphereCollider>(radius);
        }
        else if (type == "capsule") 
        {
            float radius = col.value("r", 1.0f);
            float height = col.value("h", 1.0f);
            collider = std::make_shared<CapsuleCollider>(radius, height);
        }
    }

    if (!collider)
    {
        return;
    }

    // RigidBody
    std::shared_ptr<Rigidbody> rigidbody;
    if (j.contains("body")) 
    {
        const auto& body = j["body"];

        float mass = body.value("mass", 0.0f);
        float friction = body.value("friction", 0.5f);
        std::string typeStr = body.value("type", "static");

        BodyType type = BodyType::Static;
        if (typeStr == "dynamic") type = BodyType::Dynamic;
        else if (typeStr == "kinematic") type = BodyType::Kinematic;

        rigidbody = std::make_shared<Rigidbody>(type, collider, mass, friction);
    }

    if (rigidbody)
    {
        SetRigidbody(rigidbody);
    }
}
```

现在，`PhysicsComponent`可以从JSON加载碰撞体和刚体。

---

## 13）用坠落物体进行测试

让我们添加另一个对象来测试碰撞 —— 一个落在地上的简单立方体。

```json
{
  "name": "ObjectCollide",
  "position": { "x": 0, "y": 7, "z": 0 },
  "rotation": { "x": 1, "y": 2, "z": 0, "w": 1 },
  "components": [
    {
      "type": "MeshComponent",
      "mesh": {
        "type": "box",
        "x": 1,
        "y": 1,
        "z": 1
      },
      "material": "materials/brick.mat"
    },
    {
      "type": "PhysicsComponent",
      "collider": {
        "type": "box",
        "x": 1,
        "y": 1,
        "z": 1
      },
      "body": {
        "mass": 5,
        "friction": 0.5,
        "type": "dynamic"
      }
    }
  ]
}
```

* 网眼：1×1×1盒。
* 物理学：相同尺寸的箱式碰撞体，动态刚体（质量=5）。  
    这个立方体将坠落并与地面平台碰撞。

---

## 14）添加Light对象

让我们在场景中添加一个光源。

在`scene.json `中：

```json
{
  "name": "Light",
  "position": { "x": 0, "y": 5, "z": 0 },
  "components": [
    {
      "type": "LightComponent",
      "color": { "r": 1, "g": 1, "b": 1 }
    }
  ]
}
```

在“LightComponent”中添加：

```cpp
void LoadProperties(const nlohmann::json& j) override 
{
    if (j.contains("color")) 
    {
        auto c = j["color"];
        glm::vec3 color(
            c.value("r", 1.0f),
            c.value("g", 1.0f),
            c.value("b", 1.0f)
        );
        SetColor(color);
    }
}
```

---

## 15）测试场景

现在让我们来测试一下。

在`Game:：Init（）`中，注释掉我们之前编写的所有手动对象创建代码，并将其替换为：

```cpp
auto scene = Scene::Load("scenes/scene.sc");
m_scene = scene;
```

同时将`Game.h`中的`Scene*`替换为`std::shared_ptr<Scene>`

还有一步。打开`scene.sc`，在`children `块后添加`"camera": "MainPlayer"`。然后返回到`Scene:：Load（）`  
在返回结果之前，写下：

```cpp
if (json.contains("camera"))
{
    std::string cameraObjName = json.value("camera", "");
    for (const auto& child : result->m_objects)
    {
        if (auto object = child->FindChildByName(cameraObjName))
        {
            result->SetMainCamera(object);
            break;
        }
    }
}
```

这将查找并设置场景的主摄像头

在`Player:：Init（）`中，清理旧的初始化代码，只留下我们真正需要的东西 —— 隐藏节点，抓取`AnimationComponent`。

现在运行游戏：

* 场景从JSON加载。
* 对象显现。
* 动画触发。
* 特征控制器工作。

祝贺! 这是向前迈出的一大步。

现在，您可以用JSON手工设计不同的场景，使用不同的组件和对象层次结构。这是强大而令人兴奋的！