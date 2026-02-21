太棒了 现在，是时候深入探讨图形处理了。让我们对图形处理进行标准化，并将其封装在一组我们的引擎将使用的通用封装器中。

我们从着色器程序开始。在引擎的`source`目录中，新建一个名为`graphics`的文件夹。在该文件夹中，创建两个文件：`ShaderProgram.h`和`ShaderProgram.cpp`。将这两个文件添加到引擎的`CMakeLists`中，并在`eng.h`中包含它们。

太好了！ 现在打开`ShaderProgram.h`并定义`ShaderProgram`类。

首先，它将包含一个字段：`GLuint m_shaderProgramID`。这是由OpenGL创建的着色器程序的实际ID。

接下来，我们将添加一个`Bind()`方法，该方法将在内部调用OpenGL函数`glUseProgram(m_shaderProgramID)`。

我们还将添加一个`GetUniformLocation()`方法，该方法将使用本地缓存来避免冗余的OpenGL调用。要实现缓存，请使用：

```cpp
std::unordered_map<std::string, GLint> m_uniformLocationCache;
```

在`GetUniformLocation`函数中，我们首先会检查该位置是否已被缓存：

```cpp
auto it = m_uniformLocationCache.find(name);
```

如果未找到，我们将通过以下方式获取位置：

```cpp
GLint location = glGetUniformLocation(m_shaderProgramID, name.c_str());
```

然后我们将其存储在缓存中：

```cpp
m_uniformLocationCache[name] = location;
```

如果找到了，我们只需返回缓存的值。

我们还将实现一个`SetUniform()`方法，该方法接受一个`std::string`和一个`float`值。此方法将检索位置，如果有效，则设置统一值：

```cpp
glUniform1f(location, value);
```

接下来，添加一个显式构造函数，该函数接受一个`GLuint shaderProgramID`参数。将默认构造函数和复制构造函数标记为已删除。在析构函数中，使用`glDeleteProgram`删除着色器程序。

我们故意将构造函数设为已删除，以避免复制着色器对象。这确保着色器程序仅被创建和销毁一次，从而避免潜在的错误或崩溃。

---

现在，我们需要一种方法来 *创建* 着色器程序。我决定将图形组件的资源管理集中化。让我们创建一个名为`GraphicsAPI`的类，它将作为所有核心图形操作的封装器。

在同一个`graphics`文件夹中，新建两个文件：`GraphicsAPI.h`和`GraphicsAPI.cpp`。将这些文件添加到引擎的`CMakeLists`中，并在`eng.h`中包含它们。

如前所述，`GraphicsAPI`将作为渲染操作的集中接口。请相应地定义`GraphicsAPI`类。

就像`InputManager`一样，我们将在`Engine`类中存储`GraphicsAPI`的一个实例。因此，打开`Engine.h`，声明一个`GraphicsAPI`成员，并添加一个`GetGraphicsAPI()`方法，该方法返回对该成员的引用。

回到`GraphicsAPI`中，实现一个创建着色器程序的方法：

```cpp
std::shared_ptr<ShaderProgram> CreateShaderProgram(const std::string& vertexSource, const std::string& fragmentSource);
```

此方法将接收顶点着色器和片断着色器的源代码，对其进行编译，并将它们链接到一个着色器程序中，最后返回一个新的`ShaderProgram`实例。

编译和链接成功后，我们将生成的程序ID封装在`ShaderProgram`对象中，并通过`std::make_shared`返回。

接下来，添加一个方法：

```cpp
void BindShaderProgram(ShaderProgram* shader);
```

此方法仅调用`shader->Bind()`。这使我们能够直接或通过统一的`GraphicsAPI`绑定着色器。

---

现在，我们来测试着色器的创建。

打开`Game.cpp`并转到`Init()`函数。粘贴之前使用的顶点着色器和片段着色器源代码。然后编写：

```cpp
auto& graphicsAPI = Engine::getInstance().getGraphicsAPI();
auto shaderProgram = graphicsAPI.createShaderProgram(vertexShaderSource, fragmentShaderSource);
```

编译并运行应用程序，以确认着色器程序已成功创建且包含有效数据。

完成了！着色器程序运行正常，一切看起来都很棒。  
谢谢！