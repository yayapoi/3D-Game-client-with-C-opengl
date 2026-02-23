# 更多关于资源

太好了，让我们继续我们的资源之旅。我们已经知道如何加载和使用纹理。但是什么是纹理？没错 —— 纹理只不过是 **资源**。

因此，让我们与它们的合作更加统一和普遍。与其在“Game.cpp”中手动加载纹理，不如将该职责转移到“Texture”类本身。

---

## 重构纹理类

进入`Texture`类，添加一个只接受文件路径的新静态方法：

```cpp
static std::shared_ptr<Texture> Load(const std::string& path);
```

同时添加一个辅助方法：

```cpp
void Init(int width, int height, int numChannels, unsigned char* data);
```

将构造函数中的所有代码放在那里，并用此方法调用替换构造函数代码

### 实施

* 在新的`Texture::Load(const std::string& path)`方法中：

   1. 声明变量`int width, height, numChannels;`
   2. 构建资产的完整路径：

      ```cpp
      auto fullPath = eng::Engine::GetInstance()
                           .GetFileSystem()
                           .GetAssetsFolder() / path;
      ```

   3. 检查文件是否存在：

      ```cpp
      if (!std::filesystem::exists(fullPath)) 
      {
         return nullptr;
      }
      ```

   4. 创建返回对象：

      ```cpp
      std::shared_ptr<Texture> result;
      ```

   5. 使用**stb\_image**加载：

      ```cpp
      unsigned char* data = stbi_load(
            fullPath.string().c_str(),
            &width, &height, &numChannels, 0
      );
      ```

   6. 如果“数据”有效：

      * 调用`std::make_shared<Texture>(...)`
      * 使用`stbi_image_free(data)`释放内存

   7. 返回结果：

      ```cpp
      return result;
      ```

就是这样！我们已将所有图像加载逻辑移至“Texture:：Create”函数中。

现在在“Game.cpp”中，我们可以删除所有手动加载stb\_image的代码，只需执行以下操作：

```cpp
auto texture = eng::Texture::Load("brick.jpg");
```

请注意，该路径是相对于我们的“assets”文件夹的。干净简单。

---

## 将着色器视为资源

为什么只关注纹理？着色器也是资源。它们只是包含 GPU 执行的代码的文本文件。让我们也把它们当作 first-class 的资源。

1. 在“assets”文件夹中，创建一个新文件夹“shaders”。
2. 在那里添加两个文件：

   * `vertex.glsl` （顶点着色器）
   * `fragment.glsl` （片段着色器）

“.glsl”扩展只是一种约定，因为我们使用的是 OpenGL 及其着色语言 GLSL。但从技术上讲，它们只是文本文件。

将您之前在“Game.cpp”中硬编码的着色器代码复制到这些文件中。

---

## 扩展文件系统

我们已经有了一个文件系统，可以解析与`assets`相关的路径。现在让我们教它 **加载文件**。

添加一个通用函数：

```cpp
std::vector<char> LoadFile(const std::filesystem::path& path);
```

### `LoadFile`的实现

1. 以二进制模式打开文件并查找到末尾：

   ```cpp
   std::ifstream file(path, std::ios::binary | std::ios::ate);
   if (!file.is_open()) 
   {
    return {};
   }
   auto size = file.tellg();
   file.seekg(0);
   ```

2. 分配该大小的缓冲区：

   ```cpp
   std::vector<char> buffer(size);
   ```

3. 将整个文件读入缓冲区。如果读取失败，则返回空值。否则，返回缓冲区。

   ```cpp
   if (!file.read(buffer.data(), size))
   {
    return {};
   }

   return buffer;
   ```

---

### 资产特定助手

现在，让我们让事情变得更容易：

```cpp
std::vector<char> LoadAssetFile(const std::string& relativePath) 
{
    return LoadFile(getAssetsFolder() / relativePath);
}
```

由于着色器代码是纯文本，我们还将添加：

```cpp
std::string LoadAssetFileText(const std::string& relativePath) 
{
    auto buffer = LoadAssetFile(relativePath);
    return std::string(buffer.begin(), buffer.end());
}
```

这样，我们可以直接获取着色器源代码作为字符串。

---

## 使用新的资源系统

现在在`Game.cpp`中，我们可以用以下代码替换旧的着色器加载代码：

```cpp
auto& fs = eng::Engine::GetInstance().GetFileSystem();

auto vertexShaderSource   = fs.LoadAssetFileText("shaders/vertex.glsl");
auto fragmentShaderSource = fs.LoadAssetFileText("shaders/fragment.glsl");
```

就是这样。一切都和以前一样，但现在着色器是适当的资源，由我们的文件系统加载，就像纹理一样。

---

## 结论

我们统一了我们的方法：

* **纹理**是资源。
* **着色器**是资源。

两者都存储在`assets/`中，并且都是通过文件系统加载的。

这是向前迈出的一大步。从现在开始，我们不会用低级文件加载来扰乱游戏逻辑 —— 每种资源类型都知道如何初始化自己，文件系统将处理路径。

恭喜你 —— 你现在离理解真正的游戏引擎如何组织资源又近了一步！