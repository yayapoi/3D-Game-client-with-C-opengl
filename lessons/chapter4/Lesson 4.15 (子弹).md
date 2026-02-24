# 改善场景：添加子弹和更好的场景管理

现在，让我们让我们的场景更具可玩性，并改进我们已经拥有的一些系统。

我们将从 **渲染** 开始，然后转到 **场景管理** ，最后添加 **可以射击的子弹**。

---

## 1. 渲染改进

让我们进入`Mesh `类并添加一个新函数：

```cpp
void Mesh::Unbind() 
{
    glBindVertexArray(0);
}
```

我们为什么需要这个？  
渲染完网格后，我们不需要再保持其活动状态。从逻辑的角度来看，解除绑定更为正确。

接下来，进入`GraphicsAPI`并添加一个助手：

```cpp
void GraphicsAPI::UnbindMesh(Mesh* mesh) 
{
    if (mesh) 
    {
        mesh->Unbind();
    }
}
```

最后，打开`RenderQueue`，找到`DrawAll`函数。调用后：

```cpp
m_graphicsAPI.DrawMesh(command.mesh);
```

我们应该立即解除绑定：

```cpp
m_graphicsAPI.UnbindMesh(command.mesh);
```

太棒了。渲染现在更干净了。

---

## 2. 场景改进

您还记得使用**MarkForDestroy**删除对象。让我们让这更安全。

1. 在“Scene”类中添加：

```cpp
bool m_isUpdating = false;
```

2. `Scene::Update`中：

    * 开始时，设置`m_isUpdating = true;`.
    * 最后，设置`m_isUpdating = false;`.

3. 在运行主更新之前，删除所有死对象：

```cpp
m_objects.erase(
    std::remove_if(m_objects.begin(), m_objects.end(),
        [](auto& obj) { return !obj->IsAlive(); }),
    m_objects.end()
);
```

现在，我们免受破坏后遗留的物体的影响。

---

### 在更新过程中处理新对象

如果我们在**更新场景**的同时创建新对象呢？  
为此，让我们添加一个临时容器：

```cpp
std::vector<std::pair<GameObject*, GameObject*>> m_objectsToAdd;
```

这将存储**候选对象**（新对象+其父对象）。  

在每个`CreateObject`函数中，不要直接调用`SetParent`，而是这样做：

```cpp
if (m_isUpdating) 
{
    m_objectsToAdd.push_back({ obj, parent });
} 
else 
{
    SetParent(obj, parent);
}
```

回到`Scene::Update`中，在删除死对象后，我们处理新对象：

```cpp
for (auto& obj : m_objectsToAdd) 
{
    SetParent(obj.first, obj.second);
}
m_objectsToAdd.clear();
```

太好了！现在，我们可以在场景更新过程中安全地添加对象。

---

3. 添加子弹

让我们让我们的玩家真正**发射子弹**。

### 第一步：暴露刚体

在“PhysicsComponent”中添加：

```cpp
const std::shared_ptr<RigidBody>& GetRigidBody() const { return m_rigidBody; }
```

---

### 步骤2：创建子弹类

在游戏代码中，添加：

```cpp
class Bullet : public eng::GameObject 
{
    GAMEOBJECT(Bullet) // our macro
public:
    void Update(float dt) override 
    {
        eng::GameObject::Update(dt);
        m_lifetime -= dt;
        if (m_lifetime <= 0.0f)
        {
            MarkForDestroy();
        }
    }
private:
    float m_lifetime = 2.0f; // bullet lives for 2 seconds
};
```

因此，子弹会在2秒后自动消失。

---

### 第三步：玩家射击代码

在`Player::Update`中，我们已经播放了**射击动画**和**射击声音**，添加：

```cpp
auto bullet = m_scene->CreateObject<Bullet>("Bullet");
```

接下来，给子弹一个**材质**：

```cpp
auto material = eng::Material::Load("materials/suzanne.mat");
```

还有一个**网格**。对于子弹，让我们使用一个**球体**。  
因此，在“Mesh”中添加：

```cpp
static std::shared_ptr<Mesh> CreateSphere(float radius, int sectors, int stacks);
```

此函数为球体生成顶点、索引、法线和UV，并返回网格。

然后在`Player`中：

```cpp
auto mesh = eng::Mesh::CreateSphere(0.2f, 32, 32);
bullet->AddComponent(new eng::MeshComponent(material, mesh));
```

---

### 第四步：生成位置

我们需要子弹从枪口处射出。  
我手动选择了一个点 —— 只需重复一遍：

```cpp
auto pos = FindChildByName("BOOM_35")->GetWorldPosition();
bullet->SetPosition(pos + m_rotation * glm::vec3(-0.2f, 0.2f, -1.75f));
```

---

### 第五步：子弹物理学

添加碰撞体和刚体：

```cpp
auto collider = std::make_shared<eng::SphereCollider>(0.2f);
auto rigidbody = std::make_shared<eng::RigidBody>(
    eng::BodyType::Dynamic, collider, 10.0f, 0.1f
);

bullet->AddComponent(new eng::PhysicsComponent(rigidbody));
```

---

### 第六步：让它飞起来

在`RigidBody`中添加：

```cpp
void ApplyImpulse(const glm::vec3& impulse) 
{
    m_body->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
}
```

现在在`Player::Update`中：

```cpp
glm::vec3 front = m_rotation * glm::vec3(0, 0, -1);
rigidbody->ApplyImpulse(front * 500.0f);
```

---

## 结果

现在，当你射击时：

* 一个球体（子弹）在武器的枪口处产生，
* 它有一个**网格**、一个**物理体**和一个**碰撞体**，
* 它使用**推力**向前飞行，
* 2秒后，它消失了。

祝贺!  
现在，你有了一个可以使用物理子弹的射击系统。