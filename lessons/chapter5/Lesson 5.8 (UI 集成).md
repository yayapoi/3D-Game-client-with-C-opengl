## 构建功能UI菜单（播放和退出按钮）

好了，现在让我们添加**加载和管理新创建的UI元素**的功能，以及我们在前面学习的加载功能。

换句话说，现在是制作**主菜单**的时候了，可以使用**开始**和**退出**按钮。

---

### 步骤1 - 设置场景

转到文件夹：

```
/assets/scenes/
```

并打开文件：

```
scene.sc
```

在内部，让我们添加**三个对象**：

1. 根**Canvas**对象 - 将其命名为`MainCanvas`。
2. 在其内部，添加两个子对象：

    * `PlayButton`
    * `QuitButton`

它们每个都应该同时具有**ButtonComponent**和附加了**TextComponent**的子对象。 
这将为我们提供一个简单的主菜单UI的结构。

---

### 步骤2 - 加载活动画布

现在打开`Scene:：Load（）`，确保加载后可以告诉**哪个画布处于活动状态**。

在场景加载例程的末尾，添加以下内容：

```cpp
std::string activeCanvasName = json.value("activeCanvas", "");

for (auto& child : result->m_objects)
{
    if (auto canvasObject = child->FindChildByName(activeCanvasName))
    {
        if (auto component = canvasObject->GetComponent<CanvasComponent>())
        {
            Engine::GetInstance().GetUIInputSystem().SetCanvas(component);
            break;
        }
    }
}
```

然后，在场景JSON文件中，指定：

```json
"activeCanvas": "MainCanvas"
```

这样，引擎就知道**哪个画布**应该处于活动状态并响应用户输入。

---

### 步骤3 - 画布激活标志

让我们让`CanvasComponent`知道它是否处于活动状态。

在`CanvasComponent.h`中，添加：

```cpp
bool m_active = true;
```

以及以下方法：

```cpp
void SetActive(bool active) { m_active = active; }
bool IsActive() const { return m_active; }
```

现在，重写`LoadProperties()`：

```cpp
void CanvasComponent::LoadProperties(const nlohmann::json& jsonObject)
{
    bool active = jsonObject.value("active", true);
    SetActive(active);
}
```

---

### 步骤4 - 关于画布激活时输入

在`UIInputSystem:：Update（）`中，在开头添加一个额外的条件：

```cpp
if (!m_active || !m_activeCanvasComponent || !m_activeCanvasComponent->IsActive())
{
    return;
}
```

这确保了我们仅在画布本身处于活动状态时处理输入。

---

### 步骤5 - 将3D对象分组到根下

让我们把场景组织一下。  
将所有3D对象分组到名为 **`3DRoot`** 的单个父对象下。  
这将使一次启用或禁用整个3D世界变得容易 —— 例如，在游戏和主菜单之间切换时。

---

### 步骤6 - 按名称查找对象

在`Scene.cpp`中，添加助手函数：

```cpp
GameObject* Scene::FindObjectByName(const std::string& name)
{
    for (auto& obj : m_objects)
    {
        if (auto child = obj->FindChildByName(name))
        {
            return child;
        }
    }
    return nullptr;
}
```

---

### 步骤7 - 扩展Escape键的输入管理器

Escape键具有更高的代码值，因此我们需要扩展输入数组。

在`InputManager`中，更改：

```cpp
std::array<bool, 256> m_keys;
```

至：

```cpp
std::array<bool, 512> m_keys;
```

现在，我们可以安全地处理`GLFW_KEY_ESCAPE`之类的密钥。

---

### 步骤8 - 改进字形渲染

让我们提高字体渲染的准确性。  
在`Font.h`中，更新`GlyphDescription`结构：

```cpp
int xOffset = 0;
int yOffset = 0;
```

在`FontaManager:：GetFont（）`中，将这些偏移添加到字形描述：

```cpp
gd.xOffset  = static_cast<int>(face->glyph->bitmap_left);
gd.yOffset = static_cast<int>(face->glyph->bitmap_top);
```

然后在`TextComponent:：Render（）`中，修改计算Y坐标的方式：

```cpp
float x1 = static_cast<float>(xOrigin);
float y1 = static_cast<float>(yOrigin - desc.height + desc.yOffset);
float x2 = x1 + static_cast<float>(desc.width);
float y2 = y1 + static_cast<float>(desc.height);
```

此调整修复图示符之间的垂直对齐差异。

---

### 步骤9 - 着色器缓存优化

为了避免重复重新创建相同的着色器，让我们添加一个简单的**着色器缓存**。

在`GraphicsAPI`中，定义：

```cpp
struct ShaderKey 
{
    std::string vertexSource;
    std::string fragmentSource;

    bool operator==(const ShaderKey& other) const 
    {
        return vertexSource == other.vertexSource &&
               fragmentSource == other.fragmentSource;
    }
};

struct ShaderKeyHash 
{
    std::size_t operator()(const ShaderKey& key) const 
    {
        std::size_t h1 = std::hash<std::string>{}(key.vertexSource);
        std::size_t h2 = std::hash<std::string>{}(key.fragmentSource);
        return h1 ^ (h2 << 1);
    }
};
```

现在，在`GraphicsAPI`类中：

```cpp
std::unordered_map<ShaderKey, std::shared_ptr<ShaderProgram>, ShaderKeyHash> m_shaderCache;
```

在`CreateShaderProgram（）`中：

```cpp
ShaderKey key{ vertexSource, fragmentSource };
auto it = m_shaderCache.find(key);
if (it != m_shaderCache.end())
{
    return it->second;
}

auto shaderProgram = std::make_shared<ShaderProgram>(vertexSource, fragmentSource);
m_shaderCache.emplace(key, shaderProgram);
return shaderProgram;
```

就是这样 - 现在相同的着色器将重用现有实例。

---

### 步骤10 - 更新引擎场景所有权

在`Engine`中，替换：

```cpp
std::unique_ptr<Scene> m_currentScene;
```

为：

```cpp
std::shared_ptr<Scene> m_currentScene;
```

这将使我们能够在未来动态切换场景，而不会发生所有权冲突。同时更新`Get（）`和`Set（）`方法：

```cpp
void Engine::SetScene(const std::shared_ptr<Scene>& scene)
{
    m_currentScene = scene;
}

const std::shared_ptr<Scene>& Engine::GetScene()
{
    return m_currentScene;
}
```

---

### 步骤11 - 适当清理

在`Engine:：Run（）`中，在主循环结束后，添加：

```cpp
m_application.reset(nullptr);
```

这确保游戏在退出时释放所有内存和资源。

---

### 步骤12 - 在Game.cpp中设置

让我们声明对根3D对象的引用：

```cpp
GameObject* m_3DRoot = nullptr;
```

然后，在加载场景后：

```cpp
m_3DRoot = m_scene->FindObjectByName("3DRoot");
if (m_3DRoot)
{
    m_3DRoot->SetActive(false);
}
```

现在，让我们连接**Play**和**退出**按钮：

```cpp
if (auto button = canvasComponent->GetOwner()->FindChildByName("PlayButton"))
{
    if (auto component = button->GetComponent<eng::ButtonComponent>())
    {
        component->onClick = [this]()
        {
            auto& engine = eng::Engine::GetInstance();
            engine.GetUIInputSystem().GetCanvas()->SetActive(false);
            engine.SetCursorEnabled(false);

            if (m_3DRoot)
            {
                m_3DRoot->SetActive(true);
            }
        };
    }
}
```

对于**退出**按钮：

```cpp
if (auto button = canvasComponent->GetOwner()->FindChildByName("QuitButton"))
{
    if (auto component = button->GetComponent<eng::ButtonComponent>())
    {
        component->onClick = [this]()
        {
            SetNeedsToBeClosed(true);
        };
    }
}
```

---

### 🏃 步骤13 - 处理Escape键

在`Game:：Update（）`中，在`Scene:：Update（）`之后，添加：

```cpp
if (eng::Engine::GetInstance().GetInputManager().IsKeyPressed(GLFW_KEY_ESCAPE))
{
    if (m_3DRoot && m_3DRoot->IsActive())
    {
        auto& engine = Engine::GetInstance();
        engine.GetUIInputSystem().GetCanvas()->SetActive(true);
        engine.SetCursorEnabled(true);
        m_3DRoot->SetActive(false);
    }
}
```

这使我们可以通过按Escape在**3D世界**和**UI菜单**之间切换 --  
它在真实游戏中的工作原理。

---

### 步骤14 - 测试

现在，当您运行应用程序时：

* 您将看到具有**Play**和**退出**按钮的UI。
* 单击**Play**隐藏UI并启用3D场景。
* 单击**退出**退出游戏。
* 在游戏中按**Escape**返回UI。

---

### 总结

我们刚刚构建了一个**功能齐全的游戏内菜单系统**：

* 可从JSON场景文件加载
* 具有回调的交互式UI元素
* 正确的输入处理和激活逻辑
* 在UI和游戏世界之间动态切换
* 用于性能的着色器缓存

**恭喜你！**  
您的引擎现在支持完整的**主菜单**工作流 - 就像真实游戏一样。