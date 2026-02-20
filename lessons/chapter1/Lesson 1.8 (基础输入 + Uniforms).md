太棒了 现在，是时候移动我们绘制的形状了。让我们先来熟悉一下基本设置和控制功能。我们从键盘开始。

键盘输入是通过事件来处理的。这是什么意思呢？当你按下某个键时，它会向操作系统发送一个信号，从而触发一个你可以监听的事件。GLFW提供了一种统一的方式来处理这些事件。

要在GLFW中处理按键事件，你需要使用 `glfwSetKeyCallback` 来设置一个按键回调函数。该函数接受两个参数：`window` 和 `keyCallback` 函数指针。

让我们声明 `keyCallback` 函数：

```cpp
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
```

参数说明：

* `window`: 事件来源的窗口；
* `key`: 被按下的键的代码（例如，空格键、箭头键、Esc）；
* `scancode`: 键的物理扫描码；
* `action`: 动作类型（按下、释放等）；
* `mods`: 任何修饰键（Shift、Ctrl等）。

我们检查是否有按键被按下：

```cpp
if (action == GLFW_PRESS) 
{
    switch (key) 
    {
        case GLFW_KEY_UP:
            std::cout << "GLFW_KEY_UP" << std::endl;
            break;
        case GLFW_KEY_DOWN:
            std::cout << "GLFW_KEY_DOWN" << std::endl;
            break;
        case GLFW_KEY_LEFT:
            std::cout << "GLFW_KEY_LEFT" << std::endl;
            break;
        case GLFW_KEY_RIGHT:
            std::cout << "GLFW_KEY_RIGHT" << std::endl;
            break;
        default:
            break;
    }
}
```

现在，让我们来尝试一些更有趣的操作——让我们的图形动起来。我们将通过顶点着色器为顶点坐标添加偏移量。

首先，定义一个结构：

```cpp
struct Vec2 
{
    float x = 0.0f;
    float y = 0.0f;
};
```

然后在顶点着色器中声明一个uniform变量：

```glsl
uniform vec2 uOffset;
```

在顶点着色器中应用偏移量：

```glsl
gl_Position = vec4(position.x + uOffset.x, position.y + uOffset.y, position.z, 1.0);
```

在代码中获取统一位置：

```cpp
GLint uOffsetLoc = glGetUniformLocation(shaderProgram, "uOffset");
```

创建变量：

```cpp
Vec2 offset;
```

将其发送到着色器程序：

```cpp
glUniform2f(uOffsetLoc, offset.x, offset.y);
```

最后，当按下箭头键时，更新偏移量：

```cpp
case GLFW_KEY_UP:
    offset.y += 0.01f;
    break;
case GLFW_KEY_DOWN:
    offset.y -= 0.01f;
    break;
case GLFW_KEY_LEFT:
    offset.x -= 0.01f;
    break;
case GLFW_KEY_RIGHT:
    offset.x += 0.01f;
    break;
```

现在，当你按下箭头键时，屏幕上的形状会随之移动。干得好！我们离创建第一个游戏的目标越来越近了。
