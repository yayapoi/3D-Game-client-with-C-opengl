太棒了 现在，我们已经准备好在我们刚刚创建的窗口中使用OpenGL，并开始进行渲染。  
在我们进行下一步之前，我们需要包含 **GLAD** 库。因此，我们添加以下包含指令：

```cpp
#include <glad/glad.h>
```

接下来，我们需要告诉 **GLFW** 我们想要使用哪个版本的OpenGL上下文。你可能还记得，我们决定使用 **OpenGL 3.3版本** ，因此我们需要将这一信息传递给GLFW。

这是通过 `glfwWindowHint` 函数来实现的：

```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Set major version to 3
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // Set minor version to 3
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // OpenGL core profile

```

太好了，现在GLFW已经知道我们打算使用哪个OpenGL上下文了。

为了利用这一上下文，我们必须指定它应与哪个窗口相关联。我们通过以下方式来实现这一点：

```cpp
glfwMakeContextCurrent(window);
```

在这里，`window` 是我们之前创建的窗口。

接下来，我们需要初始化 **GLAD** 库，以便能够使用OpenGL函数：

```cpp
if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
{
    std::cout << "Failed to initialize GLAD" << std::endl;
    glfwTerminate();
    return -1;
}
```

太棒了！现在我们的窗口已经准备好进行渲染了。

我们先从简单的开始——比如更改背景颜色。

### 关于颜色表示的简短说明

在所有图形应用程序编程接口（API）中，任何颜色都表示为 **红**、**绿**和 **蓝** 的组合，有时还包括一个 **透明度** 通道。  
根据格式的不同，这些组件（通道）中的每一个的值都可以在 **0到1** 或 **0到255** 的范围内变化。  

这意味着每个颜色通道可以有256个不同的值（从0到255），这些值可以用 **8位** 来存储  
由于通常有4个通道（RGBA），一种颜色在内存中通常占用 **32位**（或4个字节）。

既然我们已经了解了颜色的表示方式，接下来就为我们的屏幕设置 **背景色** 吧。

我们使用 `glClearColor` 函数，该函数接受4个参数： **红色**、**绿色**、**蓝色** 和 **透明度**。

让我们将屏幕设置为完全不透明的红色：

```cpp
glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
```

第一个参数为红色（1.0），第二个和第三个参数分别为绿色和蓝色（0.0），第四个参数为阿尔法（1.0），表示完全不透明。

现在，让我们使用以下指令来清空屏幕：

```cpp
glClear(GL_COLOR_BUFFER_BIT);
```

现在，让我们尝试运行我们的程序。

而我们看到……什么都没有改变！  
为什么？因为我们正在做的所有事情——所有的渲染——都是在 **后台缓冲区(back buffer)** 中进行的。

### 关于双缓冲的简短说明

双缓冲是图形应用程序编程接口（API）中使用的一种技术，用于防止渲染过程中出现闪烁和视觉瑕疵。  
有两个缓冲区： **前台缓冲区 front buffer**（显示在屏幕上）和 **后台缓冲区 back buffer**（用于渲染）。

所有的绘图都是在 **后台缓冲区** 中完成的，一旦所有内容准备就绪，我们就会 **交换** 缓冲区以显示渲染后的图像。

到目前为止，我们所有的绘图都是在后缓冲区中完成的，但并未将其与前缓冲区进行交换——因此没有任何内容被显示出来。

为了展示结果，我们需要使用以下代码来**交换**缓冲区：

```cpp
glfwSwapBuffers(window);
```

此函数将 `window` 作为参数。

现在，让我们再次运行程序。瞧！你可以看到窗口的颜色已经改变了。

你可以随意尝试不同的背景颜色。例如，你可以将绿色通道设置为1.0：

```cpp
glClearColor(1.0f, 1.0f, 0.0f, 1.0f); // Red + Green = Yellow
```

再运行一次——你会看到背景颜色变了。

而这是你的 **第一个OpenGL渲染命令**！

