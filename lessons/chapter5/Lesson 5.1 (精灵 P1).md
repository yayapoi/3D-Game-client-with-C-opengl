### 用精灵(Sprites)介绍2D

所以，我们现在知道如何在3D空间中完全工作。而且，正如你所记得的，我们实际上是从2D到3D的。但这里有一个小提示：我们之前使用的“2D空间”只是一个**垫脚石** —— 一个为我们现在拥有的成熟3D系统做好准备的训练阶段。

所以，是的，我们现在可以构建3D世界和3D游戏。但问题是：  
**如果我们想在真实的2D空间中制作一个真正的2D游戏呢？**

好吧，答案出奇地简单：我们几乎已经拥有了所需的一切。唯一缺少的是一个定义2D渲染的基础元素。该元素是**sprite**。

---

### 什么是精灵？

当我解释它时，你会立刻认出它。**精灵** 只是一个 **平面** —— 一个顶部应用了纹理的矩形。听起来很熟悉，对吧？我们以前画过非常相似的东西。

但精灵确实有一些特殊的特征：

1. **原点（坐标起点）**

    * 角色的坐标系从**左下角**开始，而不是中心。

2. **尺寸**

    * 精灵有宽度（X）和高度（Y）。

3. **UV坐标**

    * 精灵可能会使用纹理的一部分（如精灵图集中的一个区域），而不是整个纹理。因此，两个UV坐标（左下和右上）定义了要使用纹理的哪个部分。

4. **枢轴点**

    * 这定义了角色旋转的点。
    * 默认情况下，枢轴位于中心（0.5，0.5）。
    * 相对于角色的大小，枢轴值的范围为0到1。

5. **2D变换**

    * 与3D对象不同，精灵仅沿 **X和Y** 移动，并且仅围绕 **Z轴** 旋转。
    * 这大大简化了变换：在2D中更容易管理平移、缩放和旋转。

---

### 实现：精灵组件

正如您所猜测的，我们将把精灵实现为**新组件**。

* 进入`scene/components`并创建两个文件：

    * `SpriteComponent.h`
    * `SpriteComponent.cpp`

声明类：

```cpp
class SpriteComponent : public Component 
{
    ...
};
```

别忘了将其添加到`CMakeLists`和`eng.h`中。

---

### SpriteComponent的领域

精灵存储：

* `std::shared_ptr<Texture> m_texture;`
    应用于角色的纹理。

* `glm::vec4 m_color = glm::vec4(1.0f);`
    默认颜色为白色（无色调）。

* `glm::vec2 m_size = glm::vec2(100.0f);`
    默认大小为100x100。

* `glm::vec2 m_lowerLeftUV = glm::vec2(0.0f);`
    左下角的UV。

* `glm::vec2 m_upperRightUV = glm::vec2(1.0f);`
    右上角的UV。

* `glm::vec2 m_pivot = glm::vec2(0.5f);`
    枢轴点（默认为中心）。

* `bool m_visible = true;`
    控制角色可见性。

当然，我们还将为所有这些字段添加**getter和setter**：

```cpp
void SpriteComponent::SetTexture(const std::shared_ptr<Texture>& texture)
{
    m_texture = texture;
}

const std::shared_ptr<Texture>& SpriteComponent::GetTexture() const
{
    return m_texture;
}

void SpriteComponent::SetColor(const glm::vec4& color)
{
    m_color = color;
}

const glm::vec4& SpriteComponent::GetColor() const
{
    return m_color;
}

void SpriteComponent::SetSize(const glm::vec2& size)
{
    m_size = size;
}

const glm::vec2& SpriteComponent::GetSize() const
{
    return m_size;
}

void SpriteComponent::SetLowerLeftUV(const glm::vec2& uv)
{
    m_lowerLeftUV = uv;
}

const glm::vec2& SpriteComponent::GetLowerLeftUV() const
{
    return m_lowerLeftUV;
}

void SpriteComponent::SetUpperRightUV(const glm::vec2& uv)
{
    m_upperRightUV = uv;
}

const glm::vec2& SpriteComponent::GetUpperRightUV() const
{
    return m_upperRightUV;
}

void SpriteComponent::SetUV(const glm::vec2& lowerLeftUV, const glm::vec2& upperRightUV)
{
    m_lowerLeftUV = lowerLeftUV;
    m_upperRightUV = upperRightUV;
}

void SpriteComponent::SetPivot(const glm::vec2& pivot)
{
    m_pivot = pivot;
}

const glm::vec2& SpriteComponent::GetPivot() const
{
    return m_pivot;
}

void SpriteComponent::SetVisibile(bool visible)
{
    m_visible = visible;
}

bool SpriteComponent::IsVisible() const
{
    return m_visible;
}
```

---

### 从JSON加载

重写`LoadProperties`进行加载：

* Texture
* Color
* Size
* UVs
* Pivot

```cpp
void SpriteComponent::LoadProperties(const nlohmann::json& jsonObject)
{
    // Texture
    const std::string texturePath = jsonObject.value("texture", "");
    if (auto texture = Texture::Load(texturePath))
    {
        SetTexture(texture);
    }
    // color
    if (jsonObject.contains("color"))
    {
        auto& colorObj = jsonObject["color"];
        glm::vec4 color;
        color.r = colorObj.value("r", 1.0f);
        color.g = colorObj.value("g", 1.0f);
        color.b = colorObj.value("b", 1.0f);
        color.a = colorObj.value("a", 1.0f);
        SetColor(color);
    }
    // size
    if (jsonObject.contains("size"))
    {
        auto& sizeObj = jsonObject["size"];
        glm::vec2 size;
        size.x = sizeObj.value("x", 100.0f);
        size.y = sizeObj.value("y", 100.0f);
        SetSize(size);
    }
    // lowerLeftUV
    if (jsonObject.contains("lowerLeftUV"))
    {
        auto& uvObj = jsonObject["lowerLeftUV"];
        glm::vec2 uv;
        uv.x = uvObj.value("u", 0.0f);
        uv.y = uvObj.value("v", 0.0f);
        SetLowerLeftUV(uv);
    }
    // upperRightUV
    if (jsonObject.contains("upperRightUV"))
    {
        auto& uvObj = jsonObject["upperRightUV"];
        glm::vec2 uv;
        uv.x = uvObj.value("u", 1.0f);
        uv.y = uvObj.value("v", 1.0f);
        SetUpperRightUV(uv);
    }
    // pivot
    if (jsonObject.contains("pivot"))
    {
        auto& pivotObj = jsonObject["pivot"];
        glm::vec2 pivot;
        pivot.x = pivotObj.value("x", 0.5f);
        pivot.y = pivotObj.value("y", 0.5f);
        SetPivot(pivot);
    }
}
```

同时覆盖`Update`：

```cpp
void SpriteComponent::Update(float deltaTime) override 
{
    if (!m_texture || !m_visible) return;
    // 现在，我们把它留空
}
```

---

### 注册组件

在`Scene::RegisterTypes`中，注册它：

```cpp
SpriteComponent::Register();
```

---

### 便利性：游戏对象中的2D变换

在2D中工作也应该很方便。那么，让我们在`GameObject`中添加一些辅助方法。

对于职位：

```cpp
glm::vec2 GetPosition2D()
{
    return glm::vec2(m_position);
}

void SetPosition2D(const glm::vec2& pos)
{
    m_position = glm::vec3(pos, 0.0f);
}

```

对于旋转（仅围绕Z）：

```cpp
float GetRotation2D()
{
    return glm::angle(m_rotation);
}

void SetRotation2D(float rotation)
{
    m_rotation = glm::angleAxis(rotation, glm::vec3(0.0f, 0.0f, 1.0f));
}
```

关于缩放：

```cpp
glm::vec2 GetScale2D()
{
    return glm::vec2(m_scale);
}

void SetScale2D(const glm::vec2& scale)
{
    m_scale = glm::vec3(scale, 1.0f);
}
```

并转换：

```cpp
glm::mat4 GetLocalTransform2D() const
{
    glm::mat4 mat = glm::mat4(1.0f);

    const auto rotationZ = GetRotation2D();
    float c = cos(rotationZ);
    float s = sin(rotationZ);

    mat[0][0] = m_scale.x * c;
    mat[0][1] = m_scale.x * s;
    mat[1][0] = -m_scale.y * s;
    mat[1][1] = m_scale.y * c;
    mat[3][0] = m_position.x;
    mat[3][1] = m_position.y;

    return mat;
}

glm::mat4 GameObject::GetWorldTransform2D() const
{
    if (m_parent)
    {
        return m_parent->GetWorldTransform2D() * GetLocalTransform2D();
    }
    else
    {
        return GetLocalTransform2D();
    }
}
```

最后：

```cpp
glm::vec2 GetWorldPosition2D() const
{
    glm::vec4 hom = GetWorldTransform2D() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    return glm::vec2(hom) / hom.w;
}
```

在这些内部，我们简单地调整了3D等价内容的逻辑，但**忽略了Z**。

---

### 渲染精灵

现在我们需要实际渲染精灵。就像`MeshComponent`处理3D网格一样，`SpriteComponent`将处理2D精灵。

在`RenderQueue.h `中，定义一个新结构：

```cpp
struct RenderCommand2D 
{
    glm::mat4 modelMatrix;
    Texture* texture = nullptr;
    glm::vec4 color;
    glm::vec2 size;
    glm::vec2 lowerLeftUV;
    glm::vec2 upperRightUV;
    glm::vec2 pivot;
};
```

在`RenderQueue`中，添加：

* 一个新的向量`std::vector<RenderCommand2D> m_commands2D;`
* 一个新的`Submit(const RenderCommand2D& command)`方法，将命令推送到其中。

在`DrawAll()`中，渲染3D世界后，**迭代`m_commands2D`** 并绘制所有精灵：

```cpp
for (auto& command : m_comands2D)
{

}
```

---

### 精灵几何

精灵需要什么几何图形？只是一个**平面**。

因此，在`Mesh`中添加：

```cpp
static std::shared_ptr<Mesh> CreatePlane();
```

这将创建一个具有四个顶点的简单矩形：

```cpp
std::shared_ptr<Mesh> Mesh::CreatePlane()
{
    std::vector<float> vertices =
    {
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
    };

    std::vector<uint32_t> indices =
    {
        0, 1, 2,
        0, 2, 3
    };

    eng::VertexLayout vertexLayout;

    // Postion
    vertexLayout.elements.push_back({
        VertexElement::PositionIndex,
        2,
        GL_FLOAT,
        0
        });
    vertexLayout.stride = sizeof(float) * 2;

    auto result = std::make_shared<eng::Mesh>(vertexLayout, vertices, indices);

    return result;
}
```

在“RenderQueue”中，添加：

```cpp
std::shared_ptr<Mesh> m_mesh2D;
```

在新方法`RenderQueue::Init()`中初始化它：

```cpp
m_mesh2D = Mesh::CreatePlane();
```

在`Engine::Init()`中，调用：

```cpp
m_renderQueue.Init();
```

绘制二维命令时：

* 绑定`m_mesh2D`
* 绘制每个`RenderCommand2D`
* 之后解除绑定

```cpp
m_mesh2D->Bind();
for (auto& command : m_comands2D)
{
    //...
    m_mesh2D->Draw();
}
m_mesh2D->Unbind();
```

---

### 精灵着色器

最后，我们需要一个**2D着色器**。

在`GraphicsAPI`中，添加：

```cpp
std::shared_ptr<ShaderProgram> m_default2DShaderProgram;
const std::shared_ptr<ShaderProgram>& GetDefault2DShaderProgram() const;
```

此着色器将获取纹理和颜色，并相应地渲染精灵。

---

这就是精灵的第一部分。  
在下一部分中，我们将继续构建2D渲染系统。