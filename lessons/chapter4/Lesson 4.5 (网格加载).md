# 加载三维模型（网格）

好了，是时候加强我们的游戏，学习如何加载更酷的东西了 —— **3D模型**，或网格。

你可能知道，3D模型可以有各种格式。它们太多了，你很容易在挑选一个时迷路。不过，为了我们的目的，我们不要把事情搞得过于复杂。我们只同意使用一种格式：**GLTF**。

GLTF 是由 Khronos 集团开发的一种相对较新的格式。老实说，它对我们来说是完美的：现代、紧凑、广泛支持。

因此，就像图像一样，我们的任务是能够：

1. 将GLTF文件读入内存。
2. 解析它。
3. 将顶点和索引数据提取到缓冲区中。

为了使生活更轻松，我为您准备了一个名为**cgltf**的仅包含头部的小型库。它的工作方式与 stb\_image 或 nlohmann/json 相同：只需将标头放入您的`thirdparty`文件夹中，并将其添加到CMake中：

```cmake
include_directories(third_party/cgltf)
```

就是这样。没有链接，没有配置。

---

## 准备资产

在“assets”文件夹中，让我们创建一个名为“models”的新文件夹。
我们将把以下内容放入其中：

* `Suzanne.gltf` —— 这基本上只是一个描述几何体的JSON文件。
* `Suzanne.bin` —— 一个包含所有原始顶点/索引数据的大型二进制缓冲区。
* `Suzanne_BaseColor.png` —— 一种纹理。
* `Suzanne_MetallicRoughness.png` —— 另一种纹理。

现在，我们只关心“.gltf”和“.bin”文件。

---

## 网格加载器骨架

在`Mesh`类中，让我们添加一个静态函数：

```cpp
static std::shared_ptr<Mesh> Load(const std::string& path);
```

在内部，我们做的第一件事是阅读JSON描述：

```cpp
auto contents = Engine::getInstance()
                   .GetFileSystem()
                   .LoadAssetFileText(path);

if (contents.empty()) return nullptr;
```

如果没有内容 —— 好吧，没有什么可加载的。

---

## 访问和助手函数

cgltf 通过*访问器(accessors)*公开顶点/索引数据。它们基本上描述了“如何解释”二进制缓冲区的一部分（类型、步幅、偏移量等）。

让我们写两个助手lambda：

```cpp
auto readFloats = [](const cgltf_accessor* acc, cgltf_size i, float* out, int n) 
{
    std::fill(out, out + n, 0.0f); // zero-init
    return cgltf_accessor_read_float(acc, i, out, n) == 1;
};

auto readIndex = [](const cgltf_accessor* acc, cgltf_size i) 
{
    cgltf_uint out = 0;
    cgltf_bool ok = cgltf_accessor_read_uint(acc, i, &out, 1);
    return ok ? static_cast<uint32_t>(out) : 0;
};
```

现在我们可以提取浮点数（位置、UVs等）和无符号整数（索引）。

---

## 使用 cgltf 进行解析

现在我们让 cgltf 来承担重任。

```cpp
cgltf_options options = {};
cgltf_data* data = nullptr;

cgltf_result res = cgltf_parse(&options, contents.data(), contents.size(), &data);
if (res != cgltf_result_success) return nullptr;

auto fullPath = Engine::GetInstance().GetFileSystem().GetAssetsFolder() / path;

// Now load the .bin buffer(s)
res = cgltf_load_buffers(&options, data, fullPath.remove_filename().string().c_str());
if (res != cgltf_result_success) {
    cgltf_free(data);
    return nullptr;
}
```

如果这两个步骤都成功，“data”现在将包含所有内容：网格、访问器(accessors)和缓冲区视图。

---

## 迭代网格和图元

GLTF 可以包含多个网格，每个网格可以包含多个子图元。基本体基本上是一块几何体：一组属性（位置、法线、UVs）加上可选索引。

我们只需抓取**第一个使用三角形**的基本体 —— 现在就足够了。

```cpp
std::shared_ptr<Mesh> result = nullptr;

for (cgltf_size mi = 0; mi < data->meshes_count && !result; ++mi) 
{
    const cgltf_mesh& mesh = data->meshes[mi];
    for (cgltf_size pi = 0; pi < mesh.primitives_count && !result; ++pi) 
    {
        const cgltf_primitive& prim = mesh.primitives[pi];
        if (prim.type != cgltf_primitive_type_triangles) continue;

        // process this primitive
    }
}
```

---

## 顶点布局

记住，在我们的引擎中，我们有一个“VertexLayout”，它告诉 GPU 如何解释顶点数据。让我们修复一些属性索引以避免混乱：

```cpp
struct VertexElement {
    static constexpr int PositionIndex = 0;
    static constexpr int ColorIndex    = 1;
    static constexpr int UVIndex       = 2;
};
```

所以：

* Positions → 始终处于 index 0
* Colors → index 1
* UVs → index 2

这将有助于保持顶点布局与着色器的一致性。

---

```cpp
VertexLayout vertexLayout;
const cgltf_accessor* accessors[3] = { nullptr, nullptr, nullptr };
```

---

## 读取属性

GLTF 基元具有一系列属性。对于每一个基元，我们检查其类型：

* 如果它是 **POSITION**，将其映射到 index 0。
* 如果它是 **COLOR\_0**，请将其映射到 index 1。
* 如果它是 **TEXCOORD\_0**，请将其映射到 index 2。

```cpp
for (cgltf_size ai = 0; ai < prim.attributes_count; ++ai) 
{
    const cgltf_attribute& attr = prim.attributes[ai];
    const cgltf_accessor*  acc  = attr.data;
    if (!acc) continue;

    VertexElement element{};
    element.type = GL_FLOAT;

    switch (attr.type) 
    {
        case cgltf_attribute_type_position:
            accessors[VertexElement::PositionIndex] = acc;
            element.index  = VertexElement::PositionIndex;
            element.size = 3;
            break;
        case cgltf_attribute_type_color:
            if (attr.index != 0) continue;
            accessors[VertexElement::ColorIndex] = acc;
            element.index  = VertexElement::ColorIndex;
            element.size = 3;
            break;
        case cgltf_attribute_type_texcoord:
            if (attr.index != 0) continue;
            accessors[VertexElement::UVIndex] = acc;
            element.index  = VertexElement::UVIndex;
            element.size = 2;
            break;
        default:
            continue;
    }

    //插入顶点元素
}
```

对于每个属性，我们填写一个“顶点元素”并将其推入`VertexLayout`。别忘了更新步幅：

```cpp
if (element.size > 0)
{
    element.offset = vertexLayout.stride;
    vertexLayout.stride += element.size * sizeof(float);
    vertexLayout.elements.push_back(element);
}
```

---

## 填充顶点数据

一旦我们有了布局，我们终于可以分配缓冲区了。

1. 从位置访问器中获取顶点数。如果没有位置，就没什么可画的了。

```cpp
if (!accessors[VertexElement::PositionIndex]) 
{
    continue;
}
auto vertexCount = accessors[VertexElement::PositionIndex]->count;
```

2. 调整顶点缓冲区大小：

```cpp
vertices.resize((vertexLayout.stride / sizeof(float)) * vertexCount);
```

```cpp
for (cgltf_size vi = 0; vi < vtxCount; ++vi) 
{
    //顶点处理
}
```

3. 对于每个顶点，遍历布局中的所有元素，然后：
   
    ```cpp
    for (auto& el : vertexLayout.elements) 
    {
    }
    ```

    * 检查此元素是否存在访问器。
        ```cpp
        if (!accessors[el.index]) continue;
        ```

    * 如果是，则将`readFloats`调用到缓冲区中的正确位置。
        ```cpp
        auto index = (vi * vertexLayout.stride + el.offset) / sizeof(float);
        float* outData = &vertices[index];
        readFloats(accessors[index], i, outData, el.size);
        ```

---

## Indices

如果图元有 accessor：

```cpp
if (prim.indices)
{
    auto indexCount = prim.indices->count;
    std::vector<uint32_t> indices(indexCount);
    for (cgltf_size i = 0; i < indexCount; ++i) 
    {
        indices[i] = readIndex(prim.indices, i);
    }
    result = std::make_shared<Mesh>(vertexLayout, vertices, indices);
}

```

如果没有：

```cpp
else
{
    result = std::make_shared<Mesh>(vertexLayout, vertices);
}
```

如果已经创建了网格，则跳出（记住现在我们只针对一个网格）：

```cpp
if (result)
{
    break;
}
```

也跳出更高层的循环：

```cpp
if (result)
{
    break;
}
```

完成后，释放 cgltf 数据：

```cpp
cgltf_free(data);
```

并返回结果：

```cpp
return result;
```

---

## Suzanne 的材质

让我们为新网格创建一种材质。

* 复制`brick.mat` → `suzanne.mat`。
* 在`suzanne.mat`中，将纹理路径更改为：
    ```json
    "path": "models/Suzanne_BaseColor.png"
    ```

* 将**两种材质**（`brick.mat` and `suzanne.mat`）中的纹理 uniform 从`brickTexture`重命名为`baseColorTexture`。
* 更新片段着色器（`fragment.glsl`）以使用`baseColorTexture`。
* 为了获得更清晰的外观，请从顶点着色器和片段着色器中删除顶点颜色（`vColor`）。

---

## 使用网格

现在让我们在代码中尝试一下。转到“Game.cpp”并写：

```cpp
auto suzanneMesh = eng::Mesh::load("models/suzanne.gltf");
auto suzanneMat  = eng::Material::load("materials/suzanne.mat");

auto suzanneObj = m_scene->CreateObject("Suzanne");
suzanneObj->AddComponent(new MeshComponent(suzanneMat, suzanneMesh));
suzanneObj->SetPosition(glm::vec3(0.0f, 0.0f, -5.0f));

// 移动旧立方体，使其不重叠
objectA->SetPosition(glm::vec3(1.0f, 0.0f, -5.0f));
```

运行该项目 —— 如果一切正常，您将看到 Suzanne 的猴头应用了纹理。是的，经典的黄眼睛 Suzanne。

---

## 结论

就是这样：我们的引擎现在可以从 GLTF 文件加载 **网格**。

* 纹理 → 资源
* 着色器 → 资源
* 材质 → 资源
* 网格 → 资源

我们正在逐步建立一个适当的资源管道。Suzanne 还活着，有质感，准备好了。

好极了！旅程仍在继续...