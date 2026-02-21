太棒了 现在，让我们将核心渲染对象封装到 **场景级对象** 中，并介绍 **场景** 及其 **游戏对象** 的概念。

---

### **场景与游戏对象概述**

如前一课所述，所有3D对象都是在 **场景** 中进行定位和渲染的。这一概念在游戏开发中得到了广泛应用。

**场景** 是特定游戏上下文中所有对象的容器。我们将以 **层次树** 结构来实现它。

---

#### 一座房子

想象一下，**房子** 是根对象。房子里有 **桌子** 和 **椅子** 这样的家具——它们是房子的子对象。

* 移动桌子不会影响房屋。
* 但是移动房子也会移动桌子和椅子，因为它们是房子的子对象。

这种层次结构使得可以对分组对象进行有组织的操作和变换。

---

### Step 1: 场景文件夹和文件

在引擎的`source`目录下创建一个名为`scene`的新文件夹。

在其中，创建以下文件：

* `GameObject.h`
* `GameObject.cpp`
* `Scene.h`
* `Scene.cpp`

更新`CMakeLists.txt`文件，并将头文件添加到`eng.h`中。

---

### Step 2: 实现`GameObject`类

场景的核心单元是`GameObject`。它应具备：

#### 字段:

```cpp
std::string m_name;
GameObject* m_parent = nullptr;
std::vector<std::unique_ptr<GameObject>> m_children;
bool m_isAlive = true;
```

将`Scene`类设置为`GameObject`的友元。

#### 方法:

```cpp
virtual ~GameObject() = default;
virtual void Update(float deltaTime);
const std::string& GetName() const;
void SetName(const std::string& name);
GameObject* GetParent();
bool IsAlive() const;
void MarkForDestroy();
```

将 **默认构造函数设为protected** 以防止直接实例化（强制通过工厂方法使用）。

---

### Step 3: 实现`GameObject`方法

在`GameObject.cpp`中，定义以下方法：

#### `Update()`:

```cpp
void GameObject::Update(float deltaTime) 
{
    for (auto it = m_children.begin(); it != m_children.end(); ) 
    {
        if ((*it)->IsAlive()) 
        {
            (*it)->Update(deltaTime);
            ++it;
        } 
        else 
        {
            it = m_children.erase(it);
        }
    }
}
```

其他简单方法:

```cpp
const std::string& GameObject::GetName() const 
{ 
    return m_name; 
}

void GameObject::SetName(const std::string& name) 
{ 
    m_name = name; 
}

GameObject* GameObject::GetParent() 
{ 
    return m_parent; 
}

bool GameObject::IsAlive() const 
{ 
    return m_isAlive; 
}

void GameObject::MarkForDestroy() 
{ 
    m_isAlive = false; 
}
```

---

### Step 4: 实现`Scene`类

在`Scene.h`中，定义`Scene`类：

#### 字段:

```cpp
std::vector<std::unique_ptr<GameObject>> m_objects;
```

#### 方法:

```cpp
void Update(float deltaTime);
void Clear();

GameObject* CreateObject(const std::string& name, GameObject* parent = nullptr);

template <typename T,
          typename = std::enable_if_t<std::is_base_of<GameObject, T>::value>::type>
T* CreateObject(const std::string& name, GameObject* parent = nullptr);
```

---

### Step 5: 实现场景方法

#### `Update()`:

```cpp
void Scene::Update(float deltaTime) 
{
    for (auto it = m_objects.begin(); it != m_objects.end(); ) 
    {
        if ((*it)->IsAlive()) 
        {
            (*it)->Update(deltaTime);
            ++it;
        } 
        else 
        {
            it = m_objects.erase(it);
        }
    }
}
```

#### `Clear()`:

```cpp
void Scene::Clear() 
{
    m_objects.clear();
}
```

#### `CreateObject()`:

```cpp
GameObject* Scene::CreateObject(const std::string& name, GameObject* parent) 
{
    auto obj = new GameObject();
    obj->SetName(name);
    SetParent(obj, parent);
    return obj;
}
```

#### 派生类型的模板版本:

```cpp
template <typename T,
          typename = std::enable_if_t<std::is_base_of<GameObject, T>::value>::type>
T* Scene::createObject(const std::string& name, GameObject* parent) 
{
    auto obj = new T();
    obj->SetName(name);
    SetParent(obj, parent);
    return obj;
}
```

---

### Next Step: 下一步：`SetParent()` 函数的实现

我之前已经提到过`SetParent()`方法——接下来我们将实现它。该方法将允许动态重新设置游戏对象的父对象，并且应该能够处理从旧父对象的子对象向量中移除对象并将其添加到新父对象的子对象向量中。