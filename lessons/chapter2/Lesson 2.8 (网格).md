既然我们已经拥有了一个可运行的 **材质** 系统，接下来就让我们开始创建实际的 **可渲染实体** —— 换言之，也就是将在屏幕上绘制的 **几何体** 或 **3D对象**。

这就引出了 **网格** 的概念。

---

### 什么是网格？

*Mesh* 是3D几何数据的容器。在我们的例子中，它将保存一个 **顶点集和索引** ，这些顶点集和索引定义了空间中的3D对象。

让我们逐步实现`Mesh`类。

---

### Step 1: 设置

在`render`文件夹中，创建两个文件：

* `Mesh.h`
* `Mesh.cpp`

将它们添加到引擎的`CMakeLists`中，并在`eng.h`中包含`Mesh.h`。

---

### Step 2: 定义网格类

在`Mesh.h`文件中，首先声明`Mesh`类。

由于它存储的是几何图形，我们需要：

```cpp
GLuint m_VBO = 0;  // Vertex Buffer Object
GLuint m_EBO = 0;  // Element Buffer Object (Index Buffer)
GLuint m_VAO = 0;  // Vertex Array Object
```

全部初始化为“0”，以表示它们未初始化。

---

### Step 3: 定义顶点布局

为了向GPU传达顶点结构，我们需要定义一个 **VertexLayout** —— 即顶点数据格式的描述。

在`graphics`文件夹中新建一个名为`VertexLayout.h`的文件。

#### 定义 `VertexElement`:

```cpp
struct VertexElement 
{
    GLuint index;      // 着色器中的属性位置（例如，layout(location = 0)）
    GLuint size;       // 分量的数量（例如，位置为3，颜色为4
    GLuint type;       // 数据类型（例如，GL_FLOAT）
    uint32_t offset;   // 从顶点起始的字节偏移量
};
```

#### 定义 `VertexLayout`:

```cpp
struct VertexLayout 
{
    std::vector<VertexElement> elements;
    uint32_t stride = 0; // 单个顶点以字节为单位的总大小
};
```

这种布局告诉OpenGL如何解释原始顶点数据。

---

### Step 4: 向网格添加布局和元数据

在`Mesh.h`中，包含：

```cpp
VertexLayout m_VertexLayout;
size_t m_vertexCount = 0;
size_t m_indexCount = 0;
```

这些会跟踪当前存储在GPU缓冲区中的几何体。

---

### Step 5: 实现`Bind()`和`Draw()`方法

* `Bind()` 激活顶点属性对象（VAO）：

```cpp
void Bind() 
{
    glBindVertexArray(m_VAO);
}
```

* `Draw()` 渲染网格：

```cpp
void Draw() 
{
    if (m_indexCount > 0) 
    {
        glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
    } 
    else 
    {
        glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    }
}
```

---

### Step 6: 构造函数和限制

* **禁用复制和赋值** 以避免意外复制资源：

```cpp
Mesh(const Mesh&) = delete;
Mesh& operator=(const Mesh&) = delete;
```

* 添加两个构造函数：

  1. **带索引**的网格
  2. **不带索引**的网格

索引版本的签名：

```cpp
Mesh(const VertexLayout& layout,
     const std::vector<float>& vertices,
     const std::vector<uint32_t>& indices);
```

对于非索引项：

```cpp
Mesh(const VertexLayout& layout,
     const std::vector<float>& vertices);
```

---

### Step 7: 实现构造函数逻辑

在构造函数中：

1. 保存布局：

```cpp
m_vertexLayout = layout;
```

2. 获取`GraphicsAPI`实例：

```cpp
auto& graphicsAPI = Engine::GetInstance().GetGraphicsAPI();
```

3. 创建顶点缓冲区和索引缓冲区：

#### 添加到 `GraphicsAPI`:

* `GLuint CreateVertexBuffer(const std::vector<float>& vertices);`
* `GLuint CreateIndexBuffer(const std::vector<uint32_t>& indices);`

使用标准的OpenGL缓冲区创建逻辑来实现这些方法。

4. 回到`Mesh`，进行初始化：

```cpp
m_VBO = graphicsAPI.CreateVertexBuffer(vertices);
m_EBO = graphicsAPI.CreateIndexBuffer(indices); // 仅在使用索引时才需要此操作
```

5. 生成并绑定顶点属性对象（VAO）：

```cpp
glGenVertexArrays(1, &m_VAO);
glBindVertexArray(m_VAO);
```

6. 绑定 VBO:

```cpp
glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
```

7. 使用`VertexLayout`设置顶点属性：

```cpp
for (const auto& element : m_vertexLayout.elements) 
{
    glVertexAttribPointer(element.index, element.size, element.type, GL_FALSE, m_vertexLayout.stride, (void*)(uintptr_t)element.offset);
    glEnableVertexAttribArray(element.index);
}
```

8. 绑定 EBO:

```cpp
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
```

9. 解除所有绑定：

```cpp
glBindVertexArray(0);
glBindBuffer(GL_ARRAY_BUFFER, 0);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
```

---

### Step 8: 统计顶点数和索引

* 顶点数量计算方式为：

```cpp
m_vertexCount = (vertices.size() * sizeof(float)) / m_vertexLayout.stride;
```

* 索引计数:

```cpp
m_indexCount = indices.size();
```

---

### Step 9: 处理未索引网格

在第二个构造函数（无索引）中，跳过索引缓冲区的创建。仅初始化VBO（顶点缓冲区对象）和VAO（顶点数组对象），并跳过对`glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ...)`的调用。