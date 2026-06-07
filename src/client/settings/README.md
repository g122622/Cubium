# Client Settings 模块

客户端设置模块，管理 Minecraft 客户端的所有用户配置选项，包括视频、音频、控制、游戏、网络等设置，以及按键绑定管理。

## 目录结构

```
src/client/settings/
├── ClientSettings.hpp    # 客户端设置类定义，包含所有设置选项和按键绑定管理
└── ClientSettings.cpp    # 客户端设置类实现，包含默认按键绑定注册和设置持久化
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                      ClientSettings                          │
│  (继承 SettingsBase，管理所有客户端设置选项)                    │
└─────────────────────────────────────────────────────────────┘
        │
        ├──► 视频设置组 (video)
        │       renderDistance, framerateLimit, guiScale, fullscreen, vsync
        │       graphics, clouds, mipmapLevels, fovEffectScale, screenShakeScale
        │       fogDensity, ambientOcclusion, biomeBlendRadius, antiAliasing
        │
        ├──► 音频设置组 (audio)
        │       masterVolume, musicVolume, recordVolume, weatherVolume
        │       blockVolume, hostileVolume, neutralVolume, playerVolume
        │       ambientVolume, voiceVolume
        │
        ├──► 控制设置组 (control)
        │       mouseSensitivity, invertMouse, rawMouseInput
        │       mouseWheelSensitivity, autoJump
        │
        ├──► 游戏设置组 (game)
        │       viewBobbing, fov, showFps, showDebug, language
        │
        ├──► 网络设置组 (network)
        │       serverAddress, serverPort, username
        │
        ├──► 日志设置组 (log)
        │       logLevel
        │
        ├──► 资源包设置组 (resourcePacks)
        │       resourcePacks (ResourcePackListOption)
        │
        └──► 按键绑定 (静态)
                s_keyBindings: 移动、游戏控制、物品栏、功能键
```

## 上下游外部依赖关系

### 内部依赖

```
┌─────────────────────────────────────────────────────────────┐
│                      ClientSettings                          │
└─────────────────────────────────────────────────────────────┘
        │
        ├──► common/core/settings/SettingsBase.hpp       # 设置基类（分组管理、序列化）
        ├──► common/core/settings/SettingsTypes.hpp      # 选项类型（IOption, BooleanOption, RangeOption 等）
        ├──► common/core/settings/ResourcePackListOption.hpp  # 资源包列表选项
        ├──► common/input/KeyBinding.hpp                 # 按键绑定系统
        ├──► common/sound/SoundCategory.hpp              # 声音类别枚举
        └──► common/core/DefaultValues.hpp               # 默认值常量
```

### 外部依赖

| 库 | 用途 |
|----|------|
| nlohmann_json | JSON 序列化/反序列化 |
| spdlog | 日志输出 |
| std::filesystem | 配置文件路径操作 |

### 被依赖

客户端设置模块是底层基础模块，被以下模块依赖：
- `ClientApplication` - 初始化、加载、保存设置
- UI 系统 - 读取/修改设置选项
- 渲染引擎 - 读取视频设置
- 音频系统 - 读取音量设置
- 输入系统 - 查询按键绑定状态

## 容易踩的坑

### 1. 按键绑定必须先初始化

```cpp
// 错误：未初始化就获取按键绑定
auto* key = ClientSettings::getKeyBinding("key.forward"); // 返回 nullptr

// 正确：先初始化
settings.initializeKeyBindings();
auto* key = ClientSettings::getKeyBinding("key.forward"); // 返回有效指针
```

### 2. 按键绑定是静态全局的

KeyBinding 使用静态注册表，所有 ClientSettings 实例共享。修改按键绑定会影响全局，只需初始化一次。

### 3. 设置变更回调只在值真正改变时触发

相同值设置不会触发回调，只有值实际变化时才会触发 `onChange` 回调。

### 4. RangeOption 和 FloatOption 会自动 clamp

设置超出范围的值会被自动 clamp 到有效范围，不会报错。

### 5. EnumOption 只接受预定义值

设置无效的枚举值会被忽略，保持原值不变。

### 6. FloatOption 使用 epsilon 比较

使用 epsilon 进行浮点数比较，微小差异视为相等，`isDefault()` 可能返回 true。

### 7. 资源包优先级顺序

MC 资源加载顺序：先加载低优先级，后加载高优先级。高优先级资源会覆盖低优先级同名资源。`getSortedEntries()` 返回高优先级在前（用于显示），实际加载应该反过来遍历。

### 8. 自动保存需要手动启用

修改设置不会自动保存到文件。需要调用 `enableAutoSave(path)` 启用自动保存，或手动调用 `saveSettings(path)`。

### 9. 音频设置通过 SoundCategory 映射

使用 `getVolumeForCategory(SoundCategory)` / `setVolumeForCategory(SoundCategory, volume)` 访问音量，而不是直接访问成员变量。这样便于音频系统通过枚举统一访问。

### 10. 默认值来自 DefaultValues.hpp

所有默认值定义在 `common/core/DefaultValues.hpp` 的 `defaults::client` 和 `defaults::server` 命名空间中，便于统一管理和修改。添加新设置时应先在 DefaultValues.hpp 中定义默认值。
