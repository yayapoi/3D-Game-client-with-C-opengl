# 添加光线（漫射照明）

正如你所注意到的，我们所有的物体 —— 无论是立方体还是像 Suzanne 这样的加载3D模型 —— 都是在3D空间中绘制的，但它们看起来……是平的。

是什么给了他们体积的错觉？ **照明**

光照会产生阴影和高光，使这些微小的凸起、凹痕和折痕可见，这取决于光源的位置、颜色等。没有光照，一切看起来都没有生气。

因此，让我们通过在引擎中实现 **light**，为场景带来更多的美感和深度。

我们将从简单但能立即产生强烈视觉效果的东西开始：**漫射照明**。

---

## 漫射照明的概念

漫反射照明的核心思想非常简单。

想象一下，我们有一个 **点光源** —— 就像一个发光的球体或一盏灯。它位于3D空间的某个位置，在各个方向上均匀发光。

光向外传播，最终到达我们物体的表面。光线照射到表面上的一个小点（一个*片段*）。

该片段的亮度取决于入射光线与曲面 **法向量** 之间的 **角度**。

* **法线** 只是一个垂直于曲面的向量。
* 由于我们所有的对象都是由三角形组成的，因此每个三角形都是平的，并且有自己的法线。

因此，要计算片段的光照，我们只需要 **两个向量**：

1. 该片段处的 **法向量**。
2. **光方向矢量**（从片段朝向光源）。

我们如何计算光线的方向？

```cpp
lightDir = normalize(lightPosition - fragPosition);
```

现在我们取法线和lightDir的 **点积**（标量积）：

```cpp
float diff = dot(normal, lightDir);
```

* 如果光直接位于片段上方（法线和lightDir对齐），则点积为“1” → 最大亮度。
* 如果光线与表面成90°，则点积为“0” → 完全黑暗。

很简单，对吧？考虑到这个概念，让我们一步一步地实现它。

---

## 光照组件

我们将首先创建一个“LightComponent”。

在“scene/components/”中，添加两个新文件：

* `LightComponent.h`
* `LightComponent.cpp`

当然，在“CMakeLists.txt”中注册它们并包含。

“LightComponent”将有两个主要属性：

* **位置**（但它来自其父游戏对象，因此不需要单独存储）。
* **颜色**（让我们储存这个）。

```cpp
glm::vec3 m_color = glm::vec3(1.0f); //默认：白光
```

---

## 收集灯光

与通常只有一个活动相机的相机不同，我们在一个场景中可能有多个灯光。

所以我们会把它们都收集起来。

为了保持整洁，让我们在“engine/source/”中创建一个新的公共标头“common.h”。它将存储可重用的数据结构。

将“CameraData”移到此处，并添加：

```cpp
struct LightData 
{
    glm::vec3 color;
    glm::vec3 position;
};
```

现在在“Scene”类中，添加一个方法：

```cpp
std::vector<LightData> CollectLights();
```

其实施将：

1. 遍历所有对象。
2. 递归检查它们是否有“LightComponent”。
3. 如果是，抓住它的颜色和世界坐标。

对于递归，添加一个私有函数：

```cpp
void CollectLightsRecursive(GameObject* obj, std::vector<LightData>& out);
```

里面：

```cpp
if (auto* light = obj->GetComponent<LightComponent>()) 
{
    LightData data;
    data.color = light->GetColor();
    data.position = obj->GetWorldPosition(); // global position
    out.push_back(data);
}
for (auto& child : obj->m_children) 
{
    CollectLightsRecursive(child.get(), out);
}
```

最后：

```cpp
std::vector<LightData> Scene::CollectLights() 
{
    std::vector<LightData> lights;
    for (auto& obj : mObjects)
        CollectLightsRecursive(obj.get(), lights);
    return lights;
}
```

---

## 获得世界坐标

为了计算世界空间中的光线位置，我们将在“GameObject”中添加一个助手：

```cpp
glm::vec3 getWorldPosition() const 
{
    glm::vec4 hom = getWorldTransform() * glm::vec4(0,0,0,1);
    return glm::vec3(hom) / hom.w;
}
```

---

## 将光线传递到渲染器

在`Engine:：Update（）`中：

```cpp
auto lights = currentScene->CollectLights();
```

将此向量传递给`RenderQueue：：Draw（）`。
在`RenderQueue：：Draw（）`中，现在我们只使用 **第一盏灯**：

```cpp
if (!lights.empty()) 
{
    const auto& light = lights[0];
    //将其传递给着色器
}
```

---

## 着色器中的光线 Uniform

在片段着色器中，声明一个结构体：

```glsl
struct Light 
{
    vec3 color;
    vec3 position;
};

uniform Light uLight;
```

现在，从C++中，我们可以设置：

```cpp
shader->SetUniform("uLight.color", light.color);
shader->SetUniform("uLight.position", light.position);
```

要支持这一点，请添加“ShaderProgram”：

```cpp
void SetUniform(const std::string& name, const glm::vec3& value) 
{
    GLint loc = GetUniformLocation(name);
    glUniform3fv(loc, 1, glm::value_ptr(value));
}
```

---

## 添加法线

为了计算光照，我们需要 **法线**。
* 在“VertexLayout.h”中添加新的属性索引：

```cpp
static constexpr int NormalIndex = 3;
```

* 加载网格时，将 accessors 数组大小扩展到4。
* 如果属性的类型为“NORMAL”，请将其映射到“NormalIndex”。

```cpp
if (attr.type == cgltf_attribute_type_normal) 
{
    accessors[VertexElement::NormalIndex] = attr.data;
    element.index  = VertexElement::NormalIndex;
    element.type   = GL_FLOAT;
    element.size   = 3;
    element.offset = vertexLayout.stride;
    vertexLayout.stride += 3 * sizeof(float);
    vertexLayout.elements.push_back(element);
}
```

---

## 顶点着色器更改

我们现在需要通过：

* 法线（在世界空间中）。
* 片段位置（在世界空间中）。

```glsl
layout(location = 3) in vec3 aNormal;

out vec3 vNormal;
out vec3 vFragPos;

void main() 
{
    // position in world space
    vFragPos = vec3(uModel * vec4(aPos, 1.0));

    // transform normal into world space
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;

    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
```

---

## 片段着色器更改

```glsl
in vec3 vNormal;
in vec3 vFragPos;

uniform Light uLight;
uniform sampler2D baseColorTexture;

void main() 
{
    // normalize normal
    vec3 norm = normalize(vNormal);

    // light direction
    vec3 lightDir = normalize(uLight.position - vFragPos);

    // diffuse factor
    float diff = max(dot(norm, lightDir), 0.0);

    // diffuse component
    vec3 diffuse = diff * uLight.color;

    // sample texture
    vec4 texColor = texture(baseColorTexture, vUV);

    // final color
    vec3 result = diffuse * texColor.rgb;
    fragColor = vec4(result, 1.0);
}
```

---

## 在场景中添加灯光

在“game.cpp”中：

```cpp
auto& lightObj = scene.createObject("Light");
auto& light = lightObj.addComponent<LightComponent>();
light.setColor(glm::vec3(1.0f, 1.0f, 1.0f)); // white
lightObj.setPosition(glm::vec3(0.0f, 5.0f, 0.0f));
```

现在 Suzanne 将被点亮。

---

## 立方体的法线

但是等等，我们的立方体是漆黑的。为什么？因为我们从来没有给他们法线。
让我们解决这个问题。
我们将把立方体创建移动到`Mesh：createCube（）`中，并为每个面添加法线：
* 正面：`（0，0，1）`
* 返回：`（0，0，-1）`
* 顶部：`（0，1，0）`
* 底部：`（0，-1，0）`
* 右：`(1, 0, 0)`
* 左：`(-1, 0, 0)`

更新顶点布局：

```cpp
vertexLayout.elements.push_back({ VertexElement::NormalIndex, 3, GL_FLOAT, ... });
vertexLayout.stride = 11 * sizeof(float); // pos(3) + color(3) + normal(3) + uv(2)
```

---

## 结果

运行项目。

现在 Suzanne 看起来是三维的，有适当的阴影。我们的立方体 —— 现在有了法线 —— 也被照亮了。

恭喜🎉 我们刚刚为我们的世界增添了**光**。事物不再平坦；它们现在有深度、阴影和高光。

欢迎来到照亮的3D世界！