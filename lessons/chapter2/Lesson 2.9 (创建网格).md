现在，让我们尝试在游戏中创建我们新实现的 **Mesh** 的一个实例。

---

### Step 1: 声明网格指针

打开`Game.h`并声明一个指向`Mesh`的指针：

```cpp
std::unique_ptr<Mesh> m_mesh;
```

---

### Step 2: 在`Game::Init()`中初始化几何体

转到`Game.cpp`文件，在`Init()`方法内部。首先，为简单的 **矩形** 声明顶点数组和索引数组。

我们将复用之前示例中的数据：一个由两个三角形组成的矩形，其顶点属性包括 **位置** 和 **颜色**。

#### 示例:

```cpp
std::vector<float> vertices = 
{
    // positions       // colors
    0.5f, 0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // top-right, red
    -0.5f, 0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // top-left, green
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  // bottom-left, blue
    0.5f,  -0.5f, 0.0f,  1.0f, 1.0f, 0.0f   // bottom-right, yellow
};

std::vector<uint32_t> indices = 
{
    0, 1, 2,  // first triangle
    0, 2, 3   // second triangle
};
```

---

### Step 3: 定义顶点布局

创建一个`VertexLayout`对象，用于描述顶点数据的结构：

```cpp
VertexLayout vertexLayout;

// Position attribute (location = 0)
vertexLayout.elements.push_back({ 
    0,                      // index in shader
    3,                      // size (x, y, z)
    GL_FLOAT,               // type
    0                       // offset
});

// Color attribute (location = 1)
vertexLayout.elements.push_back({ 
    1,                      // index in shader
    3,                      // size (r, g, b)
    GL_FLOAT,               // type
    sizeof(float) * 3       // offset after position
});

vertexLayout.stride = sizeof(float) * 6; // 3 for position + 3 for color
```

---

### Step 4: 创建网格实例

现在我们已经定义了顶点数据、索引数据和布局，接下来就可以创建网格了：

```cpp
m_mesh = std::make_unique<Mesh>(vertexLayout, vertices, indices);
```

至此，网格将使用VBO（顶点缓冲区对象）、EBO（元素缓冲区对象）和VAO（顶点属性对象）进行完全初始化。你可以通过运行程序来测试你的代码——网格应该能够无错误地创建并准备好进行渲染。

---

### Step 5：将`BindMesh()`和`DrawMesh()`添加到`GraphicsAPI`中

为了统一网格操作，让我们在`GraphicsAPI`中添加两个辅助方法：

#### 在 `GraphicsAPI.h`中：

```cpp
void BindMesh(Mesh* mesh);
void DrawMesh(Mesh* mesh);
```

#### 在 `GraphicsAPI.cpp`中:

```cpp
void GraphicsAPI::BindMesh(Mesh* mesh) 
{
    if (mesh) 
    {
        mesh->Bind();
    }
}

void GraphicsAPI::drawMesh(Mesh* mesh) 
{
    if (mesh) 
    {
        mesh->Draw();
    }
}
```

这些方法遵循与`BindShaderProgram()`和`BindMaterial()`相同的模式，为渲染提供了一个干净且一致的接口。

---

### 摘要

现在，您已经拥有了一个完全可运行的渲染管道：

* **ShaderProgram** 告诉OpenGL如何对像素进行着色。
* **Material**（材质）存储着着色器（shader）和任何 uniform 值。
* 一个 **Mesh**（网格）包含要绘制的实际几何体。
* **GraphicsAPI** 管理对这些组件的统一访问。