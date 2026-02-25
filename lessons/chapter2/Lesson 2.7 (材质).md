既然我们已经拥有了一个统一且可复用的着色器程序，那么是时候引入一个重要概念了 —— **材质**。

这一概念在现代渲染系统中得到了广泛应用，不仅在游戏开发中，而且在涉及图形的其他领域 —— 如CAD系统、模拟或任何渲染2D或3D场景的软件 —— 也得到了广泛应用。

顾名思义，*材质* 代表物体的表面特征 —— 就像在现实世界中一样。想想由木头、金属、塑料或玻璃制成的物体。这些物体各自具有独特的视觉特性：有些有光泽，有些粗糙，有些高度透明，等等。

在3D图形中，*Material*（材质） 对象的作用与此相同 —— 它描述了渲染对象的独特视觉属性。换言之，它告诉渲染系统对象应该是什么样子。

简而言之，**材质** 是一组设置，它告诉图形API如何绘制特定的对象。而且，正如你可能已经猜到的，材质的核心组件是 **着色器程序** —— 也就是我们刚刚实现的程序。

着色器程序定义了对象的着色方式、光照的应用方式以及如何从几何体生成像素颜色。由于我们正在处理3D渲染，所有视觉输出最终都归结为多边形的处理方式和像素的着色方式。

除了着色器程序，材质还可以存储一组 **uniform 参数** 。这些参数被传递给着色器程序，以定义颜色、光泽度、透明度等属性。

所以，总结如下：

> **材质** 本质上是一个着色器程序，以及一组唯一定义的参数，这些参数决定了对象的渲染方式

---

### 让我们从理论转向实践。

为了更好地理解这是如何工作的，让我们来实现`Material`类。

1. 在`source/engine`目录中，新建一个名为`render`的文件夹。
2. 在`render`文件夹中，创建两个文件：`Material.h`和`Material.cpp`。
3. 将这两个文件添加到引擎的`CMakeLists`中，并在`eng.h`中包含`Material.h`。

现在，让我们在`Material.h`中定义`Material`类。

由于该材料必须存储着色器程序，我们首先从以下内容开始：

```cpp
std::shared_ptr<ShaderProgram> m_ShaderProgram;
```

然后，由于每种材料可能包含特定的参数，我们将为它们创建一个容器。现在，让我们从简单的`float`参数开始：

```cpp
std::unordered_map<std::string, float> m_floatParams;
```

未来，这可以扩展到支持向量、颜色、纹理等。

---

### 实施材料方法

现在，让我们添加几个方法：

#### 1. `SetShaderProgram()`

```cpp
void SetShaderProgram(const std::shared_ptr<ShaderProgram>& shaderProgram) 
{
    m_shaderProgram = shaderProgram;
}
```

#### 2. `SetParam()`

这设置了一个浮点数 Uniform 值：

```cpp
void SetParam(const std::string& name, float value) 
{
    m_floatParams[name] = value;
}
```

#### 3. `Bind()`

此方法绑定着色器并设置所有 Uniform 参数：

```cpp
void Bind() 
{
    if (!m_shaderProgram) 
    {
        return;
    }

    m_shaderProgram->Bind();

    for (const auto& param : m_floatParams) 
    {
        m_ShaderProgram->SetUniform(param.first, param.second);
    }
}
```

---

### 与`GraphicsAPI`集成

接下来，转到`GraphicsAPI.h`并添加一个方法：

```cpp
void BindMaterial(Material* material);
```

在其实施过程中：

```cpp
void GraphicsAPI::BindMaterial(Material* material) 
{
    if (material) 
    {
        material->Bind();
    }
}
```

---

### 在游戏中使用材质

现在，我们来测试一下。

1. 打开`Game.h`文件，并添加一个新字段：

```cpp
eng::Material m_material;
```

2. 在`Game::Init()`函数（位于`Game.cpp`文件中）中，创建着色器程序后，初始化材质：

```cpp
m_material.SetShaderProgram(shaderProgram);
```

现在，你的材质已经准备好用于渲染了。稍后，你可以通过添加纹理支持、多种参数类型（如`vec3`、`mat4`等）来对其进行扩展。
