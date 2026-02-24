## 什么是字体，以及如何使用它渲染文本

那么，什么 **是** 字体？它是文本渲染的基础，也是我们在引擎中使用文本的方式。

您已经知道字体存在于特殊文件中（例如 **.ttf**、**.otf** 等）。你以前见过他们。事实上，字体文件基本上是一组 **数学函数**：该文件描述 **glyphs**（来自各种字母的单个字符）。每个字形都有一个 **code** （统一），因此从任何字符表中，我们都可以通过相同的代码找到相同的字形，并以一致的样式获得其形状。

字体文件存储一个 **字形表** ：对于每个字形，它存储其形状的数学描述。这意味着我们可以 **任意缩放** 字体 —— 大小由我们决定 —— 而不会损失质量，因为形状是通过分析定义的。

但我们的图形引擎不绘制解析曲线；它绘制 **rasters**（位图）。因此，在使用字体的数学之前，必须 **将其光栅化** 为特定大小。光栅化后，我们可以将字形图像视为常规 **纹理**（例如，在精灵上绘制它们）。这是光栅管道中字体渲染背后的核心概念。

我准备了一种字体，并将其放在`assets/fonts/`文件夹中。我还为光栅化准备了第三方库：**FreeType**。它已经添加到适当的文件夹中，并以通常的方式连接到CMake中。

现在，让我们实现基础知识。

---

## 1）`Font` class

创建新文件夹：`source/font/`（或`engine/source/font/`，根据您的结构）。  
在内部，创建`Font.h`和`Font.cpp`。

我们将声明一个`Font`类。我们将能够请求 **给定像素大小** 的渲染。

* 字段：

```cpp
// 每个 glyph 都有一个描述（在 atlas + metrics 中的位置）
struct GlyphDescription 
{
    int x0, y0;  // 地图集左下角
    int x1, y1;  // 地图集右上角

    int width;   // 光栅化图示符宽度
    int height;  // 光栅化图示符高度

    int advance; // 水平前进（到下一个字形开始的距离）
};

class Font 
{
public:
    int GetSize() const { return m_size; }

    const GlyphDescription& GetGlyphDescription(char asciiCode) const 
    {
        return m_descriptions[static_cast<unsigned char>(asciiCode)];
    }

    const std::shared_ptr<Texture>& GetTexture() const { return m_texture; }

private:
    // 为此字体实例选择的像素大小
    int m_size = 0;

    // 出于学习目的，我们将保留ASCII 0..127（128个符号）
    GlyphDescription m_descriptions[128];

    // 包含所有轮廓位图的纹理图集
    std::shared_ptr<Texture> m_texture;

    // FontManager需要填充私有字段：
    friend class FontManager;
};
```

为什么**128**？因为我们将在本课程中使用标准的**ASCII**表：英文字母、数字、标点符号等。这对我们的学习任务来说已经足够了。

不要忘记`#include "graphics/Texture.h"`（或您的实际包含路径）用于`Texture`类型。

---

## 2）`FontManager`（字体缓冲区/管理器）

我们还需要一个**manager**来加载、存储和服务不同系列和大小的字体。在同一`font`文件夹中创建`FontManager.h`和`FontManager.cpp`。

我们将集成**FreeType**：

* FreeType 要求的转发声明：

```cpp
// Forward decls as FreeType expects:
typedef struct FT_LibraryRec_* FT_Library;
```

* 类大纲：

```cpp
class FontManager 
{
public:
    ~FontManager();
    void Init();
    std::shared_ptr<Font> GetFont(const std::string& path, int size);

private:
    FT_Library m_fontLibrary = nullptr;

    // 对于每个字体路径，我们存储一个映射：size->font实例
    using FontFamily = std::unordered_map<int, std::shared_ptr<Font>>;
    std::unordered_map<std::string, FontFamily> m_fonts;
};
```

### 引擎集成

将`FontManager`字段添加到`Engine`：

```cpp
FontManager m_fontManager;
FontManager& GetFontManager() { return m_fontManager; }
```

在`Engine:：Init（）`（底部附近，在`Application:：Init（）`之前）中，调用：

```cpp
m_fontManager.Init();
```

---

## 3）FreeType 初始化和关闭

在`FontManager.cpp`中：

```cpp
#include <ft2build.h>
#include FT_FREETYPE_H

void FontManager::Init() 
{
    FT_Error error = FT_Init_FreeType(&m_fontLibrary);
    if (error != FT_Err_Ok) 
    {
        // Initialization failed; handle (log) as needed
        m_fontLibrary = nullptr;
        return;
    }
}

FontManager::~FontManager() 
{
    if (m_fontLibrary) 
    {
        FT_Done_FreeType(m_fontLibrary);
        m_fontLibrary = nullptr;
    }
}
```

---

## 4）加载或检索字体：`GetFont(path, size)`

该功能将：

1. 尝试在缓存中查找请求的字体系列和大小，
2. 如果找不到，**从内存加载**字体，
3. 设置**像素大小**，
4. **将**ASCII标志符号光栅化为单个RGBA地图集，
5. 从地图集创建`Texture`，
6. 填充`Font`的字形描述，
7. 存储在缓存中并返回。

伪代码（包括所有重要步骤）：

```cpp
std::shared_ptr<Font> FontManager::GetFont(const std::string& path, int size) 
{
    // 1) 检查缓存
    if (auto famIt = m_fonts.find(path); famIt != m_fonts.end()) 
    {
        auto& fam = famIt->second;
        if (auto it = fam.find(size); it != fam.end())
        {
            return it->second;
        }
    }

    // 2) 将字体文件加载到内存中
    auto buffer = Engine::GetInstance()->GetFileSystem()->LoadAssetFile(path);
    if (buffer.empty()) 
    {
        return nullptr;
    }

    // 3) 从内存创建FreeType面
    FT_Face face = nullptr;
    FT_Error res = FT_New_Memory_Face(
        m_fontLibrary,
        reinterpret_cast<const FT_Byte*>(buffer.data()), buffer.size(),
        0, &face
    );
    if (res != FT_Err_Ok) 
    {
        return nullptr;
    }

    // 4) 设置像素大小
    FT_Set_Pixel_Sizes(face, 0, size);

    //5）确定地图集大小（两个足以容纳所有ASCII符号的幂）
    //我们将使用基于字体度量的简单启发式方法。
    //26.6 fixed-point 高度 → 向右移动6以获得像素。
    const int lineHeight = (face->size->metrics.height >> 6);
    // 估计可以容纳128个字形的正方形（粗略启发式）
    int maxDimension = static_cast<int>(std::sqrtf(128.0f) * (lineHeight + 1));
    int textureWidth = 1;
    while (textureWidth < maxDimension) textureWidth <<= 1;
    int textureHeight = textureWidth;

    // 6) 创建RGBA缓冲区并用零填充（透明）
    const size_t stride = textureWidth * 4;
    const size_t totalBytes = static_cast<size_t>(textureWidth) * textureHeight * 4;
    auto* atlas = new unsigned char[totalBytes];
    std::memset(atlas, 0, totalBytes);

    // 7) 将ASCII 0..127光栅化到地图集中
    int penX = 0;
    int penY = 0;

    auto font = std::make_shared<Font>();

    for (int c = 0; c < 128; ++c) 
    {
        // 加载呈现图示符（&R）
        if (FT_Load_Char(face, c, FT_LOAD_RENDER) != FT_Err_Ok) 
        {
            // 失败的glyph；跳过但保持指标健全
            font->m_descriptions[c] = {0,0,0,0, 0,0, 0};
            continue;
        }

        FT_Bitmap& bmp = face->glyph->bitmap;

        // 溢出时换行
        if (penX + static_cast<int>(bmp.width) >= textureWidth) 
        {
            penX = 0;
            penY += lineHeight + 1;
        }

        // 将图示符位图（灰度）复制到RGBA地图集
        for (uint32_t row = 0; row < bmp.rows; ++row) 
        {
            for (uint32_t col = 0; col < bmp.width; ++col) 
            {
                int x = penX + static_cast<int>(col);
                int y = penY + static_cast<int>(row);
                if (x < 0 || x >= textureWidth || y < 0 || y >= textureHeight) continue;

                const unsigned char value = bmp.buffer[row * bmp.pitch + col];
                const size_t idx = static_cast<size_t>(y) * stride + x * 4;

                // 将灰度写入所有RGB通道，以及alpha
                atlas[idx + 0] = value;
                atlas[idx + 1] = value;
                atlas[idx + 2] = value;
                atlas[idx + 3] = value;
            }
        }

        // 存储图示符矩形 & metrics
        auto& gd = font->m_descriptions[c];
        gd.x0 = penX;
        gd.y0 = penY;
        gd.x1 = penX + static_cast<int>(bmp.width);
        gd.y1 = penY + static_cast<int>(bmp.rows);
        gd.width  = static_cast<int>(bmp.width);
        gd.height = static_cast<int>(bmp.rows);
        gd.advance = (face->glyph->advance.x >> 6); // 26.6 fixed 转为像素

        // 间隔1px的前进光标
        penX += static_cast<int>(bmp.width) + 1;
    }

    // 8) 从贴图集中创建纹理
    font->m_texture = std::make_shared<Texture>(textureWidth, textureHeight, 4, atlas);
    font->m_size = size;

    // 9) 缓存它
    m_fonts[path][size] = font;

    // 10) 清理
    delete[] atlas;
    FT_Done_Face(face);

    return font;
}
```

这是我们的字体加载管道的核心：我们生成 **纹理图集** 并存储 **每个字形描述符**。

---

## 5）将字体挂接到`TextComponent`

让我们扩展`TextComponent`，让它知道要使用哪种字体、颜色等。

添加到`TextComponent`：

```cpp
private:
    std::string m_text;
    glm::vec4   m_color = glm::vec4(1.0f);
    std::shared_ptr<Font> m_font;

public:
    void SetText(const std::string& t) { m_text = t; }
    const std::string& GetText() const { return m_text; }

    void SetColor(const glm::vec4& c) { m_color = c; }
    const glm::vec4& GetColor() const { return m_color; }

    void SetFont(const std::shared_ptr<Font>& font) { m_font = font; }
    void SetFont(const std::string& path, int size) 
    {
        m_font = Engine::GetInstance().GetFontManager().GetFont(path, size);
    }
    const std::shared_ptr<Font>& GetFont() const { return m_font; }
```

并添加`LoadProperties(const nlohmann::json& j)`:

```cpp
void TextComponent::LoadProperties(const nlohmann::json& j) 
{
    const std::string text = j.value("text", "");
    SetText(text);

    if (j.contains("font")) 
    {
        const auto& fj = j["font"];
        std::string fontPath = fj.value("path", "");
        int fontSize = fj.value("size", 12);
        if (!fontPath.empty()) 
        {
            SetFont(fontPath, fontSize);
        }
    }

    if (j.contains("color")) 
    {
        const auto& cj = j["color"];
        glm::vec4 col(
            cj.value("r", 1.0f),
            cj.value("g", 1.0f),
            cj.value("b", 1.0f),
            cj.value("a", 1.0f)
        );
        SetColor(col);
    }
}
```

在`Render(CanvasComponent* canvas)`中，我们很快将绘制文本。目前，我们准备先决条件：

```cpp
void TextComponent::render(CanvasComponent* canvas) 
{
    if (m_text.empty() || !m_font || !canvas) 
    {
        return;
    }

    // 我们将在下一课画画。
}
```

---

## 6）枢轴感知放置助手

我们将通过计算 **pivot** 和渲染字符串的 **最后一个矩形** 来计算从何处开始绘制文本。

添加小辅助对象：

```cpp
glm::vec2 GetPivotPos() const 
{
    auto pos = m_owner->GetWorldPosition2D(); // 我们早期的2D助手

    glm::vec2 rect(0.0f); // 全文的宽度/高度
    for (const char c : m_text) 
    {
        const auto& d = m_font->GetGlyphDescription(c);
        rect.x += static_cast<float>(d.advance);
        rect.y = std::max(rect.y, static_cast<float>(d.height));
    }

    // 通过枢轴移动位置（两个轴上的0..1）
    // 记住：文本渲染从左下角开始
    pos.x -= std::round(rect.x * m_pivot.x);
    pos.y -= std::round(rect.y * m_pivot.y);
    return pos;
}
```

这将计算**起点**，以便根据轴心对齐完整渲染的字符串（例如，(0.5,0.5)使其居中）。

---

## 我们现在所处的位置

* 我们解释了**font**是什么，以及为什么需要光栅化。
* 我们创建了一个具有**glyph描述符**和**纹理图集**的`Font`类。
* 我们构建了一个`FontManager`，**初始化FreeType**，**从内存加载**字体，**将ASCII**光栅化为两个RGBA图集的幂，**通过`(path, size)`缓存**结果。
* 我们将`FontManager`集成到`Engine`中。
* 我们使用**font**、**color**和**text**扩展了`TextComponent`，添加了JSON加载，并准备了一个**支持枢轴点（pivot-aware）定位**的辅助工具。

剩下的是最有趣的部分 —— **实际绘制**文本，使用我们的 2D 管道（每个字形类似精灵的四边形，从地图集采样，按每个字形的`advance`前进）。我们将在**下一课**中进行此操作。