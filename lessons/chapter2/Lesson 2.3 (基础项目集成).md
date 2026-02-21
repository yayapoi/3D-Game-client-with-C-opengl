### 将引擎集成到实际项目中

现在，让我们开始在实际项目中运用我们编写的引擎代码。

---

### Step 1: 为引擎创建一个公共头文件

要开始使用这个引擎，我们首先需要向用户公开相关的头文件。我们将为引擎的公共接口创建一个中心包含文件。

1. 前往`engine/source`文件夹。
2. 创建一个名为`eng.h`的新文件。
3. 在此文件中，包含您想要公开的所有公共头文件。在此阶段，我们将包含两个主要类：

```cpp
#include "Application.h"
#include "Engine.h"
```

---

### Step 2: 更新主项目以使用引擎

打开你的主项目的`CMakeLists.txt`文件，确保它能够识别引擎模块。具体来说，将引擎的`source`目录添加到包含路径中：

```cmake
include_directories(engine/source)
```

这将使我们能够在项目的任何位置包含引擎头文件。

---

### Step 3: 创建自定义应用程序

接下来，我们将通过扩展引擎的`Application`类来定义我们自己的应用程序。

1. 前往你项目的`source`文件夹。
2. 创建两个文件：`Game.h` 和 `Game.cpp`。
3. 将它们添加到`CMakeLists.txt`文件中的项目源代码列表下。

#### `Game.h`

```cpp
#include <eng.h>

class Game : public eng::Application 
{
public:
    bool Init() override;
    void Update(float deltaTime) override;
    void Destroy() override;
};
```

#### `Game.cpp`

```cpp
#include "Game.h"
#include <iostream>

bool Game::Init() 
{
    return true;
}

void Game::Update(float deltaTime) 
{
    std::cout << "Current delta: " << deltaTime << std::endl;
}

void Game::Destroy() 
{
    // Cleanup if needed
}
```

---

### Step 4: 编写入口点（main.cpp）

现在，让我们初始化并运行游戏。

#### `main.cpp`

```cpp
#include "Game.h"
#include <eng.h>

int main() 
{
    Game* game = new Game();                // Create game instance
    eng::Engine engine;                     // Create engine instance

    engine.SetApplication(game);           // Pass game instance to engine

    if (engine.Init())                      // Initialize engine and game
    {                                       
        engine.Run();                      // Start main game loop
    }

    engine.Destroy();                      // Clean up resources
    return 0;
}
```

在此设置中：

* 我们创建了一个`Game`和`Engine`的实例。
* 我们使用 `engine.SetApplication(...)` 将游戏实例传递给引擎。
* 我们初始化引擎（以及游戏）。
* 如果初始化成功，我们将通过`engine.Run()`进入主循环。
* 退出循环后，我们调用`engine.Destroy()`来释放资源。

---

### Step 5: 测试循环

为了验证循环是否正常工作以及更新函数是否被调用，我们在`Game::Update`函数中添加了一个简单的`std::cout`语句，该语句会在每一帧将当前的delta时间打印到控制台。

现在，构建并运行项目。  
**好了！** 一切正常。