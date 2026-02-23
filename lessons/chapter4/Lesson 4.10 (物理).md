# 将子弹物理集成到我们的发动机中

是时候召回艾萨克·牛顿爵士，将物理学带入我们的引擎了。  
最实用的方法是整合现有的成熟 **物理引擎**，而不是重新发明它。物理模拟在数学上很重，行业标准实践（在游戏、电影、模拟中）是使用 **第三方物理库**。

我们将整合[**子弹物理**](https://github.com/bulletphysics/bullet3).它被广泛使用，开源，可靠。

---

## 1）将Bullet添加到我们的项目中

我已经下载了Bullet并将其放入我们的`thirdparty/`文件夹中。  
在`CMakeLists.txt`中，我们调整：

* 禁用不必要的部件：

    * `BUILD_UNIT_TESTS = OFF`
    * `BUILD_BULLET2_DEMOS = OFF`
    * `BUILD_EXTRAS = OFF`
    * `BUILD_SHARED_LIBS = OFF`
* 仅链接我们需要的内容（静态库）：

    * `BulletCollision`
    * `BulletDynamics`
    * `LinearMath`

这使构建保持精简和专注。

---

## 2）物理管理类

我们将创建一个**PhysicsManager**来拥有和控制子弹世界。

### 2.1 文件设置

创建文件夹`source/physical/`添加：

* `PhysicsManager.h`
* `PhysicsManager.cpp`

### 2.2 类声明

转发声明我们需要的子弹类型：

```cpp
class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
```

然后定义：

```cpp
class PhysicsManager 
{
public:
    PhysicsManager();
    ~PhysicsManager();

    void Init();
    void Update(float deltaTime);

    btDiscreteDynamicsWorld* GetWorld() { return m_world.get(); }

private:
    std::unique_ptr<btBroadphaseInterface> m_broadphase;
    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfig;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_world;
};
```

### 2.3 实施

在“PhysicsManager.cpp”中：

```cpp
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
```

* `Init`：

    * 按顺序实例化所有Bullet子系统。
    * 创建 `btDiscreteDynamicsWorld`。
    * 设置重力：

        ```cpp
        void PhysicsManager::Init()
        {
            // --- World setup ---
            m_broadphase = std::make_unique<btDbvtBroadphase>();
            m_collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
            m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collisionConfig.get());
            m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();
            m_world = std::make_unique<btDiscreteDynamicsWorld>(
                m_dispatcher.get(), m_broadphase.get(), m_solver.get(), m_collisionConfig.get());

            m_world->setGravity(btVector3(0, -9.81f, 0));
        }
        ```

    * `Update`：

        ```cpp
        const btScalar fixedTimeStep = 1.0f / 60.0f;
        const int maxSubsteps = 4;
        m_world->stepSimulation(deltaTime, maxSubsteps, fixedTimeStep);
        ```

---

## 3）引擎集成

通过`Engine`显示`PhysicsManager`：

* 添加`PhysicsManager m_physicsManager;`
* 添加`PhysicsManager& GetPhysicsManager();`
* 在`Engine::Init()`中，初始化画面后，调用：

    ```cpp
    m_physicsManager.Init();
    ```
* 在`Engine::Run()`中，就在`m_application->Update()`之前有deltaTime之后，  
调用`m_physicsManager.Update(deltaTime)`

现在，物理学作为引擎更新循环的一部分运行。

---

## 4）碰撞器

**碰撞器**是物理引擎用于计算碰撞的简化几何形状。  
常见的碰撞器类型包括：

* **盒子碰撞器**（`btBoxShape`）

    * 由宽度、高度和长度定义。
    * 非常适合板条箱、墙壁、门或任何块状物体。

* **球形碰撞器**（`btSphereShape`）

    * 由单个半径定义。
    * 非常适合球、投射物或任何圆形物体。

* **胶囊碰撞器**（`btCapsuleShape`）

    * 由两个半球覆盖的圆柱体。
    * 通常用于角色，因为它在不平坦的表面上平滑移动，不会卡在边缘上。

为什么是这些形状？  
因为它们是：

* **数学上很简单** → 快速碰撞检测。
* **稳定** → 小故障的可能性更小。
* **足够好** → 它们在不浪费资源的情况下接近真实的游戏行为。

在专业游戏中，即使是非常详细的角色和物体也会由几个碰撞器来代表。

我们创建了一个统一的界面。

### 4.1 文件

`source/physics/Collider.h`和`Collider.cpp`。

### 4.2 基本类

```cpp
class btCollisionShape;

class Collider 
{
public:
    virtual ~Collider() { if (m_shape) delete m_shape; }
    btCollisionShape* GetShape() const { return m_shape; }

protected:
    btCollisionShape* m_shape = nullptr;
};
```

### 4.3 衍生形状

* **BoxCollider**:

  ```cpp
  class BoxCollider : public Collider 
  {
  public:
      BoxCollider(const glm::vec3& extents) 
      {
          glm::vec3 halfExtents = extents * 0.5f;
          m_shape = new btBoxShape(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
      }
  };
  ```

* **SphereCollider**:

  ```cpp
  class SphereCollider : public Collider 
  {
  public:
      SphereCollider(float radius) 
      {
          m_shape = new btSphereShape(btScalar(radius));
      }
  };
  ```

* **CapsuleCollider**:

  ```cpp
  class CapsuleCollider : public Collider 
  {
  public:
      CapsuleCollider(float radius, float height) 
      {
          m_shape = new btCapsuleShape(btScalar(radius), btScalar(height));
      }
  };
  ```

---

## 5）刚体

虽然碰撞器定义了 **形状**，但它们不会自行移动。  
为了真正模拟物理，我们需要一个 **刚性体**。

**刚体** 表示物理世界中的对象。它结合了：

* **碰撞器（形状）** → 物体如何碰撞。
* **质量** → 它有多重。
* **摩擦** → 它如何抵抗滑动。
* **运动状态** → 它在哪里以及如何移动。

有三种主要的**刚性体类型**：

1. **静态**

    * 不动。
    * 例如：地面、墙壁、大型不可移动的障碍物。
    * 其他物体与之相撞。

2. **动态**

    * 完全由物理模拟。
    * 例如：一个落下的盒子，一个滚动的球，一把你可以推的椅子。
    * 受重力和碰撞等力的影响。

3. **运动学**

    * 手动控制（通过代码），而不是物理控制。
    * 仍然与动态对象碰撞。
    * 示例：移动平台、电梯、玩家角色舱。

这样想：

* **静态** = 环境。
* **动态** = 自由移动的物体。
* **运动学** = 脚本或受控运动。

### 5.1 声明

将`RigidBody.h`和`RigidBody.cpp`添加到`source/physics`中  
在`RigidBody.h`中：

```cpp
enum class BodyType 
{ 
    Static, 
    Dynamic, 
    Kinematic 
};

class RigidBody 
{
public:
    RigidBody(BodyType type,
              const std::shared_ptr<Collider>& collider,
              float mass,
              float friction);
    ~RigidBody();

    btRigidBody* GetBody() { return m_body.get(); }

    BodyType GetType() const { return m_type; }

    void SetAddedToWorld(bool added);
    bool IsAddedToWorld() const;

    void SetPosition(const glm::vec3& pos);
    void SetRotation(const glm::quat& rot);
    glm::vec3 GetPosition() const;
    glm::quat GetRotation() const;

private:
    BodyType m_type = BodyType::Static;
    std::shared_ptr<Collider> m_collider;
    float m_mass = 0.0f;
    float m_friction = 0.5f;
    bool m_addedToWorld = false;

    std::unique_ptr<btRigidBody> m_body;
};
```

### 5.2 行为

* 动态车身 → 质量>0，计算惯性。

* 静态/动态 → 质量=0。

```cpp
RigidBody::RigidBody(BodyType type, const std::shared_ptr<Collider>& collider, float mass, float friction)
    : m_type(type), m_collider(collider), m_mass(mass), m_friction(friction)
{
    btVector3 inertia(0, 0, 0);
    if (m_type == BodyType::Dynamic && mass > 0.0f && m_collider->GetShape())
    {
        m_collider->GetShape()->calculateLocalInertia(btScalar(mass), inertia);
    }

    btTransform transform;
    transform.setIdentity();
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);

    btRigidBody::btRigidBodyConstructionInfo info(
        (m_type == BodyType::Dynamic) ? btScalar(mass) : btScalar(0.0),
        motionState, m_collider->GetShape(), inertia);

    m_body = std::make_unique<btRigidBody>(info);
    m_body->setFriction(btScalar(friction));

    // Kinematic:
    if (m_type == BodyType::Kinematic)
    {
        m_body->setCollisionFlags(m_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        m_body->setActivationState(DISABLE_DEACTIVATION);
    }
}

void RigidBody::SetAddedToWorld(bool added)
{
    m_isAddedToWorld = added;
}

bool RigidBody::IsAddedToWorld() const
{
    return m_isAddedToWorld;
}

```

* **转换设置器**仅在添加到世界**之前有效**，因为一旦模拟，Bullet会在每一步都覆盖它们。

```cpp
void RigidBody::SetPosition(const glm::vec3& pos)
{
    if (!m_body)
    {
        return;
    }

    auto& tr = m_body->getWorldTransform();
    tr.setOrigin(btVector3(btScalar(pos.x), btScalar(pos.y), btScalar(pos.z)));
    if (m_body->getMotionState()) 
    {
        m_body->getMotionState()->setWorldTransform(tr);
    }
    m_body->setWorldTransform(tr);
}

glm::vec3 RigidBody::GetPosition() const
{
    const auto& pos = m_body->getWorldTransform().getOrigin();
    return glm::vec3(pos.x(), pos.y(), pos.z());
}

void RigidBody::SetRotation(const glm::quat& rot)
{
    if (!m_body)
    {
        return;
    }

    auto& tr = m_body->getWorldTransform();
    tr.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
    if (m_body->getMotionState())
    {
        m_body->getMotionState()->setWorldTransform(tr);
    }
    m_body->setWorldTransform(tr);
}

glm::quat RigidBody::GetRotation() const
{
    const auto& rot = m_body->getWorldTransform().getRotation();
    return glm::quat(rot.w(), rot.x(), rot.y(), rot.z());
}
```

---

### 5.3 让PhysicsManager添加/删除实体

在“PhysicsManager.h”中：

```cpp
class RigidBody;

public:
    void AddRigidBody(RigidBody* body);
    void RemoveRigidBody(RigidBody* body);
```

在`PhysicsManager.cpp`中：

```cpp
#include "RigidBody.h"
#include <btBulletDynamicsCommon.h>

void PhysicsManager::AddRigidBody(RigidBody* body) 
{
    if (!body || !m_world)
    {
        return;
    }

    if (auto* rb = body->GetBody()) 
    {
        m_world->addRigidBody(rb,
            btBroadphaseProxy::StaticFilter,
            btBroadphaseProxy::AllFilter);
        body->SetAddedToWorld(true);
    }
}

void PhysicsManager::RemoveRigidBody(RigidBody* body) 
{
    if (!body || !m_world) 
    {
        return;
    }

    if (auto* rb = body->GetBody()) 
    {
        m_world->removeRigidBody(rb);
        body->SetAddedToWorld(false);
    }
}
```

最后是`RigidBody`的析构函数:

```cpp
RigidBody::~RigidBody()
{
    if (m_isAddedToWorld)
    {
        Engine::GetInstance().GetPhysicsManager().RemoveRigidBody(this);
    }
}
```

--

## 6）物理组件
要将刚体绑定到“游戏对象”，请添加`PhysicsComponent`。

### 6.1 文件
创建`scene/components/PhysicsComponent.h/.cpp`。它在初始化时同步变换，对于动态体，每帧都会拉回变换。

### 6.2 类

**PhysicsComponent.h**

```cpp
class PhysicsComponent : public Component 
{
public:
    PhysicsComponent(std::shared_ptr<RigidBody> body);

    void Init() override;
    void Update(float dt) override;

private:
    std::shared_ptr<RigidBody> m_rigidBody;
};
```

### 6.3实施

* `Init`：同步从`GameObject`到`RigidBody`的初始转换，然后将本体添加到物理世界：
```cpp
void PhysicsComponent::Init() 
{
    if (!m_rigidBody) 
    {
        return;
    }

    const auto pos = m_owner->GetWorldPosition();
    const auto rot = m_owner->GetRotation();

    m_rigidBody->SetPosition(pos);
    m_rigidBody->SetRotation(rot);

    Engine::GetInstance().GetPhysicsManager().AddRigidBody(m_rigidBody.get());
}
```
* `Update`：对于动态物体，从物理同步转换回`GameObject`：
```cpp
void PhysicsComponent::Update(float) 
{
    if (!m_rigidBody) 
    {
        return;
    }

    if (m_rigidBody->GetType() == BodyType::Dynamic) 
    {
        m_owner->SetPosition(m_rigidBody->GetPosition());
        m_owner->SetRotation(m_rigidBody->GetRotation());
    }
}
```

---

## 7）更新组件类

转到`Component`类并添加空的`virtual void Init() {}`函数，然后转到`GameObject::AddComponent()`函数，在所有设置完成后调用  `component->Init()`。这确保了每个组件在添加后都可以初始化  到对象。

---

## 7）示例：地平面+掉落的箱子

让我们通过创建一个简单的场景来测试物理：一个静态地面和一个落在上面的动态盒子。

### 7.1 扩展网格

重命名`Mesh::CreateCube` → `Mesh::CreateBox(const glm::vec3& extents = glm::vec3(1))`  
生成顶点时，使用 **半范围** 来定位角：

```cpp
const glm::vec3 half = extents * 0.5f;
// use half.x/half.y/half.z for the 8 corners
```

### 7.2 在`Game.cpp`中

**地面**：

```cpp
auto ground = m_scene->CreateObject("ground");
ground->SetPosition(glm::vec3(0, -5, 0));

glm::vec3 groundExtents(20, 2, 20);
auto groundMesh = eng::Mesh::CreateBox(groundExtents);
ground->AddComponent(new MeshComponent(material, groundMesh));

auto groundCollider = std::make_shared<eng::BoxCollider>(groundExtents);
auto groundBody = std::make_shared<eng::RigidBody>(
    eng::BodyType::Static, groundCollider, 0.0f, 0.5f);

ground->AddComponent(new PhysicsComponent(groundBody));
```

**掉落的箱子**：

```cpp
auto boxObj = m_scene->CreateObject("fallingBox");
boxObj->AddComponent(new MeshComponent(material, mesh));

boxObj->SetPosition(glm::vec3(0, 2, 2));
boxObj->SetRotation(glm::quat(glm::vec3(1, 2, 0))); // rotated

auto boxCollider = std::make_shared<eng::BoxCollider>(glm::vec3(1));
auto boxBody = std::make_shared<eng::RigidBody>(
    eng::BodyType::Dynamic, boxCollider, 5.0f, 0.5f);

boxObj->AddComponent(new PhysicsComponent(boxBody));
```

**玩家开始位置**：

```cpp
player->SetPosition(glm::vec3(0, 1, 7));
```

---

## 8）结果

* 地面是静止的，不会移动。
* 一个盒子在其上方产生，并在重力作用下落下（Y轴为`-9.81`）。
* 在接触时，子弹会解决碰撞，箱子会放在地上。
* 变换同步确保游戏对象在视觉上更新以匹配物理模拟。

---

祝贺！  
您已成功将 **Bullet Physics** 集成到我们的引擎中，构建了一个干净的抽象（`Collider`, `RigidBody`, `PhysicsComponent`），并创建了您的第一个物理模拟场景。从这里，我们可以添加更多的形状、碰撞响应、角色控制器和交互式游戏。