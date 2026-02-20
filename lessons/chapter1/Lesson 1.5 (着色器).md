在我们开始之前，请确保包含必要的标准库头文件：

```cpp
#include <string>
```

我们将使用 `std::string` 来存储着色器的源代码。

---

## Step 1: Vertex Shader(顶点着色器) — 编写和理解代码

首先，让我们定义 **顶点着色器** 的源代码：

```cpp
std::string vertexShaderSource = R"(
  #version 330 core
  layout (location = 0) in vec3 position;

  void main() 
  {
      gl_Position = vec4(position.x, position.y, position.z, 1.0);
  }
)";
```

### 逐行解释：

* `#version 330 core`  
  指示GLSL编译器使用OpenGL 3.3（核心配置文件）。

* `layout (location = 0) in vec3 position;`  
  声明位置0处的顶点输入。这是一个用于顶点位置的3D向量。

* `void main()`  
  着色程序的入口点。

* `gl_Position = vec4(position.x, position.y, position.z, 1.0);`  
  将3D位置转换为4D裁剪空间向量。其中 `w` 分量设置为1.0（齐次坐标的默认值）。

---

## Step 2: 编译顶点着色器（带错误处理）

```cpp
GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
const char* vertexSourceCStr = vertexShaderSource.c_str();
glShaderSource(vertexShader, 1, &vertexSourceCStr, nullptr);
glCompileShader(vertexShader);

// Compilation error check
GLint success;
glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
if (!success) 
{
    char infoLog[512];
    glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
    std::cerr << "ERROR::VERTEX_SHADER_COMPILATION_FAILED\n" << infoLog << std::endl;
}
```

---

## Step 3: Fragment Shader(片段着色器) — 编写和理解代码

现在，让我们编写 **片段着色器**，它定义了每个像素的颜色：

```cpp
std::string fragmentShaderSource = R"(
  #version 330 core
  out vec4 FragColor;

  void main() 
  {
      FragColor = vec4(1.0, 0.0, 0.0, 1.0);
  }
)";
```

### 逐行解释：

* `#version 330 core`  
  同样，我们指定使用 OpenGL 3.3 核心配置文件。

* `out vec4 FragColor;`  
  这行代码声明了一个由4个分量组成的输出颜色：红、绿、蓝、透明度。

* `FragColor = vec4(1.0, 0.0, 0.0, 1.0);`  
  完全不透明的红色。R（红）= 1.0，G（绿）= 0.0，B（蓝）= 0.0，A（透明度）= 1.0。

---

## Step 4: 编译片段着色器（带错误处理）

```cpp
GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
const char* fragmentSourceCStr = fragmentShaderSource.c_str();
glShaderSource(fragmentShader, 1, &fragmentSourceCStr, nullptr);
glCompileShader(fragmentShader);

// Compilation error check
glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
if (!success) 
{
    char infoLog[512];
    glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
    std::cerr << "ERROR::FRAGMENT_SHADER_COMPILATION_FAILED\n" << infoLog << std::endl;
}
```

---

## Step 5: 创建并链接着色器程序

现在，我们将顶点着色器和片断着色器合并为一个 **单一的着色器程序**：

```cpp
GLuint shaderProgram = glCreateProgram();
glAttachShader(shaderProgram, vertexShader);
glAttachShader(shaderProgram, fragmentShader);
glLinkProgram(shaderProgram);

// Link error check
glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
if (!success) 
{
    char infoLog[512];
    glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
    std::cerr << "ERROR::SHADER_PROGRAM_LINKING_FAILED\n" << infoLog << std::endl;
}
```

---

## Step 6: 清理

一旦程序成功链接，我们 **就不再需要单独的着色器对象** 了，因此我们可以删除它们以释放GPU内存：

```cpp
glDeleteShader(vertexShader);
glDeleteShader(fragmentShader);
```

这是一种良好的实践——一旦程序被链接，OpenGL会在内部保留已编译的代码。
