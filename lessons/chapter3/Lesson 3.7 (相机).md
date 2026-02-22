到目前为止，我们已经能够渲染场景中的单个对象。

但是，如果我们想渲染 **许多** 对象呢？如果这些物体分散在一个巨大的3D世界中，那么整个场景能不能都放在屏幕上呢？

这就是我们需要引入一个关键概念的地方：

### 相机.

---

### 为什么我们需要相机？

想想电影制作中的相机。  
一个大型电影场景可能有建筑物、道具、演员...但观众看到的内容取决于 **摄像头在哪里**、**它指向什么** 以及 **它的配置方式**。

同样的想法也适用于游戏开发。

* 我们有一个 **场景**，其中有许多 **对象**。
* 为了决定玩家看到什么，我们需要模拟一个 **摄像头**。

这个虚拟相机让我们：

* 环游世界
* 旋转以观察不同的方向
* 放大和缩小（通过投影）
* 并最终控制屏幕上显示的内容

---

### 从世界空间到屏幕空间

以前，我们只使用 **模型矩阵**，它将对象的局部坐标转换为世界坐标。这足以移动物体。但是，为了正确渲染大型3D场景，我们需要引入另外两个变换：

1. **Model Matrix**
   将对象从**局部空间**转换为**世界空间**。

2. **View Matrix**
   将世界转换为 **相机空间**（就像在静止的相机前移动世界一样）。

3. **Projection Matrix**
   将相机空间转换为 **屏幕空间**，应用透视或正交投影。

因此，在顶点着色器中，变换管道变为：

```glsl
gl_Position = uProjection * uView * uModel * vec4(position, 1.0);
```

每个矩阵都有不同的用途：

| Matrix     | Purpose                                 |
| ---------- | --------------------------------------- |
| Model      | 将对象放置在世界中                       |
| View       | 移动世界以模拟相机视图                  |
| Projection | 将三维坐标投影到二维屏幕                 |

> 🔁 注意：视图矩阵是相机变换的 **逆**。我们模拟移动 *世界*，而不是移动 *相机*。

---

## 逐步实现

---

### 1. 创建“CameraComponent”`

在`scene/components`中，创建：

* `CameraComponent.h`
* `CameraComponent.cpp`

#### CameraComponent.h

```cpp
#pragma once

#include "scene/component.h"
#include <glm/glm.hpp>

class CameraComponent : public Component 
{
public:
    // Empty update - 在这种简单的情况下，相机不会随着时间的推移而改变
    void Update(float deltaTime) override {}

    // 返回一个模拟相机视角的视图矩阵
    glm::mat4 GetViewMatrix() const;

    // 返回投影矩阵（透视或正交）
    glm::mat4 GetProjectionMatrix() const;
};
```

#### CameraComponent.cpp

```cpp
#include "CameraComponent.h"
#include "scene/GameObject.h"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 CameraComponent::getViewMatrix() const 
{
    // 对相机进行世界变换并将其反转
    return glm::inverse(m_owner->GetWorldTransform());
}

// 我们稍后将根据我们想要透视投影还是正交投影来定义它
glm::mat4 CameraComponent::GetProjectionMatrix() const 
{
    // 暂时的占位符
    return glm::mat4(1.0f);
}
```

> ✅ 相机只是另一个物体 —— 它和其他物体一样有变换
> ❗ 但与其他对象不同，我们在构建视图矩阵时 **反转** 其变换。

---

### 2. 在场景中添加"主摄像头"

在"Scene"类中，添加一个字段和两个方法：

```cpp
// 每个场景只有一个活动摄像头
GameObject* m_mainCamera = nullptr;

void setMainCamera(GameObject* camera) 
{
    m_mainCamera = camera;
}

GameObject* getMainCamera() const 
{
    return m_mainCamera;
}
```

> 这为我们提供了一个“活动”相机 —— 在渲染时很有用，因为你通常一次只从一个角度进行渲染。

---

### 3. 将视图和投影矩阵传递给着色器

在 `RenderQueue.h`中:

```cpp
struct CameraData 
{
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};
```

更新“Draw”方法:

```cpp
void Draw(GraphicsAPI& graphics, const CameraData& cameraData);
```

在实现中，将摄影机矩阵传递给着色器：

```cpp
shader->setUniform("uView", cameraData.viewMatrix);
shader->setUniform("uProjection", cameraData.projectionMatrix);
```

> 🧠 现在，您为着色器提供了模拟相机和将3D投影到2D所需的所有数据。

---

### 4. 更新顶点着色器

在顶点着色器中：

```glsl
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() 
{
    gl_Position = uProjection * uView * uModel * vec4(position, 1.0);
}
```

这将执行 **MVP** 转换。

---

### 5. 把所有东西都装进引擎里

在“Engine”类中：

```cpp
std::unique_ptr<Scene> m_currentScene;

void SetScene(std::unique_ptr<Scene> scene);
Scene* GetScene() const;
```

在 `Engine::Run()`中:

```cpp
CameraData cameraData;

// If there's an active scene...
if (m_currentScene) 
{
    if (auto* cameraObject = m_currentScene->getMainCamera()) 
    {
        // Try to get the camera component
        if (auto* camera = cameraObject->GetComponent<CameraComponent>()) 
        {
        }
    }
}

// Now pass cameraData into the render queue
m_renderQueue.Draw(m_graphicsAPI, cameraData);
```

---

## 摘要

在本课中，你学习了：

* 为什么我们需要一个 **相机** 来渲染大型3D场景 
* 如何通过反转相机的变换来计算 **视图矩阵** 
* 如何将 **模型→视图→投影**(**Model → View → Projection**)矩阵传递给着色器 
* 如何构建一个具有 **相机感知** 功能的渲染系统

> 现在，你可以渲染大型3D环境，用相机在其中移动，并模拟出真实世界的视角。