太棒了 现在，除了每个顶点的位置属性外，我们还可以添加颜色属性。让我们从修改着色器代码开始。

#### 顶点着色器（Vertex Array）

打开顶点着色器并添加一个新的输入属性：

```glsl
layout(location = 1) in vec3 color;
```

这一行告诉我们，在位置索引为1的位置，我们将接收一个`vec3`类型的颜色属性。

然后声明一个输出变量：

```glsl
out vec3 vColor;
```

这意味着我们将把一个名为`vColor`的`vec3`变量从顶点着色器传递给片断着色器。

在`main()`函数内部，添加：

```glsl
vColor = color;
```

#### 片段着色器（Fragment Shader）

现在转到片段着色器，并声明来自顶点着色器的输入变量：

```glsl
in vec3 vColor;
```

然后修改设置最终颜色的那一行：

```glsl
FragColor = vec4(vColor, 1.0);
```

#### 向顶点数组添加颜色数据

接下来，我们需要将颜色属性添加到顶点数组中。打开顶点数组，并为每个顶点添加RGB颜色值。

Example:

```cpp
// Position         // Color
{  0.0f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f }, // Red
{ -0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f }, // Green
{  0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f }  // Blue
```

现在，配置顶点数组对象（VAO）以了解如何读取新的属性。在位置属性设置之后添加以下内容：

```cpp
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
glEnableVertexAttribArray(1);
```

Note:

* 步长现在为`6 * sizeof(float)`，因为每个顶点有3个位置值和3个颜色值。
* 偏移量为`3 * sizeof(float)`，因为颜色是在位置之后。

此外，请确保更新位置属性的步幅值，以匹配这一新的布局。

#### 通过插值计算梯度

现在你可以运行程序，看到一个带有渐变色的三角形。这个渐变效果之所以出现，是因为OpenGL在三角形表面的顶点之间进行了颜色插值。
当我们把数据（如颜色）从 **顶点着色器** 传递到 **片断着色器** 时，OpenGL会自动对其进行 **插值**。

这意味着:

* 如果一个顶点为红色，另一个为绿色，
* OpenGL能够为中间像素平滑地混合颜色。
* 这会在三角形或矩形等表面上产生 **渐变**。

这是图形流水线的一个强大功能——而且我们是免费获得它的！

#### 使用索引绘制矩形

让我们画一个稍微复杂一些的图形——一个矩形。对于矩形，需要定义四个顶点。
之前，我们有一个`std::vector<float>`类型的`vertices`变量，它仅存储三角形的顶点位置和颜色。我们将 **重用这个vector** ，但对其进行修改以表示 **具有四个顶点的矩形**：

```cpp
std::vector<float> vertices = {
    // Positions       // Colors (R, G, B)
     0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f, // Top Right (Red)
    -0.5f,  0.5f, 0.0f,   0.0f, 1.0f, 0.0f, // Top Left (Green)
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f, // Bottom Left (Blue)
     0.5f, -0.5f, 0.0f,   1.0f, 1.0f, 0.0f  // Bottom Right (Yellow)
};
```

我们现在有4个顶点，每个顶点包含3个位置值和3个颜色值。


### 为什么使用索引？

OpenGL将所有内容都渲染为三角形。要渲染一个矩形，我们必须将其定义为 **两个三角形**。如果我们简单地把六个顶点都写进去，就会有两个顶点重复，造成数据冗余，这非常低效 —— 尤其是对于复杂模型而言。

**索引** 通过允许我们 **重用** 顶点数据来解决这个问题。我们不再多次发送相同的顶点，而是通过其在数组中的索引来引用它。

构成两个三角形的示例索引列表：

```cpp
std::vector<unsigned int> indices = {
    0, 1, 2,  // First triangle (Top Right - Top Left - Bottom Left)
    0, 2, 3   // Second triangle (Top Right - Bottom Left - Bottom Right)
};
```

这减少了内存使用，并允许更好的GPU优化（如顶点缓存重用）。

OpenGL要求三角形中的顶点按逆时针方向排序。

---

### 什么是EBO？

**元素缓冲区对象（EBO）** — 也称为 **索引缓冲区对象** — 与 **顶点缓冲区对象（VBO）** 类似，但不同之处在于，它不存储位置或颜色等顶点属性，而是存储 **索引**，这些索引告诉 OpenGL **要绘制哪些顶点以及以何种顺序绘制**。

EBO 是使用不同的缓冲区类型进行绑定的：

```cpp
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
```

这与VBOs（顶点缓冲区对象）不同，VBOs绑定的是：

```cpp
glBindBuffer(GL_ARRAY_BUFFER, vbo);
```

`GL_ELEMENT_ARRAY_BUFFER`绑定告诉OpenGL：*"这些是我在绘制元素时想要使用的索引。"*

---

### 设置EBO

```cpp
GLuint ebo;
glGenBuffers(1, &ebo);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
```

### 激活EBO

我们还需要在活动的顶点数组对象（VAO）中激活它。使用 `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);` 现在，GPU知道如何重用`vertices`数组中的顶点来高效地绘制我们的矩形。

#### 带索引的渲染

现在我们从 `glDrawArrays()` 切换到 `glDrawElements()`：

```cpp
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
```

Parameters:

* `GL_TRIANGLES`: 基元类型.
* `6`: 索引数量（2个三角形 × 每个三角形3个顶点）.
* `GL_UNSIGNED_INT`: 索引的数据类型.
* `0`: EBO中的起始偏移量.

#### 颜色处理 uniform

`uniform`是一个由CPU设置的 **全局GPU变量**，在绘制调用中所有处理的顶点/片段之间共享。
让我们给整个形状上色。

在片段着色器中，声明一个全局变量：

```glsl
uniform vec4 uColor;
```

然后将插值后的颜色乘以 uniform 值：

```glsl
FragColor = vec4(vColor, 1.0) * uColor;
```

在CPU端，要设置 uniform 值：

1. 获取其位置：

```cpp
GLint uColorLoc = glGetUniformLocation(shaderProgram, "uColor");
```

2. 使用着色器程序并设置值：

```cpp
glUseProgram(shaderProgram);
glUniform4f(uColorLoc, 0.0f, 1.0f, 0.0f, 1.0f); // Example: Green
```

运行程序并观察最终结果 —— 一个由插值顶点颜色绘制的矩形，受 uniform 颜色乘数的影响。
