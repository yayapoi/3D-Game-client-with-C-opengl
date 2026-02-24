# 为场景添加声音

现在是时候将 **声音** 添加到我们的场景中了。  
对于音频，我们将使用一个名为**miniaudio**的轻量级纯头文件库。

你需要做的就是：

* 在您的**CMake**文件中添加一个`include_tirectories`条目，
* 为音频驱动程序添加特定于平台的依赖关系。

好了，我们可以开始了。

---

## 1）音频管理器

我们将从简单的开始，就像其他系统一样：创建一个**音频管理器**。

1. 转到`engine/source/`。  
    创建一个新文件夹：`audio/`。  
    在内部，添加两个文件：

    * `AudioManager.h`
    * `AudioManager.cpp`
    * 声明一个类`AudioManager`

2. 在`Engine`中添加：

    * `#include "audio/AudioManager.h"`
    * 私有字段：

    ```cpp
    AudioManager m_audioManager;
    ```
    * getter：

    ```cpp
    AudioManager& GetAudioManager() { return m_audioManager; }
    ```

这使我们可以从任何地方访问音频系统。

---

### 实现

**AudioManager.h**

```cpp
#pragma once
#include "Common.h"
#include <memory>

struct ma_engine; // forward declaration

class AudioManager 
{
public:
    AudioManager();
    ~AudioManager();

    bool Init();
    ma_engine* GetEngine();

    void SetListenerPosition(const glm::vec3& pos);

private:
    std::unique_ptr<ma_engine> m_engine;
};
```

**AudioManager.cpp**

```cpp
#include "audio/AudioManager.h"
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

AudioManager::AudioManager() 
{
    m_engine = std::make_unique<ma_engine>();
}

AudioManager::~AudioManager() 
{
    if (m_engine)
        ma_engine_uninit(m_engine.get());
}

bool AudioManager::Init() 
{
    auto result = ma_engine_init(nullptr, m_engine.get());
    return result == MA_SUCCESS;
}

ma_engine* AudioManager::GetEngine() 
{
    return m_engine.get();
}

void AudioManager::SetListenerPosition(const glm::vec3& pos) 
{
    if (m_engine) 
    {
        ma_engine_listener_set_position(m_engine.get(), 0, pos.x, pos.y, pos.z);
    }
}
```

在`Engine:：Init（）`中，调用：

```cpp
mAudioManager.Init();
```

现在音乐系统准备好了。

---

## 2）音频资源

就像我们有**纹理**、**网格**和**模型**一样，我们也有**音频**作为资源。

创建两个文件：

* `Audio.h`
* `Audio.cpp`

**Audio.h**

```cpp
#pragma once
#include "Common.h"
#include <memory>
#include <string>
#include <vector>

struct ma_sound;
struct ma_decoder;

class Audio 
{
public:
    ~Audio();

    void SetPosition(const glm::vec3& pos);
    void Play(bool loop = false);
    void Stop();
    bool IsPlaying() const;

    void SetVolume(float volume);
    float GetVolume() const;

    static std::shared_ptr<Audio> Load(const std::string& path);

private:
    std::unique_ptr<ma_sound> m_sound;
    std::unique_ptr<ma_decoder> m_decoder;
    std::vector<char> m_buffer;
};
```

**Audio.cpp（核心逻辑）**

在析构函数中，我们需要按照正确的顺序销毁东西：

* 首先是声音，
* 然后是解码器。
```cpp
Audio::~Audio()
{
    if (m_sound)
    {
        ma_sound_uninit(m_sound.get());
    }
    if (m_decoder)
    {
        ma_decoder_uninit(m_decoder.get());
    }
}
```

在`Load`函数中：
1. 我们从文件系统加载原始二进制数据，

    ```cpp
    auto buffer = Engine::GetInstance().GetFileSystem().LoadAssetFile(path);
    ```
2. 我们得到一个指向音频引擎的指针：

    ```cpp
    auto engine = Engine::GetInstance().GetAudioManager().GetEngine();
    ```
3. 我们创建一个“音频”对象，将缓冲区存储在其中，并设置一个解码器。
   
    ```cpp
    auto audio = std::make_shared<Audio>();
    audio->m_sound = std::make_unique<ma_sound>();
    audio->m_buffer = buffer;
    audio->m_decoder = std::make_unique<ma_decoder>();
    ```
4. 使用`ma_decoder_init_memory(...)`初始化解码器。  
    如果失败，返回`nullptr`。
    
    ```cpp
    auto result = ma_decoder_init_memory(audio->m_buffer.data(), audio->m_buffer.size(), nullptr, audio->m_decoder.get());
    if (result != MA_SUCCESS)
    {
        return nullptr;
    }
    ```
5. 使用`ma_sound_init_from_data_source(...)`初始化声音。  
    如果失败，返回`nullptr`。

    ```cpp
    result = ma_sound_init_from_data_source(
        engine,
        audio->m_decoder.get(),
        0,
        NULL,
        audio->m_sound.get()
    );
    if (result != MA_SUCCESS) 
    {
        return nullptr;
    }
    ```
6. 启用空间化：

    ```cpp
    ma_sound_set_spatialization_enabled(audio->m_sound.get(), MA_TRUE);
    ```
7. 返回音频对象。

其他方法：

* `SetPosition`：更新声音在3D中的位置。
    ```cpp
    void Audio::SetPosition(const glm::vec3& position)
    {
        if (m_sound)
        {
            ma_sound_set_position(m_sound.get(), position.x, position.y, position.z);
        }
    }
    ```
* `Play`：开始播放，可选择循环播放。
    ```cpp
    void Audio::Play(bool loop)
    {
        if (m_sound)
        {
            ma_sound_start(m_sound.get());
            auto looping = loop ? MA_TRUE : MA_FALSE;
            ma_sound_set_looping(m_sound.get(), looping);
        }
    }
    ```
* `Stop`：停止并倒退到第0帧。
    ```cpp
    void Audio::Stop()
    {
        if (m_sound)
        {
            ma_sound_stop(m_sound.get());
            ma_sound_seek_to_pcm_frame(m_sound.get(), 0);
        }
    }
    ```
* `IsPlaying`：返回true/false。
    ```cpp
    bool Audio::IsPlaying() const
    {
        if (m_sound)
        {
            return ma_sound_is_playing(m_sound.get());
        }
        return false;
    }
    ```
* `SetVolume`：调整音量在0到1之间。
    ```cpp
    void Audio::SetVolume(float volume)
    {
        if (m_sound)
        {
            ma_sound_set_volume(m_sound.get(), std::clamp(volume, 0.0f, 1.0f));
        }
    }
    ```
* `GetVolume`：返回当前音量。
    ```cpp
    float Audio::GetVolume() const
    {
        if (m_sound)
        {
            return ma_sound_get_volume(m_sound.get());
        }
        return 0.0f;
    }
    ```

现在，音频可以用作我们引擎中的资源。

---

## 3）音频组件

正如您可能猜到的，我们将通过**组件**控制声音。

转到`scene/components`并添加两个新文件：

* `AudioComponent.h`
* `AudioComponent.cpp`

**AudioComponent.h**

```cpp
#pragma once
#include "scene/Component.h"
#include "audio/Audio.h"
#include <unordered_map>

class AudioComponent : public Component 
{
    COMPONENT(AudioComponent)
public:
    void LoadProperties(const nlohmann::json& j) override;
    void Update(float dt) override;

    void RegisterAudio(const std::string& name, const std::shared_ptr<Audio>& clip);
    void Play(const std::string& name, bool loop = false);
    void Stop(const std::string& name);
    bool IsPlaying(const std::string& name) const;

private:
    std::unordered_map<std::string, std::shared_ptr<Audio>> m_clips;
};
```

**加载流**：

* 解析一个JSON字段“audio”。
* 它包含一系列片段。
* 每个片段都有：

    * `name`（标识符），
    * `path`（文件路径），
    * `volume`（可选，默认值=1.0）。
* 通过`Audio:：Load（）`加载每个片段，设置音量并注册。
    ```cpp
    void AudioComponent::LoadProperties(const nlohmann::json& jsonObject)
    {
        if (jsonObject.contains("audio"))
        {
            auto& clipsObject = jsonObject["audio"];
            for (auto& clip : clipsObject)
            {
                std::string name = clip.value("name", "noname");
                std::string path = clip.value("path", "");  
                auto audio = Audio::Load(path);
                if (audio)
                {
                    float volume = clip.value("volume", 1.0f);
                    audio->SetVolume(volume);
                    RegisterAudio(name, audio);
                }
            }
        }
    }
    ```

**Update**：

* 每一帧，迭代片段。
* 如果正在播放片段，请更新其位置 = 所有者的世界位置。

```cpp
void AudioComponent::Update(float deltaTime)
{
    auto pos = m_owner->GetWorldPosition();
    for (auto& clip : m_clips)
    {
        if (clip.second->IsPlaying())
        {
            clip.second->SetPosition(pos);
        }
    }
}
```

* **注册音频**：将片段存储在map中。

```cpp
void AudioComponent::RegisterAudio(const std::string& name, const std::shared_ptr<Audio>& clip)
{
   m_clips[name] = clip;
}
```

* **播放**：按名称查找片段并播放。

```cpp
void AudioComponent::Play(const std::string& name, bool loop)
{
    auto it = m_clips.find(name);
    if (it != m_clips.end())
    {
        if (it->second)
        {
            it->second->Play(loop);
        }
    }
}
```

* **停止**：停止片段。

```cpp
void AudioComponent::Stop(const std::string& name)
{
    auto it = m_clips.find(name);
    if (it != m_clips.end())
    {
        if (it->second)
        {
            it->second->Stop();
        }
    }
}
```

* **IsPlaying**：如果片段当前处于活动状态，则返回true。

```cpp
bool AudioComponent::IsPlaying(const std::string& name) const
{
    auto it = m_clips.find(name);
    if (it != m_clips.end())
    {
        if (it->second)
        {
            return it->second->IsPlaying();
        }
    }
    return false;
}
```

---

## 4）监听器组件

我们还需要一个**listener**（“耳朵”）。  
这将是一个非常简单的组件。

**AudioListenerComponent.h**

```cpp
class AudioListenerComponent : public Component 
{
    COMPONENT(AudioListenerComponent)
public:
    void Update(float dt) override 
    {
        auto pos = m_owner->GetWorldPosition();
        Engine::GetInstance()->GetAudioManager().SetListenerPosition(pos);
    }
};
```

只有一个听众——**玩家**。

---

## 5）更新场景JSON

打开`scene.sc`，找到`MainPlayer`，然后添加：

```json
"components": [
  { "type": "CameraComponent" },
  { "type": "PlayerControllerComponent" },
  { "type": "AudioListenerComponent" },
  {
    "type": "AudioComponent",
    "audio": [
      { "name": "shoot", "path": "audio/shoot.wav" },
      { "name": "step",  "path": "audio/step.wav" },
      { "name": "jump",  "path": "audio/jump.wav" }
    ]
  }
]
```

现在，我们的播放器既有一个监听器，也有一个有三种声音的音频组件。

---

## 6）注册组件

在`Scene:：RegisterTypes（）`中，添加：

```cpp
AudioComponent::Register();
AudioListenerComponent::Register();
```

---

## 7）与Player整合

在`Player.h`中，添加字段：

```cpp
AudioComponent* m_audioComponent = nullptr;
PlayerControllerComponent* m_playerControllerComponent = nullptr;
```

在`Init（）`中分配它们。

```cpp
m_audioComponent = GetComponent<eng::AudioComponent>();
m_playerControllerComponent = GetComponent<eng::PlayerControllerComponent>();
```

---

## 8）播放声音

* **射击**：

```cpp
if (input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) 
{
    if (m_animationComponent && !m_animationComponent->IsPlaying()) 
    {
        m_animationComponent->Play("shoot", false);

        if (m_audioComponent) 
        {
            if (m_audioComponent->IsPlaying("shoot"))
            {
                m_audioComponent->Stop("shoot");
            }
            m_audioComponent->Play("shoot");
        }
    }
}
```

* **跳跃**：

```cpp
if (input.IsKeyPressed(GLFW_KEY_SPACE)) 
{
    if (m_audioComponent && !m_audioComponent->IsPlaying("jump")) 
    {
        m_audioComponent->Play("jump");
    }
}
```

* **持续走路**：

```cpp
bool walking =
    input.IsKeyPressed(GLFW_KEY_W) ||
    input.IsKeyPressed(GLFW_KEY_A) ||
    input.IsKeyPressed(GLFW_KEY_S) ||
    input.IsKeyPressed(GLFW_KEY_D);

if (walking && m_playerControllerComponent && m_playerControllerComponent->OnGround()) 
{
    if (m_audioComponent && !m_audioComponent->IsPlaying("step"))
    {
        m_audioComponent->Play("step", true); // loop
    }
} 
else 
{
    if (m_audioComponent && m_audioComponent->IsPlaying("step"))
    {
        m_audioComponent->Stop("step");
    }
}
```

---

## 9）地板上的PlayerControllerComponent

添加：

```cpp
bool PlayerControllerComponent::OnGround() const 
{
    if (m_kinematicController)
    {
        return m_kinematicController->OnGround();
    }
    return false;
}
```

---

## 结果

如果我们做得正确，没有犯错误，现在我们可以运行游戏，真正听到声音。

* 射击播放射击声音，
* 跳跃播放跳跃声，
* 在地面上移动时，行走会发出一个循环的脚步声。

祝贺 !   
这是另一个非常重要的里程碑。  
我们已经成功地将**声音**集成到我们的引擎中。  
现在我们的游戏不仅有图形和物理，还有**音频反馈**。  
太棒了