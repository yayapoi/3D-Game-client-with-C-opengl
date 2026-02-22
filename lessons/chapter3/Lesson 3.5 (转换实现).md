好的，现在让我们开始实施我们之前讨论的内容。

### 1. 更新`GameObject`

首先，打开`GameObject`类。  
包含以下GLM头文件：

```cpp
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
```

接下来，声明三个私有成员变量：

```cpp
glm::vec3 m_position = glm::vec3(0.0f);
glm::vec3 m_rotation = glm::vec3(0.0f);
glm::vec3 m_scale    = glm::vec3(1.0f);
```

这些代表对象的 **位置**、**旋转** 和 **缩放** 分量。

然后，为每个组件添加getter和setter方法：

```cpp
glm::vec3 GetPosition() const;
void SetPosition(const glm::vec3& pos);

glm::vec3 GetRotation() const;
void SetRotation(const glm::vec3& rot);

glm::vec3 GetScale() const;
void SetScale(const glm::vec3& scale);
```

此外，添加两个方法来计算变换矩阵：

```cpp
glm::mat4 GetLocalTransform() const;
glm::mat4 GetWorldTransform() const;
```

---

### 2. 实现局部变换

`GetLocalTransform`方法将位置、旋转和缩放组合成一个矩阵。

从概念上来看，它是这样的：

```cpp
glm::mat4 mat = glm::mat4(1.0f); // identity

mat = glm::translate(mat, m_position);

// Apply rotation around X, Y, and Z axes
mat = glm::rotate(mat, m_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
mat = glm::rotate(mat, m_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
mat = glm::rotate(mat, m_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

mat = glm::scale(mat, m_scale);

return mat;
```

这为我们提供了最终的 **局部变换** 矩阵。

---

### 3. 实现世界变换

所有对象都存在于一个层次结构中。因此，要计算 **世界变换**，我们还必须考虑父对象的变换：

```cpp
glm::mat4 GetWorldTransform() const 
{
    if (m_parent) 
    {
        return m_parent->GetWorldTransform() * GetLocalTransform();
    } 
    else 
    {
        return GetLocalTransform();
    }
}
```

这样，就能正确计算出对象相对于全局场景的位置，而不仅仅是其局部空间的位置。

---

### 4. 更新着色器

之前在顶点着色器中，我们使用了一个`vec2`类型的 uniform 来进行手动偏移。现在，我们将用 **模型矩阵** 来替换它。

将 uniform 声明修改为：

```glsl
uniform mat4 uModel;
```

然后像这样更新顶点位置变换：

```glsl
gl_Position = uModel * vec4(position, 1.0);
```

这使得GPU能够应用完整的变换矩阵，而不仅仅是简单的二维偏移。

---

### 5. 将矩阵发送到着色器

要将这个矩阵从C++传递到着色器，请转到你的`ShaderProgram`类并添加以下内容：

```cpp
void setUniform(const std::string& name, const glm::mat4& mat);
```

实施:

```cpp
GLint location = GetUniformLocation(name);
glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
```

这将矩阵上传到GPU。

---

### 6. 更新`RenderCommand`

现在更新`RenderCommand`结构，以包含模型矩阵：

```cpp
glm::mat4 modelMatrix;
```

这将为每个对象存储世界变换。

---

### 7. 渲染前设置矩阵

在你的`RenderQueue::DrawAll()`方法中，处理每个渲染命令时，请执行以下操作：

* 从材质中获取着色器 
* 使用`SetUniform`传递模型矩阵

如果您的材质尚未公开着色器程序，请添加一个方法：

```cpp
ShaderProgram* GetShaderProgram();
```

然后调用：

```cpp
command.material->GetShaderProgram()->SetUniform("uModel", command.modelMatrix);
```

---

### 8. 在游戏逻辑中的应用

现在，转到你的测试对象或游戏更新逻辑。
在`Update()`方法中，当创建`RenderCommand`时，请设置：

```cpp
command.modelMatrix = GetWorldTransform();
```

这确保了每一帧都使用最新的变换数据。

由于我们现在使用的是`position`而不是手动偏移量（`offsetX`、`offsetY`），你可以删除那些旧变量。

相反：

```cpp
auto position = GetPosition();

// Update position based on input
if (keyPressed) {
    position.x += 1.0f; // for example
}

SetPosition(position);
```

然后，`GetWorldTransform()`函数将自动计算最终位置。

---

### 9. 测试

现在运行你的程序。从视觉上看，起初并无变化 —— 对象看起来还是一样。

但是，如果你按键来移动它，那么使用新的变换系统，对象现在应该能正确响应输入。

---

### 摘要

现在你已经：

* 一个具备位置、旋转和缩放功能的可运行“Transform”系统。
* 一种基于矩阵的3D渲染方法。
* 与着色器和渲染逻辑的集成。
* 用稳健的转换算法取代了手动偏移逻辑。

你离构建一个灵活的3D引擎又近了一步！