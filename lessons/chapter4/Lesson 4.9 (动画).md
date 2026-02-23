# 将模型变为现实——加载和播放glTF动画

现在我们可以加载模型了，是时候 **让它们栩栩如生** 了 —— 即加载和播放 **动画**。  
glTF文件可以存储对象的动画，在本课程中，我们将使用简单的对象动画。这足以理解核心概念及其在实际项目中的应用。

**什么是动画？** 
从概念上讲，随着时间的推移，它只是一组 **关键帧**。每个关键帧存储对象在特定时间的 **位置**、**旋转**或**缩放**。就是这样 —— 一个转变的时间表。

我们将首先添加一个专用组件来管理动画。

---

## 1）新组件：`动画组件`

在“scene/components/”中创建两个文件：

* `AnimationComponent.h`
* `AnimationComponent.cpp`

在`AnimationComponent.h `中，声明类`AnimationComponent`。

---

## 2）数据结构：关键帧、轨迹、片段、绑定

如前所述，动画 = 带时间戳的变换。我们将定义紧凑的关键帧结构，然后在其上构建轨迹/片段。

### 2.1 关键帧

* 对于 **位置**/**缩放**（矢量关键帧）：

```cpp
struct KeyframeVec3 
{
    float time = 0.0f;
    glm::vec3 value = glm::vec3(0.0f);
};
```

* 对于**旋转**（四元数关键帧）：

```cpp
struct KeyframeQuat 
{
    float time = 0.0f;
    glm::quat value = glm::quat(1, 0, 0, 0); // w,x,y,z
};
```

### 2.2 变换轨迹（每个目标对象）

一个轨迹以 **一个对象**（按名称）为目标，可能携带三个通道：

```cpp
struct TransformTrack 
{
    std::string targetName;                 // name of the animated GameObject
    std::vector<KeyframeVec3> positions;    // translation keys
    std::vector<KeyframeQuat> rotations;    // rotation keys (quats)
    std::vector<KeyframeVec3> scales;       // scale keys
};
```

### 2.3 动画剪辑（可以为多个对象设置动画）

一个剪辑可能包含多个轨迹（例如，武器射击：扳机、滑动、枪口闪光等）：

```cpp
struct AnimationClip 
{
    std::string name;                       // clip name
    float duration = 0.0f;                  // total length (seconds)
    bool looping = true;                    // 默认循环标志
    std::vector<TransformTrack> tracks;     // 所有片段
};
```

### 2.4 对象绑定（从片段轨迹到场景对象的快速映射）

我们将在顶级对象上有 **一个**`AnimationComponent`，而片段通常会为**子类**设置动画。我们需要一种快速的方法将轨迹目标映射到实际的“游戏对象”，并记住哪些轨迹索引会影响每个对象：

```cpp
struct ObjectBinding 
{
    GameObject* object = nullptr;           // 要设置动画的实际对象
    std::vector<size_t> trackIndices;       // AnimationClip:：tracks中的索引
};
```

---

## 3）`AnimationComponent`字段和基本API

添加到`AnimationComponent`：

```cpp
class AnimationComponent : public Component 
{
    COMPONENT(AnimationComponent)
public:
    void Update(float deltaTime) override;
    void SetClip(AnimationClip* clip);
    void RegisterClip(const std::string& name, const std::shared_ptr<AnimationClip>& clip);
    void Play(const std::string& name, bool loop = true);

private:
    // Current playback state
    AnimationClip* m_clip = nullptr;
    float m_time = 0.0f;
    bool m_looping = true;
    bool m_isPlaying = false;

    // All clips loaded from this object’s glTF
    std::unordered_map<std::string, std::shared_ptr<AnimationClip>> m_clips;

    // Target-object bindings for current clip
    std::unordered_map<GameObject*, std::unique_ptr<ObjectBinding>> m_bindings;

    // Helpers
    void BuildBindings(); // rebuild m_bindings for m_clip

    // Interpolation helpers
    glm::vec3 Interpolate(const std::vector<KeyframeVec3>& keys, float time) const;
    glm::quat Interpolate(const std::vector<KeyframeQuat>& keys, float time) const;
};
```

---

## 4）`Update`：时间、循环和应用变换

### 4.1 提前退出和时间更新

```cpp
void AnimationComponent::Update(float deltaTime) 
{
    if (!m_clip) 
    {
        return;
    }
    if (!m_isPlaying) 
    {
        return;
    }

    m_time += deltaTime;

    if (m_time > m_clip->duration) 
    {
        if (mLooping) 
        {
            m_time = std::fmod(m_time, m_clip->duration);
        } 
        else 
        {
            m_time = 0.0f;
            m_isPlaying = false; // stop (do not clear m_clip, we might replay)
            return;
        }
    }
```

### 4.2 应用于所有绑定对象

```cpp
    for (auto& binding : m_bindings) 
    {
        GameObject* obj = binding.first;
        const auto& indices = binding.second->trackIndices;

        for (size_t idx : indices) 
        {
            const TransformTrack& track = m_clip->tracks[idx];

            // Evaluate tracks at m_time
            if (!track.positions.empty())
            {
                glm::vec3 pos = Interpolate(track.positions, mTime);
                obj->SetPosition(pos);
            } 
            if (!track.rotations.empty()) 
            {
                glm::quat rot = Interpolate(track.rotations, mTime);
                obj->SetRotation(rot);
            }
            if (!track.scales.empty())
            {
                glm::vec3 scl = Interpolate(track.scales,    mTime);
                obj->SetScale(scl);
            }    
        }
    }
}
```

---

## 5）插值助手（Vec3线性，四元数插值）

我们将线性插值向量和**四元数插值(Quat slerp)**。

**处理的边缘情况**：

* No keys → 返回零/默认值。
* One key → 返回该键的值。
* 时间超出关键范围 → 跳到第一个/最后一个值。
* 使用`key.time>=time`找到第一个键以获得正确的片段。

```cpp
glm::vec3 AnimationComponent::Interpolate(const std::vector<KeyframeVec3>& keys, float time) const 
{
    if (keys.empty()) 
    {
        return glm::vec3(0.0f);
    }

    if (keys.size() == 1) 
    {
        return keys[0].value;
    }

    if (time <= keys.front().time)
    {
        return keys.front().value;
    }

    if (time >= keys.back().time)
    {
        return keys.back().value;
    }

    size_t i0 = 0;
    size_t i1 = 0;

    for (size_t i = 1; i < keys.size(); ++i)
    {
        if (time <= keys[i].time)
        {
            i1 = i;
            break;
        }
    }

    i0 = i1 > 0 ? i1 - 1 : 0;

    if (time >= keys[i0].time && time <= keys[i1].time)
    {
        float deltaTime = keys[i1].time - keys[i0].time;
        float k = (time - keys[i0].time) / deltaTime;

        return glm::mix(keys[i0].value, keys[i1].value, k);
    }

    return keys.back().value;
}

glm::quat AnimationComponent::Interpolate(const std::vector<KeyframeQuat>& keys, float t) const 
{
    if (keys.empty())
    {
        return glm::vec3(0.0f);
    }

    if (keys.size() == 1)
    {
        return keys[0].value;
    }

    if (time <= keys.front().time)
    {
        return keys.front().value;
    }

    if (time >= keys.back().time)
    {
        return keys.back().value;
    }

    size_t i0 = 0;
    size_t i1 = 0;

    for (size_t i = 1; i < keys.size(); ++i)
    {
        if (time <= keys[i].time)
        {
            i1 = i;
            break;
        }
    }

    i0 = i1 > 0 ? i1 - 1 : 0;

    if (time >= keys[i0].time && time <= keys[i1].time)
    {
        float deltaTime = keys[i1].time - keys[i0].time;
        float k = (time - keys[i0].time) / deltaTime;

        return glm::slerp(keys[i0].value, keys[i1].value, k);
    }

    return keys.back().value;
}
```

---

## 6）`SetClip`、`RegisterClip`和`Play`

```cpp
void AnimationComponent::SetClip(AnimationClip* clip) 
{
    m_clip = clip;
    m_time = 0.0f;
    BuildBindings();
}

void AnimationComponent::RegisterClip(const std::string& name,
                                      const std::shared_ptr<AnimationClip>& clip) 
{
    mClips[name] = clip;
}

void AnimationComponent::Play(const std::string& name, bool loop) 
{
    // 如果已在此剪辑中，请重新启动
    if (m_clip && m_clip->name == name) 
    {
        m_time     = 0.0f;
        m_isPlaying = true;
        m_looping  = loop;
        return;
    }
    // 切换到已注册的剪辑
    if (auto it = m_clips.find(name); it != m_clips.end()) 
    {
        SetClip(it->second);
        m_isPlaying = true;
        m_looping   = loop;
    }
}
```

### 6.1 构建绑定（跟踪目标 → 对象）

```cpp
void AnimationComponent::BuildBindings() 
{
    m_bindings.clear();
    if (!m_clip) 
    {
        return;
    }

    for (size_t i = 0; i < m_clip->tracks.size(); ++i) 
    {
        const auto& track = m_clip->tracks[i];

        GameObject* target =
            m_owner->FindChildByName(track.targetName); // recursive
        if (!target) 
        {
            continue;
        }

        auto it = m_bindings.find(target);
        if (it != m_bindings.end()) 
        {
            it->second->trackIndices.push_back(i);
        } 
        else 
        {
            auto binding = std::make_unique<ObjectBinding>();
            binding->object = target;
            binding->trackIndices.push_back(i);
            m_bindings.emplace(target, std::move(binding));
        }
    }
}
```

---

## 7）在`GameObject::LoadGLTF`中从glTF加载动画

我们将在“GameObject”中添加辅助方法，然后解析glTF的“animations”。

### 7.1 `FindChildByName`

添加到`GameObject`：

```cpp
GameObject* FindChildByName(const std::string& name) 
{
    if (m_name == name) 
    {
        return this;
    }

    for (auto& child : m_children) 
    {
        if (auto res = child->FindChildByName(name)) 
        {
            return res;
        }
    }
    return nullptr;
}
```

### 7.2 glTF访问器助手

我们将读取标量、vec3、quats、输入时间和输出值：

```cpp
static float ReadScalar(const cgltf_accessor* acc, cgltf_size index) 
{
    float v = 0.0f;
    cgltf_accessor_read_float(acc, index, &v, 1);
    return v;
}

static glm::vec3 ReadVec3(const cgltf_accessor* acc, cgltf_size index) 
{
    glm::vec3 v(0.0f);
    cgltf_accessor_read_float(acc, index, glm::value_ptr(v), 3);
    return v;
}

static glm::quat ReadQuat(const cgltf_accessor* acc, cgltf_size index) 
{
    float tmp[4] = {0,0,0,1}; // x,y,z,w expected by cgltf; adjust to glm ctor
    cgltf_accessor_read_float(acc, index, tmp, 4);
    // glm::quat(w, x, y, z)
    return glm::quat(tmp[3], tmp[0], tmp[1], tmp[2]);
}

static void ReadTimes(const cgltf_accessor* acc, std::vector<float>& outTimes) 
{
    outTimes.resize(acc->count);
    for (cgltf_size i = 0; i < acc->count; ++i)
    {
        outTimes[i] = ReadScalar(acc, i);
    }
}

static void ReadOutputVec3(const cgltf_accessor* acc, std::vector<glm::vec3>& outValues) 
{
    outValues.resize(acc->count);
    for (cgltf_size i = 0; i < acc->count; ++i)
    {
        outValues[i] = ReadVec3(acc, i);
    }
}

static void ReadOutputQuat(const cgltf_accessor* acc, std::vector<glm::quat>& outValues) 
{
    outValues.resize(acc->count);
    for (cgltf_size i = 0; i < acc->count; ++i)
    {
        outValues[i] = ReadQuat(acc, i);
    }
}
```

### 7.3 解析`animations`

在`GameObject::LoadGLTF(...)`中，创建结果根对象并解析场景节点后，添加：

```cpp
std::vector<std::shared_ptr<AnimationClip>> clips;

for (cgltf_size ai = 0; ai < data->animations_count; ++ai) 
{
    auto& anim = data->animations[ai];

    auto clip = std::make_shared<AnimationClip>();
    clip->name     = anim.name ? anim.name : "noname";
    clip->duration = 0.0f;

    // Map node* -> track index in clip->tracks
    std::unordered_map<cgltf_node*, size_t> trackIndexOf;

    auto GetOrCreateTrack = [&](cgltf_node* node) -> TransformTrack& 
    {
        if (auto it = trackIndexOf.find(node); it != trackIndexOf.end())
        {
            return clip->tracks[it->second];
        }

        TransformTrack track;
        track.targetName = node && node->name ? node->name : std::string{};
        clip->tracks.push_back(track);
        size_t idx = clip->tracks.size() - 1;
        trackIndexOf[node] = idx;
        return clip->tracks[idx];
    };

    for (cgltf_size ci = 0; ci < anim.channels_count; ++ci) 
    {
        auto& channel = anim.channels[ci];
        auto  sampler = channel.sampler;

        if (!channel.target_node || !sampler || !sampler->input || !sampler->output) 
        {
            continue;
        }

        std::vector<float> times;
        ReadTimes(sampler->input, times);
        if (times.empty()) 
        {
            continue;
        }

        auto& track = GetOrCreateTrack(channel.target_node);

        switch (channel.target_path) 
        {
        case cgltf_animation_path_type_translation: 
        {
            std::vector<glm::vec3> values;
            ReadOutputVec3(sampler->output, values);
            track.positions.resize(times.size());
            for (size_t i = 0; i < times.size(); ++i) 
            {
                track.positions[i].time  = times[i];
                track.positions[i].value = values[i];
            }
            break;
        }
        case cgltf_animation_path_type_scale: {
            std::vector<glm::vec3> values;
            ReadOutputVec3(sampler->output, values);
            track.scales.resize(times.size());
            for (size_t i = 0; i < times.size(); ++i) 
            {
                track.scales[i].time  = times[i];
                track.scales[i].value = values[i];
            }
            break;
        }
        case cgltf_animation_path_type_rotation: 
        {
            std::vector<glm::quat> values;
            ReadOutputQuat(sampler->output, values);
            track.rotations.resize(times.size());
            for (size_t i = 0; i < times.size(); ++i) 
            {
                track.rotations[i].time  = times[i];
                track.rotations[i].value = values[i];
            }
            break;
        }
        default:
            // Ignore weights/morph targets for now
            break;
        }

        // Update clip duration
        clip->duration = std::max(clip->duration, times.back());
    }

    clips.push_back(std::move(clip));
}
```

### 7.4 安装组件并注册片段

在`LoadGLTF`结束时，在释放`data`之前：

```cpp
if (!clips.empty()) 
{
    auto animComp = new AnimationComponent();
    resultObject->AddComponent(animComp);
    for (auto& clip : clips) 
    {
        animComp->RegisterClip(clip->name, clip);
    }
}
```

---

## 8）对象激活切换（“SetActive”）

我们需要隐藏原始动画中可能存在的某些效果对象（例如枪口闪光、子弹网格）。

添加到`GameObject`：

```cpp
private:
    bool m_active = true;

public:
    void SetActive(bool active) 
    { 
        m_active = active; 
    }

    bool IsActive() const 
    { 
        return m_active; 
    }
```

在`GameObject::Update`（方法顶部）中：

```cpp
if (!m_active) 
{
    return;
}
```

---

## 9）在`Game.cpp`中使用它

我们已经加载了携带动画组件的武器对象“枪”。

```cpp
if (auto anim = gun->GetComponent<AnimationComponent>()) 
{
    // 隐藏我们不需要的效果子对象
    if (auto bullet = gun->FindChildByName("bullet_33")) 
    {
        bullet->SetActive(false);
    }

    if (auto fire = gun->FindChildByName("BOOM_35")) 
    {
        fire->SetActive(false);
    }
    // 播放一次“射击”动画（无循环）
    anim->Play("shoot", /*loop=*/false);
}
```

如果设置`loop = true`，则片段将循环播放。

---

## 10）结果

* 播放动画（例如，**射击**）。
* 原始资源中存在的额外效果节点（`bullet_33`、`BOOM_35`）可以根据我们的需要禁用。
* 我们可以控制 **循环**，**启动/停止**，**绑定** 是基于目标名称自动构建的。

**恭喜你！**现在，您已经为在引擎中加载和播放简单的**glTF变换动画**奠定了工作基础，这是理解游戏构建方式的重要一步。
