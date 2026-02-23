# 加载材质

现在让我们学习如何加载 **材质**。

为了加载它们，我们首先需要决定如何将它们表示为资产。最简单、最方便的选择是将材质存储为 **JSON文本文件**。

---

## 添加JSON支持

为了解析JSON，我们将使用一个名为 **nlohmann/JSON** 的外部库。这是一个仅包含头部的库，就像stb\_image一样。将其添加到您的引擎中：

```cmake
include_directories(third_party/json)
```

就是这样，不需要额外的设置。

---

## 创建材质资产

在“assets”文件夹中，创建一个新文件夹“materials”。然后创建我们的第一个测试材料文件，例如“brick.mat”。

既然我们同意使用JSON，结构将如下所示：

```json
{
  "shader": {
    "vertex": "shaders/vertex.glsl",
    "fragment": "shaders/fragment.glsl"
  },
  "params": {
    "float": [
      { "name": "roughness", "value": 0.5 }
    ],
    "float2": [
      { "name": "uvOffset", "value0": 0.0, "value1": 0.0 }
    ],
    "textures": [
      { "name": "brickTexture", "path": "textures/brick.png" }
    ]
  }
}
```

* `shader` 对象包含对顶点和片段着色器文件的引用。
* `params` 对象包含三个子部分：`float`, `float2`, and `textures`。每个存储一个参数对象数组。

  * Floats：`{name，value}`
  * Float2:`｛name，value0，value1｝`
  * Textures：`{ name, path }`

让我们还创建一个文件夹`assets/textures`，并将`brick.png`放在那里。

---

## 材质

就像纹理一样，让我们给“Material”类一个静态加载函数：

```cpp
static std::shared_ptr<Material> Load(const std::string& path);
```

### 实施

1. **读取文件内容**

```cpp
auto contents = Engine::GetInstance()
                   .GetFileSystem()
                   .LoadAssetFileText(path);

if (contents.empty()) return nullptr;
```

2. **解析JSON**

```cpp
nlohmann::json json = nlohmann::json::parse(contents);
std::shared_ptr<Material> result;
```

3. **加载着色器程序**

```cpp
if (json.contains("shader")) 
{
    auto shaderObj = json["shader"];
    std::string vertexPath   = shaderObj.value("vertex", "");
    std::string fragmentPath = shaderObj.value("fragment", "");

    auto& fs = Engine::GetInstance().GetFileSystem();
    auto vertexSrc   = fs.LoadAssetFileText(vertexPath);
    auto fragmentSrc = fs.LoadAssetFileText(fragmentPath);

    auto& graphics = Engine::GetInstance().GetGraphicsAPI();
    auto shaderProgram = graphics.CreateShaderProgram(vertexSrc, fragmentSrc);
    if (!shaderProgram) return nullptr;

    result = std::make_shared<Material>();
    result->SetShaderProgram(shaderProgram);
}
```

4. **加载参数**

```cpp
if (json.contains("params")) 
{
    auto paramsObj = json["params"];

    // Floats
    if (paramsObj.contains("float")) 
    {
        for (auto& p : paramsObj["float"]) 
        {
            std::string name  = p.value("name", "");
            float value = p.value("value", 0.0f);
            result->SetParam(name, value);
        }
    }

    // Float2
    if (paramsObj.contains("float2")) 
    {
        for (auto& p : paramsObj["float2"]) 
        {
            std::string name = p.value("name", "");
            float v0 = p.value("value0", 0.0f);
            float v1 = p.value("value1", 0.0f);
            result->SetParam(name, v0, v1);
        }
    }

    // Textures
    if (paramsObj.contains("textures")) 
    {
        for (auto& p : paramsObj["textures"]) 
        {
            std::string name = p.value("name", "");
            std::string texPath = p.value("path", "");
            auto texture = Texture::Load(texPath);
            if (texture) 
            {
                result->setParam(name, texture);
            }
        }
    }
}
```

5. **返回结果**

```cpp
return result;
```

---

## 使用材质

现在在`Game.cpp`中，我们可以简化代码。删除手动着色器加载和着色器程序创建。相反，只需执行以下操作：

```cpp
auto material = eng::Material::Load("materials/brick.mat");
```

就是这样！运行项目 —— 一切看起来都应该一样，但现在材质完全在外部JSON文件中定义并自动加载。

---

## 结论

我们添加了一种新的资源：**材质**。

* 纹理是资源。
* 着色器是资源。
* 现在材质也是资源。

它们中的每一个都位于`assets/`文件夹中，每个文件夹都通过我们的文件系统加载。

祝贺您刚刚朝着统一引擎中的资源管理迈出了又一大步，并使您作为开发人员的生活变得更加轻松。