好吧，现在我想谈谈让相机旋转更方便。  
正如你所记得的，到目前为止，我们一直在使用三个旋转角度 —— X、Y和Z轴各一个 —— 来设置相机（以及任何物体）的方向。

乍一看，这似乎很简单：通过这三个角度，我们可以描述空间中的任何方向。  
但问题是，顺序很重要。

如果你看看`GameObject`类内部，你会看到旋转矩阵是通过一个接一个地应用三个旋转来构建的，每个旋转都围绕其相应的轴。  
这里有一个问题 —— 使用的*正确*顺序是什么？  
因为如果我们改变顺序，对象的最终方向也会改变。

例如：

* 先绕X旋转，然后绕Y旋转 → 一个方向。
* 先绕Y旋转，然后绕X旋转 → 完全不同的方向。

在某些情况下，我们可能需要一个顺序，在其他情况下，可能需要另一个顺序。当我们想要实现特定的旋转时，这可能会造成冲突的情况。

那么，我们该如何解决这个问题呢？

我有一个很好的答案：我们可以使用一个称为 **四元数** 的实体，而不是使用三个单独的旋转角度。

---

### 四元数是什么？

四元数由四个分量组成：“x”、“y”、“z”和“w”。  
本质上，四元数是一个在3D空间中存储方向的数学对象。

它定义了对象围绕其旋转的 **轴** ，以及描述旋转程度的 **角度**。  
你可以把“x”、“y”和“z”看作是定义旋转轴，而“w”表示旋转量（角度）。  

因此，我们没有使用三个独立的角度（每个轴一个），而是使用一个四元数来清晰地存储方向，并完全避免了旋转顺序问题。

---

### 将游戏对象转换为使用四元数

我们将转到“GameObject”类，并将“m_rotation”成员从“vec3”更改为“quat”。  
默认情况下，它将被初始化为：

```cpp
glm::quat(1.0f, 0.0f, 0.0f, 0.0f)
```

这是 **恒等四元数** ，意思是“不旋转”

我们还相应地更新了`GetRotation`和`SetRotation`方法。

接下来，在`GetLocalTransform`中，我们删除了按顺序应用三个单独旋转的旧代码。  
相反，我们只是写：

```cpp
mat = mat * glm::mat4_cast(m_rotation);
```

在这里，`glm:：mat4_cast`简单地将我们的四元数转换为旋转矩阵。  
这一步取代了所有早期的多轴旋转逻辑。

---

### 更新 CameraComponent
现在让我们更新“CameraComponent”的“GetViewMatrix”方法。

我们从恒等矩阵开始：

```cpp
glm::mat4 mat = glm::mat4(1);
```

对于相机，变换顺序与普通对象略有不同。  
首先，我们应用旋转：

```cpp
mat = glm::mat4_cast(m_owner->GetRotation());
```

然后，我们应用转换：

```cpp
mat = glm::translate(mat, m_owner->GetPosition());
```

我们不应用缩放，因为相机缩放对视图变换没有意义。

如果摄影机有父对象，则乘以父对象的世界变换：

```cpp
if (m_owner->GetParent())
    mat = m_owner->GetParent()->GetWorldTransform() * mat;
```

最后，我们反转矩阵：

```cpp
return glm::inverse(mat);
```

---

### 更新 PlayerController 以进行四元数旋转

在“PlayerControllerComponent:：Update”中，我们更改了相机旋转对鼠标移动的反应方式。

在编写代码之前，让我们回顾一下相机应该如何旋转：

相机有三个局部基向量：

* **前** —— 相机正在看的方向。
* **右** —— 从相机角度看向右的方向。
* **向上** —— 相对于相机的向上方向。

旋转时：

* 水平移动鼠标（ΔX）应围绕 **全局** Y轴旋转（世界向上）。
* 垂直移动鼠标（ΔY）应围绕相机的 **局部** 右轴旋转。

---

### 水平旋转（围绕全局Y轴）

```cpp
float yAngle = -deltaX * m_sensitivity * deltaTime;
glm::quat yRot = glm::angleAxis(yAngle, glm::vec3(0.0f, 1.0f, 0.0f));
```

---

### 垂直旋转（围绕局部右轴）

```cpp
float xAngle = -deltaY * m_sensitivity * deltaTime;
glm::vec3 right = rotation * glm::vec3(1.0, 0.0, 0.0);
glm::quat xRot = glm::angleAxis(xAngle, right);
```

---

### 组合旋转

我们将它们相乘：

```cpp
glm::quat deltaRot = yRot * xRot;
rotation = glm::normalize(deltaRot * rotation);
m_owner->SetRotation(rotation);
```

### 运动基本保持不变

我们仍然得到更新的旋转，然后推导出前向量和右向量：

```cpp
glm::vec3 front = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 right = rotation * glm::vec3(1.0f, 0.0f, 0.0f);
```

然后，这些与以前一样用于运动输入。

---

### 测试

当我们现在运行项目时，相机的移动和旋转与适当的FPS（第一人称射击）相机完全一样。  
我们完全消除了旋转顺序问题，旋转感觉更平滑、更可预测。

恭喜你 —— 我们现在已经学习了处理场景、定位对象、控制相机和在世界中移动的基础知识！