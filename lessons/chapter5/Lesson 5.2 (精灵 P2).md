### 构建2D着色器和渲染精灵

好的，现在让我们实现用于精灵渲染的着色器。

---

### 顶点着色器

顶点着色器将采用：

* 位置(`layout(location = 0) in vec2 position;`)
* 要传递给片段着色器的UV坐标(`out vec2 vUV;`)
* 三个矩阵：**模型**、**视图**和**投影**：
```glsl
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
```
* 转轴和大小（用于计算精灵几何体）：
```glsl
uniform vec2 uPivot;
uniform vec2 uSize;
```
* 左下角和右上角 UV 坐标：
```glsl
uniform vec2 uUVMin;      // lowerLeftCornerUV
uniform vec2 uUVMax;      // upperRightCornerUV
```

在`main（）`函数中，会发生以下情况：

1. **转换坐标**

    ```glsl
    vec2 local = (position - uPivot) * uSize;
    ```

    * 我们通过转轴进行偏移，并根据大小进行缩放。

2. **重新计算UVs**

    ```glsl
    vUV = mix(uUVMin, uUVMax, position);
    ```

    * 这将根据顶点位置插值UV坐标。

3. **输出最终位置**

    ```glsl
    gl_Position = uProjection * uView * uModel * vec4(local, 0.0, 1.0);
    ```

这就是整个**顶点着色器**。

---

### 片段着色器

片段着色器接收：

* 插值UV(`in vec2 vUV;`)
* uniform颜色(`uniform vec4 uColor;`)
* uniform纹理采样器(`uniform sampler2D uTex;`)
* 输出颜色(`out vec4 FragColor;`)

实现：

```glsl
vec4 src = texture(uTex, vUV) * uColor;
FragColor = src;
```

就是这样 —— 片段着色器将采样纹理颜色乘以 uniform 色调颜色。

---

### 连接到RenderQueue

现在，让我们将其集成到我们的渲染管道中。

* 在`RenderQueue::Draw`中，在调用`m_mesh2D->Bind()`之前，激活着色器：

    ```cpp
    const auto shaderProgram2D = graphicsAPI.GetDefault2DShaderProgram();
    shaderProgram2D->Bind();
    ```

* 设置所有必需的 uniforms：

    * `uModel`
    ```cpp
    shaderProgram2D->SetUniform("uModel", command.modelMatrix);
    ```
    * `uView`
    ```cpp
    shaderProgram2D->SetUniform("uView", cameraData.viewMatrix);
    ```
    * `uSize`
    ```cpp
    shaderProgram2D->SetUniform("uSize", command.size.x, command.size.y);
    ```
    * `uPivot`
    ```cpp
    shaderProgram2D->SetUniform("uPivot", command.pivot.x, command.pivot.y);
    ```
    * `uUVMin`
    ```cpp
    shaderProgram2D->SetUniform("uUVMin", command.lowerLeftUV.x, command.lowerLeftUV.y);
    ```
    * `uUVMax`
    ```cpp
    shaderProgram2D->SetUniform("uUVMax", command.upperRightUV.x, command.upperRightUV.y);
    ```
    * Go to `ShaderProgram.h` and add:
    ```cpp
    void ShaderProgram::SetUniform(const std::string& name, const glm::vec4& value)
    {
        auto location = GetUniformLocation(name);
        glUniform4fv(location, 1, glm::value_ptr(value));
    }
    ```
    * `uColor`
    ```cpp
    shaderProgram2D->SetUniform("uColor", command.color);
    ```
    * `uTex`
    ```cpp
    shaderProgram2D->SetTexture("uTex", command.texture);
    ```

---

### 正射投影

与使用**透视投影**（模拟眼睛）的3D渲染不同，2D使用**正交投影**。

为此，请执行以下操作：

1. 在`CameraData`（来自`Common.h`）中，添加一个字段：

    ```cpp
    glm::mat4 orthoMatrix;
    ```

2. 在`RenderQueue`中，绘制2D时将此矩阵设置为`uProjection`：
    ```cpp
    shaderProgram2D->SetUniform("uProjection", cameraData.orthoMatrix);
    ```

3. 在`Engine::Run`中，填写`CameraData`时：

    ```cpp
    cameraData.orthographicMatrix = glm::ortho(
        0.0f, static_cast<float>(width),
        0.0f, static_cast<float>(height)
    );
    ```

现在，2D渲染可以使用正交摄影机。

---

### 深度测试和透明度

有一个问题：如果我们在`z = 0`处渲染2D对象，它们可能会与3D对象冲突。为防止这种情况发生：

1. 添加到`GraphicsAPI`：

    ```cpp
    void SetDepthTestEnabled(bool enabled);
    ```

    * 启用/禁用OpenGL深度测试：
    ```cpp
    void GraphicsAPI::SetDepthTestEnabled(bool enabled)
    {
        if (enabled)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }
    }
    ```

2. 在`RenderQueue::Draw`中：

* 渲染2D之前：`graphicsAPI.SetDepthTestEnabled(false);`
* 渲染后：`graphicsAPI.SetDepthTestEnabled(true);`

这确保了3D深度不会干扰2D。

---

#### 透明度和混合

精灵通常需要透明度。让我们添加混合模式。

在`GraphicsAPI`中：

```cpp
enum class BlendMode 
{
    Disabled,
    Alpha,
    Additive,
    Multiply
};

void SetBlendMode(BlendMode mode);
```

实现（简化）：

* **Disabled** → `glDisable(GL_BLEND)`
* **Alpha** → `glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);`
* **Additive** → `glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);`
* **Multiply** → `glEnable(GL_BLEND); glBlendFunc(GL_DST_COLOR, GL_ZERO);`

当然，如果未设置任何内容，则默认返回**Disabled**：

```cpp
void GraphicsAPI::SetBlendMode(BlendMode mode)
{
    switch (mode)
    {
    case BlendMode::Disabled:
    {
        glDisable(GL_BLEND);
    }
    break;
    case BlendMode::Alpha:
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    break;
    case BlendMode::Additive:
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
    }
    break;
    case BlendMode::Multiply:
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
    }
    break;
    default:
    {
        glDisable(GL_BLEND);
    }
        break;
    }
}
```

---

在`RenderQueue::Draw`中：

    * 渲染2D:`graphicsAPI.SetBlendMode(BlendMode::Alpha);`
    * 渲染后：`graphicsAPI.SetBlendMode(BlendMode::Disabled);`


---

### 测试精灵系统

测试时间到了！

在`Game.cpp`中：

1. 注释掉旧场景加载。
2. 创建新的空场景：

```cpp
m_scene = std::make_shared<Scene>();
eng::Engine::GetInstance().SetScene(m_scene.get());
```

3. 创建精灵：

```cpp
auto sprite = m_scene->CreateObject("sprite");
auto spriteComponent = new eng::SpriteComponent();

auto texture = eng::Texture::Load("textures/brick.png");
spriteComponent->setTexture(texture);

sprite->AddComponent(spriteComponent);
sprite->SetPosition2D(glm::vec2(500, 500));
spriteComponent->SetSize(glm::vec2(200, 100));
spriteComponent->SetUpperRightUV(glm::vec2(2.0f, 1.0f));
sprite->SetRotation2D(glm::radians(45.0f));
```

4. 创建相机：

```cpp
auto camera = m_scene->CreateObject("camera");
auto cameraComponent = new eng::CameraComponent();
camera->AddComponent(cameraComponent);
m_scene->SetMainCamera(camera);
```

---

在测试之前，转到`SpriteComponent:：Update（）`并提交渲染命令：

```cpp
void SpriteComponent::Update(float deltaTime)
{
    if (!m_texture || !m_visible)
    {
        return;
    }

    RenderCommand2D command;
    command.modelMatrix = GetOwner()->GetWorldTransform2D();
    command.texture = m_texture.get();
    command.color = m_color;
    command.size = m_size;
    command.lowerLeftUV = m_lowerLeftUV;
    command.upperRightUV = m_upperRightUV;
    command.pivot = m_pivot;

    auto& renderQueue = Engine::GetInstance().GetRenderQueue();
    renderQueue.Submit(command);
}
```

---

### 结果

运行程序。您将看到：

* 在位置`（500，500）`处渲染的精灵
* 具有**砖纹理**
* 适当的大小`（200x100）`
* 旋转**45°**
* 校正UV

---

### 结论

恭喜你！

我们从2D开始作为准备阶段，然后构建了一个完整的**3D引擎**，包括组件、场景和对象。现在我们回过头来：我们重新引入了**真正的2D渲染**和精灵 —— 但这一次是在现代引擎基础架构中。

这为您提供了在**3D世界**和**2D游戏**中无缝工作的自由。