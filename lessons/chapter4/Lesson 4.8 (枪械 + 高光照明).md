# 构建更具互动性的场景（一个FPS视角的原型）

现在，让我们创造一个更有趣、更具互动性的场景，让我们更接近真实的游戏体验，或者至少是一个原型。

由于我个人喜欢射击游戏（尤其是第一人称射击游戏），我将以此作为我们进一步探索的基础。

我们已经知道如何加载完整的glTF文件（模型、对象、整个场景）。  
所以我为你准备了另一个文件：**武器模型**。我从 Sketchfab 下载了它（作者在固定评论中注明；你也可以在资源文件夹中免费找到它）。

---

## 步骤1. 组织资产

在您的“assets”文件夹中：

* 创建一个名为“models”的子文件夹（如果它还不存在）。
* 将*Suzanne*移动到其自己的子文件夹中：

  ```
  assets/models/suzanne/
  ```

* 将武器模型和纹理放入“模型”中的另一个子文件夹中：

  ```
  assets/models/weapon/
  ```

现在在“Game.cpp”中，更新 Suzanne 的路径：

```cpp
models/suzanne/suzanne.gltf
```

---

## 步骤2. 加载武器并将它归于相机子类

让我们像对待 Suzanne 一样装上武器。

然后，诀窍来了：  
我们已经有了一个带有摄像头和玩家控制器的“游戏对象”。  
我们将**将武器作为相机对象的子对象**。

这样，武器将与相机一起移动和旋转，给我们一个非常简单的FPS效果。

```cpp
gun->SetParent(camera);
```

现在设置转换。我通过实验选择了这些值，所以请遵循以下步骤：

```cpp
gun->SetPosition(glm::vec3(0.75f, -0.5f, -0.75f));
gun->SetScale(glm::vec3(-1.0f, 1.0f, 1.0f));
```

👉 沿X轴按“-1”缩放可以反映武器，因此它出现在右手中。

运行游戏：

* 武器正确加载。
* 它随鼠标旋转。
* 它随玩家移动。
  这是有效的，因为它是相机的子对象（它本身由玩家控制器驱动）。

---

## 步骤3.修复纹理伪影（RGB与RGBA）

你会注意到武器看起来很奇怪 —— 彩虹般的颜色，而不是预期的纹理。为什么？

答案在`Texture:：Load`中。  
目前，我们总是调用：

```cpp
glTexImage2D(..., GL_RGB, ..., GL_RGB, ...);
```

这假设**所有纹理都是RGB（3个通道）**。

但武器的一个纹理实际上有**4个通道（RGBA）**。  
我们仍然尝试将其作为RGB上传，这会导致伪影（彩虹效应）。

**解决方案：**  
添加处理通道计数的逻辑：

```cpp
GLint internalFormat = GL_RGB;
GLenum format = GL_RGB;

if (numChannels == 4) 
{
    internalFormat = GL_RGBA;
    format = GL_RGBA;
}

glTexImage2D(..., internalFormat, ..., format, ...);
```

现在，具有alpha通道的纹理得到了正确处理。再次运行，一切都会正确渲染。

---

## 步骤4.避免重复纹理加载（缓存）

另一个问题：  
武器模型被分割成许多小对象，每个对象都有自己的网格和材质。  
但在实践中，它们经常重复使用*相同的纹理*。

现在，在`GameObject::ParseGLTF`中，我们为每种材质调用`Texture::Load`——这意味着同一纹理文件可能会多次加载到GPU内存中。浪费！

### 解决方案：添加一个`TextureManager`

在“Texture.h”中，在“Texture”类之后添加：

```cpp
class TextureManager 
{
public:
    std::shared_ptr<Texture> GetOrLoadTexture(const std::string& path);

private:
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
};
```

实现：

```cpp
std::shared_ptr<Texture> TextureManager::GetOrLoadTexture(const std::string& path) 
{
    auto it = m_textures.find(path);
    if (it != m_textures.end())
        return it->second;

    auto texture = Texture::Load(path);
    m_textures[path] = texture;
    return texture;
}
```

现在将其集成到引擎中：

* 在`Engine`中，添加一个`TextureManager m_textureManager;`现场。
* 添加一个getter：

  ```cpp
  TextureManager& GetTextureManager();
  ```

最后，在`GameObject.cpp`中，在`ParseGLTF`中：  
将直接调用`Texture:：Load（path）`替换为：

```cpp
Engine::GetInstance().GetTextureManager().GetOrLoadTexture(path);
```

这确保了纹理被加载一次、缓存和重用。

---

## 步骤5. 添加镜面反射高光（Phong Lighting）

到目前为止，我们的照明非常基本 —— 只有朗伯漫反射（光线方向和法线的点积）。  
让我们添加 **镜面高光** 以获得更逼真的效果。

### 步骤5.1. 为什么是镜面反射？

仅漫反射模型就会使曲面看起来无光泽。然而，当光线以特定角度照射到真实物体上时，它们通常会有闪亮的反射。

这是Phong反射模型的 **镜面反射分量**。  
这取决于：

* 光的方向。
* 表面法线。
* 观察者方向（相机）。

方法：

* 光线照射到表面 → 它以与入射角相等的角度反射。
* 如果相机对准接近该反射角度，观察者会看到一个明亮的亮点。
* 越近，突出显示越清晰。

### 步骤5.2. 经过摄像头位置

为了计算这个，我们需要着色器中的 **相机位置**。

* 在“Common.h”中，添加到“CameraData”：

  ```cpp
  glm::vec3 position;
  ```

* 在“Engine.cpp”中，在“Update/Run”中：

  ```cpp
  cameraData.position = cameraObject->GetWorldPosition();
  ```

* 在渲染队列中，将其传递给着色器：

  ```cpp
  shaderProgram.SetUniform("uCameraPos", cameraData.position);
  ```

---

### 步骤5.3。更新片段着色器

在默认片段着色器中：

```glsl
uniform vec3 uCameraPos;
```

然后计算镜面反射：

```glsl
// View direction: from fragment to camera
vec3 viewDir = normalize(uCameraPos - vFragPos);

// Reflection direction of the light
vec3 reflectDir = reflect(-lightDir, norm);

// Compute specular term
float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

// Strength controls how intense the highlight is
float specularStrength = 0.5;

// Final specular component
vec3 specular = specularStrength * spec * uLight.color;
```

笔记：

* `dot(viewDir, reflectDir)` → 测量相机和反射的对准。
* `max(..., 0.0)` → 忽略负值（光线不会“穿过”曲面）。
* `pow(..., shininess)` → 突出亮点。光泽度更高 → 更紧密、更有光泽的高光（例如，塑料为128，枯木为8）。

最后，将其添加到结果中：

```glsl
vec3 result = diffuse + specular;
vec4 texColor = texture(baseColorTexture, vUV);

FragColor = texColor * vec4(result, 1.0);
```

---

## 步骤6. 测试结果

运行项目：

* 当你在 Suzanne 周围走动时，你现在会看到闪亮的亮点。
* 它们会根据您的视角而变化。
* 材质看起来不那么平坦，更逼真。

---

## 结论

祝贺！

我们已经拥有：

* 为多个模型组织资产。
* 将武器连接到相机上，模拟第一人称射击视角。
* 修复了纹理格式问题（RGB与RGBA）。
* 添加纹理缓存以节省GPU内存。
* 通过 **漫反射+镜面高光** 增强照明。

一步一步地，我们不仅仅是加载模型 —— 我们正在为一个非常简单的 **FPS引擎原型** 奠定基础。
