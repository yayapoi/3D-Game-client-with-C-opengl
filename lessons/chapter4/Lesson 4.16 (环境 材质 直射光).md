# 环境光、材质和定向阳光

让我们继续为场景添加有趣的功能。  
这一次，我们将改进**照明、材质和渲染**，使场景更具可玩性和视觉清晰度。

---

1. 环境光（AMBIENT）

如果你看看当前的照明，你会注意到：

* 我们已经有了 **漫射照明**，
* 我们已经有了 **镜面照明**，
* 但背对光源的表面是完全黑色的，仿佛完全黑暗。

事实上，环境中总是有**环境光** —— 一些全局亮度。让我们添加它。

打开`shaders/fragment.glsl`。  
在“main”函数中，添加：

```glsl
const float ambientStrength = 0.4; 
vec3 ambient = ambientStrength * uLight.color;
```

这创造了我们的环境光。

现在，更新最终结果：

```glsl
vec3 result = (ambient + diffuse + specular) * textureColor.rgb;
```

在默认着色器的**GraphicsAPI**设置中执行相同的操作，以便始终应用环境光。

---

2. 更新 Mesh::CreateBox 的 UVs

让我们改进`Mesh::CreateBox`，使其生成**正确的UV坐标**。

我们假设默认纹理是为**1×1×1立方体**设计的。  
这意味着我们必须以一种与纹理很好地配合的方式映射 UV（它们将在缩放的框中自动重复）。

更新`CreateBox`中的 UV 以指定适当的纹理坐标：

```cpp
std::shared_ptr<Mesh> Mesh::CreateBox(const glm::vec3& extents)
{
    const auto half = extents * 0.5f;
    std::vector<float> vertices =
    {
        // Front face
        half.x, half.y, half.z, 1.0f, 0.0f, 0.0f, extents.x, extents.y, 0.0f, 0.0f, 1.0f,
        -half.x, half.y, half.z, 0.0f, 1.0f, 0.0f, 0.0f, extents.y, 0.0f, 0.0f, 1.0f,
        -half.x, -half.y, half.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        half.x, -half.y, half.z, 1.0f, 1.0f, 0.0f, extents.x, 0.0f, 0.0f, 0.0f, 1.0f,

        // Top face 
        half.x, half.y, -half.z, 1.0f, 0.0f, 0.0f, extents.x, extents.z, 0.0f, 1.0f, 0.0f,
        -half.x, half.y, -half.z, 0.0f, 1.0f, 0.0f, 0.0f, extents.z, 0.0f, 1.0f, 0.0f,
        -half.x, half.y, half.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        half.x, half.y, half.z, 1.0f, 1.0f, 0.0f, extents.x, 0.0f, 0.0f, 1.0f, 0.0f,

        // Right face
        half.x, half.y, -half.z, 1.0f, 0.0f, 0.0f, extents.z, extents.y, 1.0f, 0.0f, 0.0f,
        half.x, half.y, half.z, 0.0f, 1.0f, 0.0f, 0.0f, extents.y, 1.0f, 0.0f, 0.0f,
        half.x, -half.y, half.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        half.x, -half.y, -half.z, 1.0f, 1.0f, 0.0f, extents.z, 0.0f, 1.0f, 0.0f, 0.0f,

        // Left face
        -half.x, half.y, half.z, 1.0f, 0.0f, 0.0f, extents.z, extents.y, -1.0f, 0.0f, 0.0f,
        -half.x, half.y, -half.z, 0.0f, 1.0f, 0.0f, 0.0f, extents.y, -1.0f, 0.0f, 0.0f,
        -half.x, -half.y, -half.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        -half.x, -half.y, half.z, 1.0f, 1.0f, 0.0f, extents.z, 0.0f, -1.0f, 0.0f, 0.0f,

        // Bottom face
        half.x, -half.y, half.z, 1.0f, 0.0f, 0.0f, extents.x, extents.z, 0.0f, -1.0f, 0.0f,
        -half.x, -half.y, half.z, 0.0f, 1.0f, 0.0f, 0.0f, extents.z, 0.0f, -1.0f, 0.0f,
        -half.x, -half.y, -half.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
        half.x, -half.y, -half.z, 1.0f, 1.0f, 0.0f, extents.x, 0.0f, 0.0f, -1.0f, 0.0f,

        // Back face
        -half.x, half.y, -half.z, 1.0f, 0.0f, 0.0f, extents.x, extents.y, 0.0f, 0.0f, -1.0f,
        half.x, half.y, -half.z, 0.0f, 1.0f, 0.0f, 0.0f, extents.y, 0.0f, 0.0f, -1.0f,
        half.x, -half.y, -half.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,
        -half.x, -half.y, -half.z, 1.0f, 1.0f, 0.0f, extents.x, 0.0f, 0.0f, 0.0f, -1.0f
    };
    // ...
}
```

---

3. 棋盘格纹理

我准备了一个简单的**棋盘格纹理**，这在测试引擎中非常常见。

它被称为`checker.png`，放在`assets/textures/`中。

现在，让我们创建一种新材质：

* 复制`brick.mat`，
* 将`baseColorTexture`替换为`textures/checker.png`。

那是我们的**棋盘格材质**。

---

4. 使用棋盘格构建场景

让我们使用新材质构建一个**更大、更有趣**的场景。

删除`scene.cs`中的所有内容，而是创建一个包含**多个盒子**和棋盘格材质的新场景：

* 一些箱子散落一地，
* 一些堆叠在一起（这样你开枪时它们可能会掉下来），
* 四面墙环绕着这个区域，
* 一个简单的平台。

当你玩的时候，你会看到盒子被子弹击中后翻滚和倒塌。
然而，所有对象仍然共享相同的棋盘格纹理，这有时会使它们难以区分。

---

5. 向材质添加颜色参数

让我们添加**颜色覆盖**，这样我们就可以对对象进行不同的着色。

### 第一步：材质储存

在`Material`类中添加：

```cpp
std::unordered_map<std::string, glm::vec3> m_float3Params;
```

这将存储“vec3”参数，如颜色。

添加：

```cpp
void SetParam(const std::string& name, const glm::vec3& param) 
{
    m_float3Params[name] = param;
}
```

### 第二步：绑定

在`Material:：Bind（）`中，绑定纹理后，迭代参数：

```cpp
for (auto& param : m_float3Params) 
{
    m_shaderProgram->SetUniform(param.first, param.second);
}
```

现在，材质可以将自定义值传递到着色器中。

### 步骤3：从JSON加载

在`Material:：Load（）`中，检查`float3`参数：

```cpp
if (paramsObj.contains("float3")) 
{
    for (auto& p : paramsObj["float3"]) 
    {
        std::string name = p.value("name", "");
        float v0 = p.value("value0", 0.0f);
        float v1 = p.value("value1", 0.0f);
        float v2 = p.value("value2", 0.0f);
        result->SetParam(name, glm::vec3(v0, v1, v2));
    }
}
```

### 步骤4：在 MeshComponent 中覆盖

在`MeshComponent::LoadProperties()`中，支持一个对象，而不是只将`material`读取为字符串：

```json
"material": {
  "path": "materials/checker.mat",
  "params": {
    "float3": [
      { "name": "color", "value0": 1, "value1": 0, "value2": 0 }
    ]
  }
}
```

在`fragment.glsl`中添加一个 uniform 的`uniform vec3 color;`并在最终颜色计算中使用：

```glsl
vec3 result = (ambient + diffuse + specular) * texColor.xyz * color;
```

通过这种方式，我们可以加载基础材质，然后覆盖其参数（例如色调）。

现在我们可以让不同的盒子变成不同的颜色，即使它们使用相同的格子纹理。

---

6. 方向光（阳光）

现在我们的灯光是**点光源**。让我们切换到**平行光**，就像太阳一样。

### 着色器更改

在`fragment.glsl`的`Light`结构中，将基于位置的逻辑替换为方向：

```glsl
vec3 lightDir = normalize(-uLight.direction);
```

因此，我们直接使用固定方向，而不是计算从位置到碎片的光方向。

### 渲染队列

在默认着色器的“RenderQueue”设置中，替换：

```cpp
shader->SetUniform("uLight.direction", glm::normalize(-light.position));
```

现在，光线就像来自太阳的平行光线。

---

## 结果

启动游戏：

* 由于**环境光**，物体现在更亮了，
* 棋盘格纹理无处不在，
* 盒子可以用**定制颜色**着色，
* 照明来自**定向太阳源**。

您的场景更加丰富多彩、可定制和逼真