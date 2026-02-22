现在，让我们来实现场景图中对象 **分配父节点** 的核心逻辑。

我们将涵盖各种场景，例如：

* 将一个`GameObject`移动到 **场景根节点**
* 将其设置为另一个对象的子对象
* 避免层次结构中的 **循环依赖**

---

### Step-by-Step: 实现`SetParent()`函数

我们将从以下内容开始：

```cpp
bool result = false;
auto currentParent = obj->GetParent();
```

这为我们提供了想要重新分配的对象的当前父对象。

---

### **Case 1: 分配给根节点（即`parent == nullptr`）**

```cpp
if (parent == nullptr) {
```

我们想把物体移动到场景的根节点。

#### Subcase 1.1: 对象已有父对象

```cpp
if (currentParent != nullptr) {
```

我们需要：

1. 将其从当前父级的子列表中移除。
2. 将其添加到根列表（`m_objects`）中。

首先，在对象的当前父级中定位该对象：

```cpp
auto it = std::find_if(
    currentParent->m_children.begin(),
    currentParent->m_children.end(),
    [obj](const std::unique_ptr<GameObject>& el) {
        return el.get() == obj;
    });
```

If found:

```cpp
if (it != currentParent->m_children.end()) 
{
    m_objects.push_back(std::move(*it));
    obj->m_marent = nullptr;
    currentParent->m_children.erase(it);
    result = true;
}
```

#### Subcase 1.2: 对象没有父对象，但可能尚未在场景中

如果在根列表中未找到该对象，则表示它是 **新的**。我们将其添加进去：

```cpp
auto it = std::find_if(
    m_objects.begin(), m_objects.end(),
    [obj](const std::unique_ptr<GameObject>& el) 
    {
        return el.get() == obj;
    });

if (it == m_objects.end()) 
{
    std::unique_ptr<GameObject> objHolder(obj);
    m_objects.push_back(std::move(objHolder));
    result = true;
}
```

---

### **Case 2: 分配给有效父节点（即`parent != nullptr`）**

```cpp
else {
```

我们正尝试将该对象作为子对象添加到另一个`GameObject`中。

---

#### Subcase 2.1: 对象已有父对象（重新设置父对象）

```cpp
if (currentParent != nullptr) {
```

首先，在对象的当前父级的子级中查找该对象：

```cpp
auto it = std::find_if(
    currentParent->m_children.begin(),
    currentParent->m_children.end(),
    [obj](const std::unique_ptr<GameObject>& el) {
        return el.get() == obj;
    });

if (it != currentParent->m_children.end()) {
```

现在，**确保没有循环父子关系**：我们绝不允许将一个子节点分配给其自身的后代之一。

```cpp
bool found = false;
auto currentElement = parent;
while (currentElement) 
{
    if (currentElement == obj) 
    {
        found = true;
        break;
    }
    currentElement = currentElement->GetParent();
}
```

如果一切有效：

```cpp
if (!found) 
{
    parent->m_children.push_back(std::move(*it));
    obj->m_parent = parent;
    currentParent->m_children.erase(it);
    result = true;
}
```

---

#### Subcase 2.2: 对象当前位于根列表中或刚刚被创建

如果 `currentParent == nullptr`，则：

```cpp
auto it = std::find_if(
    m_objects.begin(), m_objects.end(),
    [obj](const std::unique_ptr<GameObject>& el) {
        return el.get() == obj;
    });

```

对象刚刚被创建

```cpp
if (it == m_objects.end()) 
{
    std::unique_ptr<GameObject> objHolder(obj);
    parent->m_children.push_back(std::move(objHolder));
    obj->m_parent = parent;
    result = true;
}
else {
```

然后，再次检查是否存在 **循环父项**：

```cpp
bool found = false;
auto currentElement = parent;

while (currentElement) 
{
    if (currentElement == obj) 
    {
        found = true;
        break;
    }
    currentElement = currentElement->GetParent();
}
```

如果新的父节点有效：

```cpp
if (!found) 
{
    parent->m_children.push_back(std::move(*it));
    obj->m_parent = parent;
    m_objects.erase(it);
    result = true;
}
```

---

### ✅ Final Step: 返回结果

```cpp
return result;
```

---

### 摘要

这个`SetParent()`逻辑现在可以安全地支持：

* 将对象移入或移出场景根节点 
* 在父对象之间重新分配 
* 检测并防止循环父对象（这会破坏树形结构）