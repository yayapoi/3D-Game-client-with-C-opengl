# 将完整的glTF场景加载到引擎中

如您所知，我们已经知道如何通过解析glTF文件来加载 **网格**。  
但是，正如你可能理解的那样，我们到目前为止所做的是一个非常基本和部分的解决方案。

一个glTF文件实际上描述了更多的信息，现在我们只提取了一小部分 —— 基本上是一个网格。  
这不太合理。

因此，我建议从现在开始，我们不再只加载网格，而是更深入地开始加载 **它的所有**。

我们将使用 Suzanne 文件作为示例来完成所有这些操作。这样，你就能清楚地看到（或看不到）事情是如何变化的。

---

## 步骤1。将`LoadGLTF`添加到`GameObject`

让我们在“GameObject”类中添加一个新的静态方法，与我们加载网格的方式非常相似：

```cpp
static GameObject* LoadGLTF(const std::string& path);
```

在这个方法中，我们将遵循我们在`Mesh::Load`中使用的相同基本过程。  
如果你还记得，在`Mesh：Load`中，我们包含了如下内容：

```cpp
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
```

我们这里也需要这些，所以让我们把它们搬走。

---

## 步骤2. 读取文件内容

回到`LoadGLTF`，首先我们读取文件内容，就像我们对网格所做的那样：

```cpp
auto contents = Engine::GetInstance().GetFileSystem().LoadAsFileText(path);
```

这为我们提供了 glTF 文件的原始文本。之后，我们用 cgltf 解析它。  
解析成功后，我们可以继续。

我们还需要存储相对文件夹路径（使用`std:：filesystem:：path`），以便稍后解析纹理URIs、缓冲区等。

```cpp
if (res != cgltf_result_success)
{
    return nullptr;
}
auto fullPath = Engine::GetInstance().GetFileSystem().GetAssetsFolder() / path;
auto fullFolderPath = fullPath.remove_filename();
auto relativeFolderPath = std::filesystem::path(path).remove_filename();

res = cgltf_load_buffers(&options, data, fullFolderPath.string().c_str());
//...
```

---

## 步骤3. 关于场景结构的思考

glTF文件将数据存储在 **节点** 中。  
每个节点可能包含：

* 变换（位置、旋转、缩放，甚至是一个完整的矩阵），
* 零个或一个网格，
* 子节点（其他节点）。

那么，让我们达成一致：

* **每个glTF节点将映射到一个`游戏对象`。**

每个glTF网格可以包含多个 **图元**，每个图元可以有：

* 几何形状，
* 材质。

这与我们的引擎完美匹配：

* **一个glTF图元 → 一个`MeshComponent`。**

---

## 步骤4. 创建根对象

因为一个glTF文件可能包含一个包含许多节点的整个场景，所以我们需要一个根“GameObject”来保存所有内容：

```cpp
auto resultObject = Engine::GetInstance().GetScene().CreateObject("Result");
```

现在，一个glTF文件可能包含多个场景。  
为了简单起见，让我们只取**第一个场景**（场景 index 0）。

```cpp
auto scene = data.scenes[0];
```

然后我们迭代它的节点：

```cpp
for (cgltf_size i = 0; i < scene->nodes_count; ++i) 
{
    auto node = scene->nodes[i];
    ParseGLTFNode(node, resultObject, folder);
}
```

---

## 步骤5.实现`ParseGLTFNode`

我们将在`GameObject.cpp`中创建一个辅助函数：

```cpp
void ParseGLTFNode(cgltf_node* node,
                   GameObject* parent,
                   const std::filesystem::path& folder);
```

此功能将：

1. 为节点创建一个“游戏对象”。

    ```cpp
    auto object = parent->GetScene()->CreateObject(node->name, parent);
    ```

2. 应用变换（矩阵或单独的平移/旋转/缩放）。
3. 如果节点有网格 → 加载其所有图元。
4. 如果图元有材质 → 创建或加载它们。
5. 递归解析子节点。

---

### 步骤5.1. 处理转换

节点可以通过两种方式存储转换：

* **作为矩阵**
    在这种情况下，我们使用`glm::make_mat4(node->matrix)`，然后对其进行分解：

    ```cpp
    glm::vec3 translation, scale, skew;
    glm::vec4 perspective;
    glm::quat orientation;
    glm::mat4 mat = glm::make_mat4(node->matrix);
    glm::decompose(mat, scale, orientation, translation, skew, perspective);

    object->SetPosition(translation);
    object->SetRotation(orientation);
    object->SetScale(scale);
    ```

* **作为单独的组件**
    如果节点没有矩阵，则可以分别指定平移、旋转和缩放：

    ```cpp
    if (node->has_translation)
        object->SetPosition(glm::vec3(node->translation[0],
                                        node->translation[1],
                                        node->translation[2]));
    if (node->has_rotation)
        object->SetRotation(glm::quat(node->rotation[3],
                                        node->rotation[0],
                                        node->rotation[1],
                                        node->rotation[2]));
    if (node->has_scale)
        object->SetScale(glm::vec3(node->scale[0],
                                    node->scale[1],
                                    node->scale[2]));
    ```

---

### 步骤5.2. 处理网格和图元

如果节点具有网格：

```cpp
if (node->mesh) 
{
    for (cgltf_size pi = 0; pi < node->mesh->primitives_count; ++pi) 
    {
        auto primitive = node->mesh->primitives[pi];
        //读取顶点属性、索引等。
    }
}
```

* 读取顶点位置、法线、UV。
* 读取索引。
* 创建一个新的“网格”。
* 创建一个“MeshComponent”。
* 将其附着到对象上。

---

### 步骤5.3. 搬运材质

glTF材质 **基于PBR**。存在两个工作流：

* 金属粗糙度
* 镜面光泽度

我们需要一个默认材质，以防图元没有。  
因此，在`GraphicsAPI`中，让我们添加：

```cpp
const std::shared_ptr<ShaderProgram>& GetDefaultShaderProgram();
```

此函数将延迟创建和缓存默认着色器（例如，通过加载 vertex.glsl 和 fragment.glsl ）。

解析图元时：

* 创建一个“材质”：`auto mat = std::make_shared<Material>();`
* 为其指定默认着色器：`mat->SetShaderProgram(Engine::GetInstance().GetGraphicsAPI().GetDefaultShaderProgram());`
* 如果存在`primitive.material`，请检查：

    * `has_pbr_metall_roughness `→ 使用`baseColorTexture`。
        ```cpp
        if (gltfMaterial->has_pbr_metallic_roughness)
        {
            auto pbr = gltfMaterial->pbr_metallic_roughness;
            auto texture = pbr.base_color_texture.texture;
            if (texture && texture->image)
            {
                if (texture->image->uri)
                {
                    auto path = folder / std::string(texture->image->uri);
                    auto texture = Texture::Load(path.string());
                    mat->SetParam("baseColorTexture", texture);
                }
            }
        }
        ```

    * `has_pbr_specular_solutely `→ 使用`diffuseTexture`。
        ```cpp
        else if (gltfMaterial->has_pbr_specular_glossiness)
        {
            auto pbr = gltfMaterial->pbr_specular_glossiness;
            auto texture = pbr.diffuse_texture.texture;
            if (texture && texture->image)
            {
                if (texture->image->uri)
                {
                    auto path = folder / std::string(texture->image->uri);
                    auto texture = Texture::Load(path.string());
                    mat->SetParam("baseColorTexture", texture);
                }
            }
        }
        ```

加载相对于 glTF 文件夹的纹理（如果存在URI）。  
设置材质中的纹理：

```cpp
material->SetParam("baseColorTexture", texture);
```

最后，使用材质和网格创建网格组件。

---

### 步骤5.4. 递归处理子节点

处理当前节点后：

```cpp
for (cgltf_size ci = 0; ci < node->children_count; ++ci) 
{
    ParseGLTFNode(node->children[ci], object, folder);
}
```

这样，我们就可以处理整个层次结构。

---

## 步骤6. 清理

加载完所有内容后，释放 cgltf 数据：

```cpp
cgltf_free(data);
```

返回`resultObject`。

---

## 步骤7. 场景管理改进

为了更好地管理对象，让我们在每个`游戏对象`中存储一个对场景的引用：

```cpp
Scene* m_scene = nullptr;
```

在`Scene::CreateObject`中，设置此指针。  
另外，添加：

* `bool SetParent(GameObject* parent);`
* `Scene* GetScene();`

这为我们提供了干净的环境和父类管理。

---

## 步骤8. 在`Game.cpp`中测试

现在我们可以将 Suzanne 加载为完整场景：

```cpp
auto suzanneObject = GameObject::LoadGLTF("models/suzanne.gltf");
```

删除旧的单网格加载代码。  
同时删除阻挡 Suzanne 的虚拟对象。

最后，返回到“Mesh”类并删除“Load”方法和“include cgltf”。

当你运行它时，Suzanne 看起来和以前一样。  
但差异是巨大的：  
您现在正在加载 **整个glTF场景**，而不仅仅是一个网格。

这给了我们一个更强大、更经得起未来考验的引擎。

---

就是这样 —— 我们现在支持 **完整的glTF场景加载**，包括节点、变换、网格、材质和递归。