现在，让我们在游戏内部使用我们新创建的 **场景** 和 **游戏对象** 系统。

---

### Step 1: 创建测试对象

在你的游戏代码中，创建一个新的类来测试场景功能。添加两个文件：

* `TestObject.h`
* `TestObject.cpp`

此外，请将它们添加到您的`CMakeLists.txt`文件中。

---

### Step 2: 定义`TestObject`类

在`TestObject.h`文件中：

```cpp
#include "eng.h"

class TestObject : public eng::GameObject 
{
public:
    TestObject();

    void Update(float deltaTime) override;

private:
    std::shared_ptr<eng::Mesh> m_mesh;
    eng::Material m_material;
    float m_offsetX = 0.0f;
    float m_offsetY = 0.0f;
};
```

这个类继承自我们引擎中的`GameObject`类。

---

### Step 3: 实现构造函数

在`TestObject.cpp`中，将 **之前位于`Game`中的所有逻辑** （如网格和材质设置）移至`TestObject`的构造函数中。

```cpp
TestObject::TestObject() 
{
    // 设置着色器程序
    // 设置几何体（顶点 + 索引
    // 创建顶点布局
    // 创建网格
    // 设置材质并绑定着色器
}
```

使用`std::make_shared`将网格初始化为一个`shared_ptr`。

---

### Step 4: 实现`Update()`方法

在`TestObject::Update()`函数内部：

1. **首先调用基础更新函数**：

```cpp
eng::GameObject::Update(deltaTime);
```

2. 将之前的逻辑从`Game::Update`移到这里

---

### Step 5: 将场景添加到游戏中

打开`Game.h`文件并添加：

```cpp
eng::Scene m_scene;
```

在`Game.cpp`文件中，`Game::Init()`函数内部：

1. 包含`TestObject.h`：

```cpp
#include "TestObject.h"
```

2. 通过场景创建对象：

```cpp
m_scene.createObject<TestObject>("TestObject");
```

---

### Step 6: 在游戏循环中更新场景

在`Game::Update()`函数中：

```cpp
m_scene.Update(deltaTime);
```

---

### Step 7: 构建并运行

编译并运行游戏。你应该仍然能看到矩形对键盘输入的反应，就像之前一样。

但现在：

✅ 所有逻辑都被封装在一个可重用的`GameObject`中 
✅ 你通过场景系统来管理行为和渲染 
✅ 代码模块化、可扩展，并遵循真实的引擎设计

---

### ✅ 总结

你刚刚完成了一个重要的架构步骤：

* 将对象逻辑移入自定义的派生自`GameObject`的类中 
* 将场景系统集成到游戏中 
* 清晰地将渲染、对象逻辑和输入分离 
* 保留了所有现有功能

这正是像虚幻引擎这样的真实引擎构建其游戏对象和场景管理的方式。干得好！