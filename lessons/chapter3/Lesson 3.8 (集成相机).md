1. 如何使用`GetComponent<T>（）`检索特定类型的组件
2. 如何实现轻量级组件类型识别系统（无RTTI）
3. 3D相机的工作原理：`FOV`、`nearPlane`、`farPlane`和`纵横比`
4. 如何将相机投影和视图矩阵传递给着色器
5. 如何使用合适的相机和视角渲染场景

---

## Part 1: 按类型访问组件

### 我们为什么需要这个？

我们希望能够做到这一点：

```cpp
auto* camera = gameObject->GetComponent<CameraComponent>();
```

为了实现这一目标，我们需要：

* 一种类型安全的检索组件的方法
* 一个在运行时识别组件类型的快速系统（不使用C++的内置RTTI）

---

### Step 1: 将`GetComponent<T>（）`添加到`GameObject`

```cpp
template <typename T>
T* GetComponent() 
{
    size_t typeId = Component::StaticTypeId<T>();

    for (const auto& component : m_components) 
    {
        if (component->GetTypeId() == typeId) 
        {
            return static_cast<T*>(component.get());
        }
    }

    return nullptr;
}
```

---

### Step 2: 自定义类型识别系统

我们将组件存储为`std:：unique_ptr<Component>`。因此，我们需要一种方法来确定每个组件的实际类型，而不依赖于RTTI。

在`组件`中：

```cpp
class Component 
{
protected:
    GameObject* m_owner = nullptr;

private:
    static size_t nextId;

public:
    virtual ~Component() = default;
    virtual void Update(float deltaTime) = 0;
    virtual size_t GetTypeId() const = 0;

    template<typename T>
    static size_t StaticTypeId() 
    {
        static size_t typeId = nextId++;
        return typeId;
    }

    GameObject* GetOwner() const { return m_owner; }
};
```

在 `Component.cpp`中:

```cpp
size_t Component::nextId = 1;
```

---

### 🔁 Step 3: 使用宏来避免重复代码

为了避免在每个组件类中编写相同的样板：

```cpp
#define COMPONENT(ComponentClass) \
public: \
    static size_t TypeId() { return Component::StaticTypeId<ComponentClass>(); } \
    size_t GetTypeId() const override { return TypeId(); }
```

现在，在您的组件类中：

```cpp
class CameraComponent : public Component 
{
    COMPONENT(CameraComponent)
    // ...
};
```

以及:

```cpp
class MeshComponent : public Component 
{
    COMPONENT(MeshComponent)
    // ...
};
```

---

## Part 2: 3D相机的工作原理

游戏引擎中的 **摄像头** 是一个虚拟眼睛。它决定了场景的哪个部分是可见的，以及如何投影到屏幕上。

涉及两个基本矩阵：

| Matrix     | Role                                       |
| ---------- | ------------------------------------------ |
| View       | 相对于相机定位世界                          |
| Projection | 将3D世界投影到2D屏幕空间                    |

它们与 **模型矩阵** 一起构成了MVP链：

```cpp
gl_Position = uProjection * uView * uModel * vec4(position, 1.0);
```

---

## 相机投影概念

### 1.**视场（FOV）**

* 描述摄影机视锥的垂直角度。
* 以度为单位。
* 宽视场：更可见，更失真
* 窄视场：放大感

| FOV (degrees) | Effect                |
| ------------- | --------------------- |
| 30–45         | 变焦/远摄镜头          |
| 60 (default)  | 平衡视角              |
| 90–120        | 广角/鱼眼             |

---

### 2. **Near and Far Planes**

* 相机只渲染这两个距离 **之间** 的距离。
* 它们之外的一切都被 **剪** 掉了。
* 这定义了 **视锥** —— 3D空间中的截棱锥。

| Plane       | Description                                 |
| ----------- | ------------------------------------------- |
| `nearPlane` | 物体必须有多近才能被看到                     |
| `farPlane`  | 对象还可以渲染多远                           |

---

### 3. **宽高比**

* 屏幕的宽度/高度
* 影响FOV的水平应用方式

如果错误，场景将显得被压扁或拉伸。

---

## Part 3: 实现投影矩阵

### 在 `CameraComponent`:

#### 字段

```cpp
float m_fov = 60.0f;
float m_nearPlane = 0.1f;
float m_farPlane = 1000.0f;
```

#### 方法

```cpp
glm::mat4 GetViewMatrix() const 
{
    return glm::inverse(m_owner->GetWorldTransform());
}

glm::mat4 GetProjectionMatrix(float aspectRatio) const 
{
    return glm::perspective(glm::radians(m_fov), aspectRatio, m_nearPlane, m_farPlane);
}
```

---

## Part 4: 将其连接到发动机中

### 在 `Engine::Run()`:

```cpp
CameraData cameraData;

int width = 0, height = 0;
glfwGetWindowSize(mWindow, &width, &height);
float aspect = static_cast<float>(width) / static_cast<float>(height);

if (m_currentScene) 
{
    if (auto* cameraObj = m_currentScene->GetMainCamera()) 
    {
        if (auto* camera = cameraObj->GetComponent<CameraComponent>()) 
        {
            cameraData.viewMatrix = camera->GetViewMatrix();
            cameraData.projectionMatrix = camera->GetProjectionMatrix(aspect);
        }
    }
}

m_renderQueue.Draw(m_graphicsAPI, cameraData);
```

---

### 在 `Game::Init()`

```cpp
auto camera = m_scene->CreateObject("Camera");
camera->AddComponent<CameraComponent>();
camera->SetPosition(glm::vec3(0.0f, 0.0f, 2.0f)); // Move the camera back a bit

m_scene->SetMainCamera(camera);
eng::Engine::GetInstance().SetScene(m_scene);
```

---

## Part 5: 更新着色器

### 顶点着色器 (GLSL)

```glsl
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() 
{
    gl_Position = uProjection * uView * uModel * vec4(position, 1.0);
}
```

现在着色器按顺序应用所有三个变换：模型→视图→投影 model → view → projection.

---

## 最终结果

现在，您已经添加了一个真实且可用的 **摄像系统** ，包括：

* 视野（FOV） 
* 近剪裁平面和远剪裁平面 
* 纵横比计算 
* 与渲染流程的集成 
* 用于未来扩展的组件类型检索系统

而且你做到了这一点，并没有依赖RTTI（运行时类型识别）！

---

## 摘要

| Feature                  | Description                                            |
| ------------------------ | ------------------------------------------------------ |
| `GetComponent<T>()`      | 以类型安全、快速的方式检索组件           |
| `StaticTypeId()`         | 组件类型的唯一ID系统                   |
| `FOV`                    | 控制相机视野的宽度                      |
| `nearPlane` / `farPlane` | 限制相机的可见范围                     |
| `aspectRatio`            | 在不同分辨率下保持正确的比例 |
| `View Matrix`            | 相对于相机变换世界                    |
| `Projection Matrix`      | 将3D世界变换到2D空间                      |

---