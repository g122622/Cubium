# 音频模块

客户端音频系统。采用"独立音频引擎线程"模型，由 `AudioService` 统一接收主线程命令并在音频线程内驱动 `SoundEngine`、`SoundHandler`、环境音与音乐逻辑。

## 目录结构

```text
src/client/sound/
├── AudioService.hpp/cpp          # 音频线程入口与命令队列
├── SoundEngine.hpp/cpp           # 声音播放核心
├── SoundHandler.hpp/cpp          # sounds.json 解析与声音定义管理
├── SoundLoader.hpp/cpp           # OGG 解码与资源包读取
├── SoundPool.hpp/cpp             # 活动声音实例池
├── MusicPlayer.hpp/cpp           # 背景音乐调度
├── AudioBufferCache.hpp/cpp      # 音频缓冲区缓存
├── StbVorbisImpl.cpp             # stb_vorbis 适配实现
├── backend/                      # 音频后端
│   ├── IAudioBackend.hpp         # 音频后端抽象接口（IAudioSource、IAudioBackend）
│   ├── AudioBuffer.hpp/cpp       # AudioFormat、AudioData、IAudioBuffer 定义
│   └── OpenALBackend.hpp/cpp     # OpenAL 实现（OpenALSource、OpenALBuffer、OpenALBackend）
├── handler/                      # 环境音处理器
│   ├── IAmbientSoundHandler.hpp
│   ├── BiomeAmbientHandler.hpp/cpp
│   ├── UnderwaterAmbientHandler.hpp/cpp
│   ├── BubbleColumnAmbientHandler.hpp/cpp
│   ├── EntitySoundHandler.hpp/cpp
│   └── WeatherSoundHandler.hpp/cpp
├── instance/                     # 声音实例定义
│   ├── ISoundInstance.hpp
│   ├── SoundInstance.hpp/cpp     # 包含 TickableSound 基类
│   ├── ElytraSound.hpp/cpp
│   ├── UnderwaterLoopSound.hpp/cpp
│   └── MinecartSound.hpp/cpp
└── resource/                     # 声音资源注册表
    ├── SoundDefinition.hpp/cpp
    └── SoundRegistry.hpp/cpp
```

## 模块关系

- `ClientApplication` 只持有 `AudioService`，不直接持有 `SoundEngine`
- `AudioService` 在音频线程里独占调用 `SoundEngine`
- `SoundHandler` / `SoundLoader` 直接依赖共享的 `ResourcePackList`
- `SoundEngine` 依赖 `SoundPool`、`MusicPlayer`、`AudioBufferCache` 和 `IAudioBackend`
- 环境音 handler 只修改状态，不在主线程直接碰 OpenAL

## 外部依赖

- `common/resource/ResourcePackList.hpp`
- `common/sound/SoundCategory.hpp`
- `client/settings/ClientSettings.hpp`
- `client/sound/backend/IAudioBackend.hpp`
- OpenAL、glm、stb_vorbis、spdlog

## 整体职责

1. 统一管理客户端所有音频入口
2. 在独立线程内完成音频初始化、加载、播放、停止、暂停和恢复
3. 统一处理音乐、环境音和常规音效
4. 通过命令队列把主线程音频请求异步化，减轻主线程负担

## 输入 / 输出

- 输入：主线程投递的播放/停止/暂停/更新 listener 命令、共享 `ResourcePackList` 中的 `sounds.json` 和 `.ogg`、客户端设置中的音量/音乐开关等配置
- 输出：OpenAL 播放结果、活动声音状态、音乐状态、环境音状态、日志与性能追踪事件

## 容易踩的坑

- **线程安全**：不要在主线程持有 `SoundEngine` 或 OpenAL 资源。`SoundHandler` 会并发读取 `ResourcePackList`，资源包读侧必须保持线程安全
- **初始化顺序**：启动阶段如果额外添加资源包，会触发资源重载回调，注意初始化顺序
- **音频线程专用**：`AudioBufferCache` 和 `SoundPool` 只应在音频线程内使用
- **源计数追踪**：`OpenALBackend` 通过 `m_activeSourceCount`（原子变量）和 `OpenALSource` 析构回调实现动态源计数追踪。`getAvailableSources()` 返回 `m_maxSources - m_activeSourceCount`。移动赋值 `OpenALSource` 时旧源的回调会触发计数递减，务必注意此副作用
- **源数量上限**：`m_maxSources` 从 OpenAL 设备属性 `ALC_MONO_SOURCES + ALC_STEREO_SOURCES` 查询，回退到 `MAX_CONCURRENT_SOUNDS = 256`。源耗尽时 `createSource()` 返回 `ResourceExhausted` 错误
- **群系环境音**：群系需要配置 `BiomeAmbientSounds` 才能播放环境音效，否则使用默认心境音效
- **心境音效光照采样**：心境音效的光照采样在主线程完成（通过 `ClientApplicationAudio`），而非音频线程。主线程每帧随机采样一个位置并查询该位置的光照，然后传递给 `BiomeAmbientHandler`。由于音频线程的随机采样位置（用于声音播放位置）与主线程的光照采样位置使用不同的随机种子，两者可能不一致——这是架构限制，属于已知的近似实现
- **天气音效 canSeeSky 判断**：`WeatherSoundHandler` 通过 `canSeeSky` 参数判断雨声类型：户外（canSeeSky=true）播放 WEATHER_RAIN，遮挡物下方（canSeeSky=false）播放 WEATHER_RAIN_ABOVE。canSeeSky 由主线程通过 `ClientWorld::canSeeSky()` 计算，包含维度检查（仅主世界有天空光照）和天空光照判断（skyLight>=15）。
- **水下音乐生物群系检查**：水下音乐（`MusicType::Underwater`）仅在海洋或河流生物群系中播放，通过 `BiomeTags::IS_OCEAN()` 和 `BiomeTags::IS_RIVER()` 标签判断。判断结果由主线程计算后通过 `AudioService::updateMusicState()` 传递到音频线程
- **菜单状态判断**：菜单状态通过 `ScreenManager::instance().hasScreen()` 判断，需在 UI 更新后调用
- **实体声音跨线程**：TickableSound 子类不直接引用 ClientEntity，而是通过 `EntitySoundState` 状态快照获取实体信息，避免跨线程访问
- **蜜蜂声音切换**：蜜蜂飞行/愤怒声音的切换由 EntitySoundHandler 内嵌的 `BeeSoundBase` 实现，不使用旧版 `BeeSound`（已移除）。切换流程：`BeeSoundBase::tick()` 检测愤怒状态变化 -> `markDone()` -> `EntitySoundHandler::tick()` 检测 isDone() -> 根据当前状态重建声音并立即播放，实现同帧无缝切换
- **高度常量**：使用 `mc::world::SEA_LEVEL` 而非硬编码 63

## 接入原则

- 只通过 `AudioService` 调用音频能力
- 不要在主线程直接调用 `SoundEngine`
- 资源包变更后由 `AudioService::reloadSoundDefinitions()` 重新加载声音定义
