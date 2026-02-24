# 跳台和碰撞事件

既然我们已经有了**物理学**，让我们添加一些更有趣的东西。  
我们将创建一个**跳跃平台**：

* 它将被涂成**红色**，
* 当玩家踩到它时，它们会被向上弹射（就像许多射击游戏一样）。

为此，我们需要在物理引擎中检测**接触（碰撞）**。

---

## 1. 在PhysicsManager中检测碰撞

打开`PhysicsManager:：Update（）`。  
调用`stepSimulation`后，我们可以查询此帧中发生的所有碰撞。

Bullet Physics允许我们将 **用户指针** 附加到刚体或控制器上。  
这些指针可以是我们想要的任何东西（我们将使用它们来指向我们的 **碰撞对象**）。

计划如下：

1. 取回碰撞流形，
2. 取出两个物体，
3. 将它们的**用户指针**转换为`CollisionObject*`，
4. 提取接触点和法线，
5. 通知两个物体发生了碰撞。

---

## 2. 基类：碰撞对象(CollisionObject)

让我们为所有可碰撞对象创建一个新的基类。

文件夹：

* `CollisionObject.h`
* `CollisionObject.cpp`

**CollisionObject.h**

```cpp
class IContactListener;

enum class CollisionObjectType 
{
    RigidBody,
    KinematicCharacterController
};

class CollisionObject 
{
public:
    CollisionObjectType GetCollisionObjectType() const { return m_type; }

    void AddContactListener(IContactListener* listener);
    void RemoveContactListener(IContactListener* listener);

protected:
    void DispatchContactEvent(CollisionObject* obj,
                              const glm::vec3& position,
                              const glm::vec3& normal);

    CollisionObjectType m_type;
    std::vector<IContactListener*> m_contactListeners;

    friend class PhysicsManager;
};

class IContactListener
{
public:
    virtual void OnContact(CollisionObject* obj, const glm::vec3& position, const glm::vec3& normal) = 0;
};
```

**CollisionObject.cpp**

```cpp
void CollisionObject::AddContactListener(IContactListener* listener) 
{
    m_contactListeners.push_back(listener);
}

void CollisionObject::RemoveContactListener(IContactListener* listener) 
{
    auto it = std::find(m_contactListeners.begin(), m_contactListeners.end(), listener);
    if (it != m_contactListeners.end())
    {
        m_contactListeners.erase(it);
    }
}

void CollisionObject::DispatchContactEvent(CollisionObject* obj,
                                           const glm::vec3& pos,
                                           const glm::vec3& normal) 
{
    for (auto listener : m_contactListeners) 
    {
        if (listener) 
        {
            listener->OnContact(obj, pos, normal);
        }
    }
}
```

---

## 3. PhysicsManager更新

在“PhysicsManager：：Update”中，遍历所有流形：

```cpp
auto dispatcher = m_world->getDispatcher();
const auto numManifolds = dispatcher->getNumManifolds();
for (int i = 0; i < numManifolds; i++) 
{
    btPersistentManifold* manifold = dispatcher->getManifoldByIndexInternal(i);

    auto bodyA = reinterpret_cast<CollisionObject*>(manifold->getBody0()->getUserPointer());
    auto bodyB = reinterpret_cast<CollisionObject*>(manifold->getBody1()->getUserPointer());

    if (!bodyA || !bodyB) continue;

    for (int j = 0; j < manifold->getNumContacts(); j++) 
    {
        const btManifoldPoint& pt = manifold->getContactPoint(j);

        glm::vec3 pos(pt.getPositionWorldOnB().x(), pt.getPositionWorldOnB().y(), pt.getPositionWorldOnB().z());
        glm::vec3 normal(pt.m_normalWorldOnB.x(), pt.m_normalWorldOnB.y(), pt.m_normalWorldOnB.z());

        bodyA->DispatchContactEvent(bodyB, pos, normal);
        bodyB->DispatchContactEvent(bodyA, pos, normal);
    }
}
```

现在，每次碰撞都会通知两个对象。

---

## 4. 扩展刚体和特征控制器

`RigidBody`和`KinematicCharacterController`都应从`CollisionObject`继承。

* 在`RigidBody`构造函数中：

    ```cpp
    m_type = CollisionObjectType::RigidBody;
    m_body->setUserPointer(this);
    ```

* 在`KinematicCharacterController`构造函数中：

    ```cpp
    m_type = CollisionObjectType::KinematicCharacterController;
    m_ghost->setUserPointer(this);
    ```

现在两者都是有效的“碰撞对象”。

---

## 5. 创建跳跃平台

让我们实现我们的特殊**JumpPlatform**。

文件夹：

* `JumpPlatform.h`
* `JumpPlatform.cpp`

**JumpPlatform.h**

```cpp
class JumpPlatform : public eng::GameObject,
                     public eng::IContactListener 
{
    GAMEOBJECT(JumpPlatform)
public:
    void Init() override;
    void OnContact(emg::CollisionObject* obj,
                   const glm::vec3& position,
                   const glm::vec3& normal) override;
};
```

**JumpPlatform.cpp**

```cpp
void JumpPlatform::Init() 
{
    auto physics = GetComponent<PhysicsComponent>();
    if (physics) 
    {
        auto rigidBody = physics->GetRigidBody();
        if (rigidBody) 
        {
            rigidBody->AddContactListener(this);
        }
    }
}

void JumpPlatform::OnContact(CollisionObject* obj,
                             const glm::vec3& position,
                             const glm::vec3& normal) 
{
    if (obj->GetCollisionObjectType() == CollisionObjectType::KinematicCharacterController) 
    {
        auto controller = static_cast<KinematicCharacterController*>(obj);
        if (controller) 
        {
            controller->Jump(glm::vec3(0, 20, 0));
        }
    }
}
```

---

## 6. 注册对象

在“Game.cpp”中：

```cpp
JumpPlatform::Register();
```

在`scene.sc`中，定义对象：

```json
{
  "name": "JumpPlatform",
  "type": "JumpPlatform",
  "position": { "x": -7, "y": 1.75, "z": 1 },
  "components": [
    {
      "type": "MeshComponent",
      "mesh": { "type": "box", "x": 2, "y": 0.2, "z": 2 },
      "material": {
        "path": "materials/checker.mat",
        "params": { "float3": [ { "name": "color", "value0": 1, "value1": 0, "value2": 0 } ] }
      }
    },
    {
      "type": "PhysicsComponent",
      "collider": { "type": "box", "x": 2, "y": 0.2, "z": 2 },
      "body": { "mass": 0, "friction": 0.5, "type": "static" }
    }
  ]
}
```

这使得一个静态的红色平台起到了跳台的作用。

---

## 7. 添加球体

为了增加多样性，我还放置了一些**彩色球体**。

在`MeshComponent:：LoadProperties `中，扩展网格类型加载器：

```cpp
if (type == "box") 
{
    // existing
} 
else if (type == "sphere") 
{
    float r = meshObj.value("r", 1.0f);
    auto mesh = Mesh::CreateSphere(r, 16, 16);
    SetMesh(mesh);
}
```

对`PhysicsComponent`执行相同的操作：  
在盒子和胶囊都可用`"sphere"`碰撞体。

现在，您可以生成彩色球体，这些球体可以滚动并对子弹做出反应。

---

## 结果

* 我们现在有一个**跳跃平台**（红色框），可以向上弹跳玩家，
* 使用 **listeners** 检测和分发碰撞，
* `RigidBody`和`CharacterController`都集成到这个系统中，
* 我们增加了**球面网格**对碰撞体的支持，
* 场景包含方框、球体和跳跃板 —— 所有这些都是交互式的。

当你运行游戏时：

* 踏上平台，玩家会被弹射出，
* 射击球体和盒子会使它们产生反应，
* 这个场景感觉更生动、更有活力。

惊人的进步！现在，您可以使用交互式物理驱动元素扩展您的游戏世界。