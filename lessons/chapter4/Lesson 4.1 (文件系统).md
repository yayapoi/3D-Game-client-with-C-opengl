现在是时候熟悉*资源*的概念了，更广泛地说，是时候熟悉文件加载的工作原理了。如您所知，我们正在构建一个跨平台引擎。这意味着我们的引擎必须在不同的平台上无缝运行。每个平台都有自己独特的文件系统布局和处理文件访问的方式 —— 这是我们需要考虑的。  

因此，让我们做以下事情：我们需要组织一种统一一致的方式来访问文件。为了实现这一点，让我们就一个简单的规则达成一致 —— 所有资源（或者，通常称为*游戏资产*）都将位于一个固定文件夹中。让我们创建这个文件夹，称之为“assets”，并将其放置在项目的根目录中。太棒了

让我们创建一个CMake变量来保存该文件夹的路径：

```
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
```

接下来，我们需要定义要遵循的结构。我建议采用这种方法：在开发和实际游戏过程中，我们应该能够以相同的方式引用文件。因此，我们将支持两种模式：*开发模式*和*运行时模式*。原则如下：首先，我们将检查开发文件夹；如果我们在那里找不到资产，我们将退回到可执行文件旁边的文件夹。

为了实现这一点，我们需要调整一些项目文件。首先，我们将使用C++17标准中引入的`std::filesystem`提供的功能。因此，第一步是更新CMake配置。打开引擎的`CMakeLists.txt`，并在最上面添加两行：

```
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
```

对项目的“CMakeLists.txt”执行相同的操作。这样，我们至少可以执行C++17标准。

下一步是创建配置文件。转到`src/engine`并创建一个名为`config.h.in`的新文件。然后，在引擎的`CMakeLists.txt`中，设置项目后，添加：

```
configure_file(
  ${PROJECT_SOURCE_DIR}/config.h.in
  ${PROJECT_BINARY_DIR}/config.h
)
```

这将生成一个可以保存各种常量的配置标头。例如，在`config.h.in`中，我们可以写：

```cpp
#pragma once
#define ASSETS_ROOT "@ASSETS_DIR@"
```

在这里，`ASSETS_DIR`是我们将定义的CMake变量，它将在构建时被替换到标头中。这为我们提供了一个全局宏，用于存储资产目录的路径（对于开发模式很方便）。

现在我们需要告诉我们的项目使用这个配置文件。在`CMakeLists.txt`的底部，添加：

```
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR})
```

还有一个有用的步骤：让我们确保在构建过程中，将整个`assets`文件夹复制到可执行文件旁边。这样，当我们启动内置游戏时，资源已经在正确的位置。将此添加到CMake中：

```
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${ASSETS_DIR}
    ${CMAKE_BINARY_DIR}/assets
)
```

太好了！现在让我们继续讨论跨平台文件访问机制本身。我们将创建一个名为“FileSystem”的小包装器。在“engine/source”中，创建一个文件夹“io”，并在其中创建两个文件：“FileSystem.h”和“FileSystem.cpp”。将它们添加到引擎的CMake中，并像往常一样包含在内。

在`FileSystem.h `中，声明：

```cpp
namespace eng 
{
class FileSystem 
{
public:
    std::filesystem::path GetExecutableFolder() const;
    std::filesystem::path GetAssetsFolder() const;
};
}
```

在`FileSystem.cpp`中，实现`GetExecutableFolder`。由于这取决于平台，我们将使用特定于操作系统的调用（Windows使用`GetModuleFileName`，macOS使用`_NSGetExecutablePath`，Linux从`/proc/self/exe`读取）。我们删除可执行文件名并返回文件夹路径。

```cpp
#if defined _WIN32
#include <windows.h>
#elif defined (__APPLE__)
#include <mach-o/dyld.h>
#elif defined (__linux__)
#include <unistd.h>
#include <limits.h>
#endif

//...

std::filesystem::path FileSystem::GetExecutableFolder() const
{
#if defined _WIN32
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(NULL, buf, MAX_PATH);
    return std::filesystem::path(buf).remove_filename();
#elif defined(APPLE)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string tmp(size, '\0');
    _NSGetExecutablePath(tmp.data(), &size);
    return std::filesystem::weakly_canonical(std::filesystem::path(tmp)).remove_filename();
#elif defined(linux)
    return std::filesystem::weakly_canonical(std::filesystem::read_symlink("/proc/self/exe")).remove_filename();
#else
    return std::filesystem::current_path();
#endif
}
```

接下来，实现`GetAssetsFolder`。逻辑是：

1. 首先，尝试使用`ASSET_ROOT`宏（在`config.h`中声明）中的开发路径。如果存在，请将其返回。
2. 否则，请返回到`<executable_folder>/assets`，这是CMake在构建过程中复制资源的地方。

这样，在开发过程中，我们直接从源代码树加载资产，但在打包构建中，我们使用捆绑的资产。

```cpp
std::filesystem::path FileSystem::GetAssetsFolder() const
{
#if defined (ASSETS_ROOT)
    auto path = std::filesystem::path(std::string(ASSETS_ROOT));
    if (std::filesystem::exists(path))
    {
        return path;
    }
#endif
    return std::filesystem::weakly_canonical(GetExecutableFolder() / "assets");
}
```

还要向Engine添加一个`FileSystem`成员。转到`Engine.h`并使用getter添加此成员。

现在我们有了一个可用的文件系统，让我们实际尝试加载一些东西。由于我们的下一个主题是*纹理*，我们将从图像开始。我准备了一个轻量级的、仅包含头部的图像加载库，名为 **stb\_image**。您将在`thirdparty/stb_image`下找到它。因为它只是头文件，你不需要任何额外的CMake设置，只需添加：

```
include_directories(thirdparty/stb_image)
```

在引擎和客户端项目中都这样做，这样两者都可以使用它。

我还包含了一个示例纹理`brick.jpg`。将其放入`assets`文件夹中。

现在在`Game.cpp`中，包括 stb\_image：

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
```

然后让我们使用我们的文件系统加载纹理：

```cpp
auto& fs = eng::Engine::GetInstance().GetFileSystem();
auto path = fs.GetAssetsFolder() / "brick.jpg";

int width, height, channels;
unsigned char* data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);

if (data) 
{
    std::cout << "Loaded image!" << std::endl;
    //不要忘记稍后使用stbiimage_free（data）释放图像；
}
```

如果您在控制台中看到`"Loaded image!"`，恭喜您——您的文件系统正常工作，图像加载器也正常工作。

从这里开始，我们准备继续下一个大主题：**纹理**。