## 纹理
现在是时候弄清楚 *纹理* 到底是什么了。这是你在游戏世界里听到的最常见的术语之一。

那么，什么是纹理？简单地说：它只是一个应用于对象的图像。你可以把它想象成糖果包装纸、贴在某物上的贴纸、穿上的衣服或围绕物品的包装。纹理只是物体表面拉伸的图像 —— 仅此而已。

### UV坐标
要将纹理应用于网格，我们需要纹理坐标，也称为 **UV**。

UV坐标形成一个二维系统，范围从左下角的（0,0）到右上角的（1,1）。网格的每个顶点都有自己的UV坐标。

由于纹理本质上是一个图像，因此一切都始于图像处理。让我们创建一个“Texture”类。

转到“Graphics”文件夹并创建两个文件：“Texture.h”和“Texture.cpp”。将它们添加到“CMakeLists”中，并像往常一样包含它们。

### 纹理类

OpenGL中的纹理存储在GPU上，并具有自己的唯一标识符。首先，声明一个字段：

```cpp
GLuint m_textureID = 0;
```

纹理还具有关键属性：宽度、高度和颜色通道的数量（例如RGB为3）。声明它们为：

```cpp
int m_width = 0;
int m_height = 0; 
int m_numChannels = 0;
```

接下来，创建一个构造函数：

```cpp
Texture(int width, int height, int numChannels, unsigned char* data);
```

还添加一个析构函数和一个简单的getter：

```cpp
GLuint GetID() const;
```

### 实现

在构造函数中，初始化“m_width”、“m_height”和“m_numChannels”。然后，在GPU内存中创建纹理对象：

```cpp
glGenTextures(1, &m_textureID);
glBindTexture(GL_TEXTURE_2D, m_textureID);
```

现在我们需要将原始图像数据上传到GPU。由于我们已经知道如何加载图像，我们将使用提供的“data”指针：

```cpp
glTexImage2D(
    GL_TEXTURE_2D, 0, GL_RGB,
    width, height, 0,
    GL_RGB, GL_UNSIGNED_BYTE, data
);
```

之后，生成 **mipmaps**：

```cpp
glGenerateMipmap(GL_TEXTURE_2D);
```

Mipmaps是相同纹理的逐渐缩小的版本（每次大小减半，降至1×1）。当对象在场景中移动得更近或更远时，它们用于过滤和平滑渲染。

### 环绕和过滤

当我们上传纹理时，我们还需要告诉OpenGL，如果我们的UV坐标超出标准\[0,1]范围，它应该如何表现。这被称为 **纹理环绕(texture wrapping)**。有几种常见模式：

* **GL\_REPEAT** —— 默认值。纹理就像地砖一样，无限平铺。如果你的 UV 是`1.2`，它只会回到`0.2`。非常适合图案（砖、草等）。
* **GL\_MIRORED\_REPEAT** —— 类似于重复，但每个图块都会镜像前面的图块。这避免了铺瓷砖时出现硬接缝。想象一下壁纸条来回翻转。
* **GL\_CLAMP\_TO\_EDGE** —— 边缘像素不是平铺，而是向外拉伸。如果您希望纹理在其边界处停止并且根本不重复（例如UI元素），这很有用。

让我们为纹理设置环绕：

```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
```

这里的“S”和“T”是 UV 空间中的水平轴和垂直轴。

接下来，让我们配置 **过滤**。过滤决定了当绘制的纹理大于或小于其实际分辨率时，如何缩放纹理：

```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
```

* **MIN\_FILTER** 控制纹理被 *缩小*（收缩）时的显示方式。
* **MAG\_FILTER** 控制它们被 *放大* 时的显示方式。

在这里，我们使用线性滤波和mipmaps进行缩小，使用简单的线性滤波进行放大 —— 这会得到平滑的结果。

最后，在析构函数中，释放纹理：

```cpp
if (mTextureId > 0) 
{
    glDeleteTextures(1, &m_textureID);
}
```

太好了 —— 我们现在有了一个“纹理”对象。

---

### 将纹理连接到材质

转到`ShaderProgram`并添加一个方法：

```cpp
void SetTexture(const std::string& name, Texture* texture);
```

然后在`Material`中，添加一个纹理容器：

```cpp
std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
```

实现`SetParam（name，texture）`，以便将纹理存储在该贴图中。在`Bind（）`方法中，循环遍历所有纹理，并通过`SetTexture（）`将它们传递到着色器中。

---

### UV坐标

在`Game.cpp`中更新顶点数据以包含 UVs。现在我们需要分别声明每张面。

```
std::vector<float> vertices =
{
   // Front face
   0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
   -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
   -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
   0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

   // Top face
   0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
   -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
   -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
   0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

   // Right face
   0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
   0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
   0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
   0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

   // Left face
   -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
   -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
   -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
   -0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

   // Bottom face
   0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
   -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
   -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
   0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

   // Back face
   -0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
   0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
   0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
   -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f
};

std::vector<unsigned int> indices =
{
    // front face
    0, 1, 2,
    0, 2, 3,
    // top face
    4, 5, 6,
    4, 6, 7,
    // right face
    8, 9, 10,
    8, 10, 11,
    // left face
    12, 13, 14,
    12, 14, 15,
    // bottom face
    16, 17, 18,
    16, 18, 19,
    // back face
    20, 21, 22,
    20, 22, 23
};
```

相应地更新顶点布局：

* Position → 3 floats
* Color → 3 floats
* UV → 2 floats

每个顶点有8个浮点数（而不是6个）。

---

### 更新着色器

在**顶点着色器**中，添加：

```glsl
layout (location = 2) in vec2 uv;
out vec2 vUV;
...
vUV = uv;
```

在**片段着色器**中，接收它：

```glsl
in vec2 vUV;
uniform sampler2D brickTexture;
```

然后对纹理进行采样（使用vUV坐标获取纹理像素的颜色）：

```glsl
vec4 texColor = texture(brickTexture, vUV);
fragColor = texColor * vec4(vColor, 1.0);
```

这将基础顶点颜色与采样纹理颜色相乘。

---

### 纹理单位

现在让我们来谈谈一些非常重要的东西：**纹理单位**。

GPU 可以同时处理多个纹理，但着色器并不直接知道纹理ID。相反，它们通过*samplers*（我们在片段着色器中使用的`sampler2D`）引用纹理，这些采样器链接到一个称为*纹理单元*的东西。

将纹理单元想象成 GPU 内的编号槽或单元，就像一排储物柜。每个储物柜一次只能容纳一种纹理。当着色器想要读取纹理时，它不会说“给我这个ID的纹理#5”；相反，它说“来自 unit 0 的样本”或“来自 unit 1 的样本”。然后，作为引擎开发人员，你的工作是将正确的纹理绑定到该槽。

它是如何一步一步地工作的：

1. **激活纹理单元**

   ```cpp
   glActiveTexture(GL_TEXTURE0 + unitIndex);
   ```

   这条消息告诉OpenGL：“从现在起，我将使用纹理单元编号`unitIndex`。”

2. **将纹理绑定到活动单元**

   ```cpp
   glBindTexture(GL_TEXTURE_2D, texture->GetID());
   ```

   现在，这个插槽可以容纳你的纹理。

3. **告诉着色器使用哪个单元**

   记住，在着色器中，您声明了一个`uniform sampler2D brickTexture;`.该采样器不保存纹理ID，它只需要一个整数，表示从哪个纹理单元读取。

   ```cpp
   glUniform1i(location, unitIndex);
   ```

所以整个序列看起来像这样：

```cpp
glActiveTexture(GL_TEXTURE0 + unitIndex);
glBindTexture(GL_TEXTURE_2D, texture->GetID());
glUniform1i(location, unitIndex);
```

这里，“unitIndex”是纹理槽的索引，“location”是采样器的统一位置。

就是这样 —— 着色器现在对正确的纹理进行采样。

一个重要的注意事项：由于一种材质中可以使用多个纹理，因此我们需要仔细地为每个纹理分配自己的自由单位。一个简单的方法（以及我们现在将使用的方法）是在“ShaderProgram”中保留一个计数器。每次绑定新纹理时，我们都会增加计数器并指定下一个可用单位。当着色器被绑定时，我们将计数器重置为零。这使得一切都是可预测的。

因此，每次绑定着色器时，创建一个从0开始的计数器“m_currentTextureUnit”。每个新纹理都绑定到下一个可用单元，并且“m_currentTextureUnit”递增。

```cpp
glActiveTexture(GL_TEXTURE0 + m_currentTextureUnit);
glBindTexture(GL_TEXTURE_2D, texture->GetID());
glUniform1i(location, m_currentTextureUnit);
++m_currentTextureUnit;
```

---

### 把它们放在一起

现在，在`Game.cpp`中，您可以执行以下操作：

创建纹理
```cpp
auto texture = std::make_shared<eng::Texture>(width, height, channels, data);
```

将其设置为材质
```cpp
material->SetParam("brickTexture", texture);
```

使用后别忘了清除数据
```cpp
stbi_image_free(data);
```

绑定材质时，着色器获取纹理，网格提供UV，管道正确渲染。

运行该项目，您应该看到您的立方体（或四边形）用 **砖墙纹理** 进行纹理处理，并按顶点颜色着色。

恭喜你 —— 你刚刚实现了纹理！