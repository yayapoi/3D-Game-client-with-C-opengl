好啦 —— 现在让我们终于来实现这段文本了。我们进入 `TextComponent::Render` 方法中，逐步完成实现过程，保持非常实用的风格，并且贴近我们绘制精灵的方式。

---

## 文本组件：渲染 — 每个字符的四边形

首先，获取字体图集的尺寸：

```cpp
int width  = m_font->GetTexture()->GetWidth();
int height = m_font->GetTexture()->GetHeight();
```

接下来，根据枢轴点的坐标来确定开始绘制的位置：

```cpp
auto pos = GetPivotPos(); // 上一课中我们使用的辅助函数
float xOrigin = pos.x;
float yOrigin = pos.y;
```

现在对字符串中的每个字符进行迭代，并将其绘制为带有纹理的矩形（就像一个精灵图一样）：

```cpp
for (const auto c : m_text) 
{
    const auto& desc = m_font->GetGlyphDescription(c);

    float x1 = static_cast<float>(xOrigin);
    float y1 = static_cast<float>(yOrigin);
    float x2 = static_cast<float>(xOrigin + desc.width);
    float y2 = static_cast<float>(yOrigin + desc.height);

    float u1 = static_cast<float>(desc.x0) / static_cast<float>(width);
    float v1 = static_cast<float>(desc.y0) / static_cast<float>(height);
    float u2 = static_cast<float>(desc.x1) / static_cast<float>(width);
    float v2 = static_cast<float>(desc.y1) / static_cast<float>(height);

    // 将笔尖位置向前移动以适应下一个字符
    xOrigin += desc.advance;
}
```

让我们前往`CanvasComponent`组件，并声明一个空函数（暂时如此）：

```cpp
void DrawRect(
    const glm::vec2& lowerLeftPos, const glm::vec2& upperRightPos,
    const glm::vec2& lowerLeftUV, const glm::vec2& upperRightUV,
    Texture* texture, const glm::vec4& color);
```

回到 `TextComponent::Render()` 函数：

```cpp
// 通过画布绘制字形
// 注意：我们对 v1/v2 进行了交换，因为我们的用户界面 Y 坐标原点不同（左下角 vs 上下角）
canvas->DrawRect(
        glm::vec2(x1, y1), glm::vec2(x2, y2),
        glm::vec2(u1, v2), glm::vec2(u2, v1),
        m_font->GetTexture().get(), m_color);
```

为什么 `v1` 和 `v2` 被交换了？按照惯例，许多用户界面系统将 **左上角** 视为原点，而我们的世界空间则采用 **左下角** 作为原点。交换 V 的值可以解决在采样纹理图时的坐标方向问题。

`TextComponent`渲染就到这里了。

---

## 画布：用户界面着色器以及我们将如何绘制

在执行低级绘制操作之前，让我们在 `GraphicsAPI` 中设置好“默认用户界面着色器”。

添加：

```cpp
std::shared_ptr<ShaderProgram> m_defaultUIShaderProgram;

const std::shared_ptr<ShaderProgram>& GetDefaultUIShaderProgram();
```

### 顶点着色器

我们只需要**位置**、**颜色**和**纹理坐标**，再加上一个单一的**投影**矩阵（正交投影）。无需世界/视图矩阵；用户界面是在屏幕空间中实现的：

```glsl
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;

out vec2 vUV;
out vec4 vColor;

uniform mat4 uProjection;

void main() 
{
    vUV = uv;
    vColor = color;
    gl_Position = uProjection * vec4(position, 0.0, 1.0);
}
```

### 片段着色器

我们可以选择对纹理进行采样，并将其与顶点颜色相乘；或者直接渲染出纯色效果：

```glsl
in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTex;
uniform int uUseTexture;

out vec4 FragColor;

void main() 
{
    vec4 src = (uUseTexture != 0) ? texture(uTex, vUV) * vColor : vColor;
    FragColor = src;
}
```

---

## 策略：动态顶点缓冲区 + 批处理

对于用户界面（UI），我们不会为每个元素单独创建一个网格 —— 这对于许多小型组件来说效率很低。  
相反，我们会在每一帧中将所有 UI 几何图形汇总到一个单一的大型“顶点/索引”缓冲区中，并每帧只上传一次。这是常规做法，通常比管理许多微小的网格表现得更好。

我们还将添加“批量处理”功能：将具有相同着色器状态（特别是相同的“使用纹理？”设置以及相同的纹理）的几何体进行合并。同一批次中的元素可以在一个“绘制调用”中进行绘制。

* 示例：`文本1` 和 `文本2` 使用相同的字体纹理和颜色 → 同一批次。
* 没有纹理（纯色）的按钮 → 单独的批次。
* 另一个使用不同字体纹理的文本 → 另一个批次。

---

## 指令与数据结构

在 `Common.h` 文件中，声明：

```cpp
struct UIBatch 
{
    Texture* texture;
    uint32_t indexCount = 0;
};
```

在`CanvasComponent`中：

```cpp
std::vector<UIBatch> m_batches;
std::vector<float>   m_vertices; // 交错排列：位置（2 个浮点数）+ 颜色（4 个浮点数）+ 法线（2 个浮点数） = 每个顶点 8 个浮点数
std::vector<uint32_t> m_indices;
std::shared_ptr<Mesh> m_mesh;
```

### 绘布初始化

在 `CanvasComponent::Init()` 方法中，设置顶点布局以与着色器相匹配：

```cpp
VertexLayout layout;
layout.elements.push_back({VertexElement::PositionIndex, 2, GL_FLOAT, 0});
layout.elements.push_back({VertexElement::ColorIndex, 4, GL_FLOAT, sizeof(float) * 2});
layout.elements.push_back({VertexElement::UVIndex, 2, GL_FLOAT, sizeof(float) * 6});
layout.stride = sizeof(float) * 8;

m_mesh = std::make_shared<Mesh>(layout, m_vertices, m_indices);
```

---

## 画布：：绘制矩形

附加 4 个顶点 + 6 个索引并进行更新批量处理：

```cpp
uint32_t base = static_cast<uint32_t>(m_vertices.size() / 8);

// p1 为左下角，p2 为右上角
m_vertices.insert(m_vertices.end(), {
    p2.x, p2.y, color.r, color.g, color.b, color.a, uv2.x, uv2.y, // UR
    p1.x, p2.y, color.r, color.g, color.b, color.a, uv1.x, uv2.y, // UL
    p1.x, p1.y, color.r, color.g, color.b, color.a, uv1.x, uv1.y, // LL
    p2.x, p1.y, color.r, color.g, color.b, color.a, uv2.x, uv1.y  // LR
});

m_indices.insert(m_indices.end(), { base, base+1, base+2, base, base+2, base+3 });

UpdateBatches(texture)
```

```cpp
// 批处理：若当前批次为空或纹理内容有变化，则开始新建一个批次；否则则扩展最后一个批次
void CanvasComponent::UpdateBatches(Texture* texture)
{
    if (m_batches.empty() || m_batches.back().texture != texture)
    {
        m_batches.push_back({ texture, 6 });
    }
    else
    {
        m_batches.back().indexCount += 6;
    }
}
```

---

## 开始与清空

在 `CanvasComponent::Update()` 方法的开始处：

```cpp
void CanvasComponent::BeginRendering() 
{
    m_vertices.clear();
    m_indices.clear();
    m_batches.clear();
}
```

前往`GraphicsAPI`，并声明一个结构体：

```cpp
struct Rect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};
```

同时定义一个成员函数以及获取和设置方法：

```cpp
class GraphicsAPI
{
    //...
   void SetViewport(int x, int y, int width, int height)
   {
       glViewport(x, y, width, height);
       m_viewport.x = x;
       m_viewport.y = y;
       m_viewport.width = width;
       m_viewport.height = height;
   }

   const Rect& GraphicsAPI::GetViewport() const
   {
       return m_viewport;
   }

private:
    Rect m_viewport;
    //...
};
```

然后在 `Engine::Init()` 函数中调用 `m_graphicsAPI.SetViewport(0, 0, width, height);`

此外，还需添加一个回调函数，用于监听屏幕尺寸的变化：

```cpp
void windowSizeCallback(GLFWwindow* window, int width, int height)
{
    eng::Engine::GetInstance().GetGraphicsAPI().SetViewport(0, 0, width, height);
}

glfwSetWindowSizeCallback(m_window, windowSizeCallback);
```

在递归的用户界面遍历完成后（此过程会多次调用 `DrawRect` 函数），请执行以下操作：

```cpp
void CanvasComponent::Flush()
{
    m_mesh->UpdateDynamic(m_vertices, m_indices);
    auto& gfx = Engine::GetInstance()->GetGraphicsAPI();
    const auto& viewport = gfx.GetViewport();
    RenderCommandUI cmd;
    cmd.mesh = m_mesh.get();
    cmd.shaderProgram = gfx.GetDefaultUIShaderProgram().get();
    cmd.batches = m_batches;
    cmd.screenWidth = viewport.width;
    cmd.screenHeight = viewport.height;
    Engine::GetInstance()->GetRenderQueue().submit(cmd);
}
```

---

## 网格更新（动态）

添加到`Mesh`中：

```cpp
void Mesh::UpdateDynamic(const std::vector<float>& vertices)
{
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_vertexCout = (vertices.size() * sizeof(float)) / m_vertexLayout.stride;
}

void Mesh::UpdateDynamic(const std::vector<float>& vertices, const std::vector<uint32_t>& indices)
{
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_vertexCout = (vertices.size() * sizeof(float)) / m_vertexLayout.stride;

    if (m_EBO == 0)
    {
        Engine::GetInstance().GetGraphicsAPI().CreateIndexBuffer(indices);
    }
    else
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    m_indexCount = indices.size();
}
```

以及一个“部分绘制”辅助工具：

```cpp
void Mesh::DrawIndexedRange(uint32_t startIndex, uint32_t indexCount) 
{
    if (indexCount == 0) return;
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT,
                   reinterpret_cast<void*>(static_cast<size_t>(startIndex) * sizeof(uint32_t)));
}
```

---

## 渲染队列 — 用户界面层

在`RenderQueue`中添加一个命令：

```cpp
struct RenderCommandUI 
{
    Mesh* mesh;
    ShaderProgram* shaderProgram;
    size_t screenWidth = 0;
    size_t screenHeight = 0;
    std::vector<UIBatch> batches;
};
```

“提交并保存”

```cpp
std::vector<RenderCommandUI> m_commandsUI;

void RenderQueue::submit(const RenderCommandUI& command) 
{
    m_commandsUI.push_back(command);
}
```

在 `RenderQueue::Draw()` 函数中添加“UI 通道”（在 3D 通道之后）：

```cpp
graphicsAPI.SetDepthTestEnabled(false);
graphicsAPI.SetBlendMode(BlendMode::Alpha);

for (auto& cmd : m_commandsUI) 
{
    glm::mat4 ortho = glm::ortho(
        0.0f, static_cast<float>(cmd.screenWidth),
        0.0f, static_cast<float>(cmd.screenHeight)
    );
    cmd.shaderProgram->Bind();
    cmd.shaderProgram->SetUniform("uProjection", ortho);

    cmd.mesh->Bind();

    uint32_t indexBase = 0;
    for (auto& batch : cmd.batches) 
    {
        if (batch.texture) 
        {
            cmd.shaderProgram->SetUniform("uUseTexture", 1);
            cmd.shaderProgram->SetTexture("uTex", batch.texture);
        } 
        else 
        {
            cmd.shaderProgram->SetUniform("uUseTexture", 0);
        }

        cmd.mesh->DrawIndexedRange(indexBase, batch.indexCount);
        indexBase += batch.indexCount;
    }

    cmd.mesh->Unbind();
}

graphicsAPI.SetBlendMode(BlendMode::Disabled);
graphicsAPI.SetDepthTestEnabled(true);
m_commandsUI.clear();
```

--- 

然后来到`ShaderProgram`类中，并添加了一个接受整数的`SetUniform`方法：

```cpp
void ShaderProgram::SetUniform(const std::string& name, int value)
{
    auto location = GetUniformLocation(name);
    glUniform1i(location, value);
}
```

---

## 快速测试

在“Game.cpp”文件中：

```cpp
// 创建一个画布根对象
auto canvas = m_scene->CreateObject("Canvas");
auto canvasComp = new eng::CanvasComponent();
canvas->AddComponent(canvasComp);

// 创建一个文本对象
auto text = m_scene->CreateObject("Text", canvas); // 如果您希望将其置于画布之下，请将其作为画布的子对象
text->SetPosition2D(glm::vec2(300.0f, 300.0f));

auto textComp = new eng::TextComponent();
text->AddComponent(textComp);

textComp->SetText("Some Text");
textComp->SetFont("fonts/arial.ttf", 24);
textComp->SetColor(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
```

运行它。

您应该会在屏幕上看到“一些文本” —— 这无疑就是“FreeType”正在正常运行的明证：它会按照指定尺寸加载字体，将字形转换为纹理图集，获取准确的尺寸数据，将四边形与正确的 UV 坐标对齐，并在我们的用户界面流程中渲染结果。

**恭喜！** 对于引擎而言，这是重大的进步一步。我们现在在原有的 2D/3D 基础之上又新增了 **文本渲染** 和一个强大的 **用户界面批处理** 路径。