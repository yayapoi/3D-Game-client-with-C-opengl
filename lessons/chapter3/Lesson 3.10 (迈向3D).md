**好吧！**

现在，让我们最终进入一个真正的3D空间，开始绘制实际的三维对象，而不是我们迄今为止渲染的二维对象。让我们让矩形生长并演变成立方体。这样，我们的平面二维形状将完全变成三维形状。

**我们该怎么做？**

正如你所记得的，我们的矩形是由四个顶点定义的。我们可以使用这个矩形（当时位于Z=0处），将其稍微向后移动半个单位，然后复制它，并将副本向前移动半个单元。  
这为我们提供了立方体的 *正面* 和 *背面*。组合起来，它们形成了一个有八个顶点的实体形状。

由于我们已经有了一个存储这些顶点的地方 —— `TestObject`中的`顶点`数组 —— 我们可以更新它。  
计划如下：

* 复制现有的4个顶点。
* 对于前4个顶点（正面），将Z坐标设置为“+0.5”。
* 对于重复的4个顶点（背面），将Z坐标设置为“-0.5”。

**接下来是指数**

索引定义了顶点连接形成三角形的顺序。这部分有点棘手，所以请仔细听我说。

我们按如下方式替换`indexs`数组：

* **Front face:** `0, 1, 2` and `0, 2, 3`
* **Top face:** `4, 5, 1` and `4, 1, 0`
* **Right face:** `4, 0, 3` and `4, 3, 7`
* **Left face:** `1, 5, 6` and `1, 6, 2`
* **Bottom face:** `3, 2, 6` and `3, 6, 7`
* **Back face:** `4, 7, 6` and `4, 6, 5`

就是这样 —— 现在我们的坐标和索引定义了一个立方体。
顶点布局没有改变，因此我们的网格组件将自动计算顶点和索引计数。

---

### 测试前——谈谈深度缓冲区

现在我们正在转向3D，我们需要谈谈一些重要的事情：**深度缓冲区**，也称为**Z缓冲区**。

这是怎么一回事？  
把它想象成一个不可见的灰度图像，与你的屏幕大小完全相同。对于您绘制的每个像素，深度缓冲区都会存储其 **Z坐标**（距离相机的距离）。  
当绘制下一个对象时，GPU会根据缓冲区中已存储的内容检查其深度：

* 如果它更靠近相机，它会替换像素。
* 如果它离得更远，它就隐藏在已经存在的东西后面。

**我们为什么需要它？**  
想象一下，你首先绘制一个离相机更近的平面，然后在它后面绘制一个球体。如果没有深度缓冲区，球体可能会因为稍后渲染而错误地绘制在平面上。启用深度缓冲区后，GPU可以正确地确定球体在平面后面并相应地隐藏它。

---

### 启用深度测试

在OpenGL中，默认情况下深度测试是 **禁用** 的，因此我们需要将其打开。
在`GraphicsAPI.h`（或`.cpp`）中，添加一个方法:

```cpp
bool Init() 
{
    glEnable(GL_DEPTH_TEST);
    return true;
}
```
在`Engine:：Init（）`中，将其插入到`m_application->Init（）`之前

此外，在清除屏幕时，除了颜色缓冲区外，我们还必须清除深度缓冲区，否则旧的深度值会把事情搞砸。  
在“ClearBuffers”中，更改：

```cpp
glClear(GL_COLOR_BUFFER_BIT);
```

to:

```cpp
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

现在我们已经准备好正确绘制3D对象了。

---

### 测试立方体

运行项目并移动相机——你现在应该看到一个 **3D立方体**，而不是一个扁平的矩形。

---

### 绘制多个对象

让我们更进一步：在场景中渲染 **多个对象**。

最简单（但不是最有效）的方法是只创建多个`TestObject`实例。但问题是，每个`TestObject`构造函数都会创建一个新的网格和一种新的材质，这意味着我们不必要地复制了GPU资源。

**更好的方法：**

* 在`Game:：Init（）`中创建一个共享的`Mesh`和`Material`。
* 将这些传递给您创建的所有对象。

例子：

```cpp
auto mesh = std::make_shared<Mesh>(...);
auto material = std::make_shared<Material>(...);

auto objectA = m_scene->CreateObject();
objectA->AddComponent(new MeshComponent(material, mesh));
objectA->SetPosition({0, 0, 0});

auto objectB = m_scene->CreateObject();
objectB->AddComponent(new MeshComponent(material, mesh));
objectB->SetPosition({2, 0, 0});
objectB->SetRotation({1, 0, 0});

auto objectC = m_scene->CreateObject();
objectC->AddComponent(new MeshComponent(material, mesh));
objectC->SetPosition({-2, 0, 0});
objectC->SetScale({1.5f, 1.5f, 1.5f});
```

运行它 —— 您将在场景中看到多个立方体，所有立方体都使用相同的网格和材质高效绘制。