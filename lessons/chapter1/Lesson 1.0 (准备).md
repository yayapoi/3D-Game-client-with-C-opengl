在本课程中，我们将准备工作环境，连接第三方库，并运行第一个构建。

---

## 1) 文件夹布局

创建以下结构（名称很重要）:

```
<repo-root>/
├─ CMakeLists.txt
├─ source/
│  └─ main.cpp
└─ thirdparty/
   ├─ glfw-3.4/
   │  └─ ... (GLFW sources)
   └─ glad/
      └─ ... (GLad files)
```

* `source/` — 你的应用代码（我们将在下面添加一个最小的`main.cpp`）。
* `thirdparty/glfw-3.4` — GLFW源代码 (3.4).
* `thirdparty/glad` — GLAD源代码.

---

## 2) 先决条件

* **CMake ≥ 3.10**
* 支持C++17的编译器

  * Windows: MSVC (Visual Studio), or clang/MinGW
  * macOS: Apple Clang (Xcode command line tools)
  * Linux: GCC or Clang
* 平台SDK/开发工具:

  * Windows: 安装 “Desktop development with C++” (for MSVC)
  * macOS: `xcode-select --install`
  * Linux: 安装 `build-essential` (Debian/Ubuntu) 或等效软件包

---

## 3) 这个CMakeLists.txt的作用（逐行分析）

```cmake
cmake_minimum_required(VERSION 3.10)
```

设置最低的CMake版本。我们依赖于3.10版本之后才有的特性。

```cmake
project (GameDevelopmentProject)
```

定义项目名称。CMake还会创建如`PROJECT_NAME`这样的变量。

```cmake
set(PROJECT_SOURCE_FILES
  source/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad/src/glad.c
)
```

列出你的应用源代码。从`main.cpp`开始，从小处着手。随着引擎的发展，可以扩展这个列表或切换到`target_sources()`。  
因为我们还用到了`glad.c`，它需要直接编译进我们的程序里，所以也要加上这个。

```cmake
include_directories(source)
add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_FILES})
```

* 将`source/`添加到头文件搜索路径中（以便`#include "..."`能够找到您的头文件）。
* 构建一个名为`GameDevelopmentProject`的可执行目标。

```cmake
# Add GLFW library
add_subdirectory(thirdparty/glfw-3.4 "${CMAKE_CURRENT_BINARY_DIR}/glfw_build")
include_directories(thirdparty/glfw-3.4/include)
```

告诉CMake从源代码配置并构建GLFW，将其构建产物放置在`glfw_build/`中。我们还公开了其`include/`目录，以便我们可以`#include <GLFW/glfw3.h>`。

```cmake
# 添加glad头文件路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad/include
)
```

将GLAD头文件添加到搜索路径中。

```cmake
# Link all thirdparty libraries
target_link_libraries(${PROJECT_NAME} 
    glfw 
)
```

将我们的应用程序与GLFW、GLAD进行链接。这就是我们打开窗口和加载OpenGL入口点所需的所有操作。

---

## 4) 用于验证设置的最小化`main.cpp`

将这段代码放入`source/main.cpp`中。这是任何应用程序的基本模板：

```cpp
int main() 
{
    return 0;
}
```

---

## 5) 我们取得的成就

* 一个简洁、精简的CMake项目，从源代码中提供GLFW和GLAD。
* 一个`main`函数作为应用程序的入口点。

就是这样——你的环境已经准备好了。
