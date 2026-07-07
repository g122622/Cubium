# 音频后端

OpenAL 音频后端抽象层，提供平台无关的音频源/缓冲区/听者接口。

## 目录结构

```text
src/client/sound/backend/
├── IAudioBackend.hpp     # 音频后端抽象接口（IAudioSource、IAudioBackend）
├── AudioBuffer.hpp/cpp   # AudioFormat、AudioData、IAudioBuffer 定义
└── OpenALBackend.hpp/cpp # OpenAL 实现（OpenALSource、OpenALBuffer、OpenALBackend）
```

## 模块关系

- `OpenALBackend` 实现 `IAudioBackend` 接口，管理 OpenAL device/context 生命周期
- `OpenALSource` 实现 `IAudioSource`，封装 AL source 的播放/暂停/停止/空间属性
- `OpenALBuffer` 实现 `IAudioBuffer`，封装 AL buffer 的创建/销毁/格式映射
- `SoundEngine` 通过 `IAudioBackend` 接口操作后端，不直接依赖 OpenAL
- `AudioBufferCache` 缓存 `IAudioBuffer` 弱引用，避免重复解码
- `SoundLoader` 产出 `AudioData`，由 `OpenALBackend::createBuffer()` 上传至 OpenAL

## 外部依赖

- `common/sound/SoundTypes.hpp`（AudioBufferId、AudioSourceId 等类型）
- OpenAL Soft（`AL/al.h`、`AL/alc.h`）

## 容易踩的坑

- **格式限制**：`OpenALBuffer::getALFormat()` 仅支持 mono/stereo × 8bit/16bit 四种 PCM 格式。stb_vorbis 解码恒为 16-bit，因此运行时只会产生 `AL_FORMAT_MONO16` 或 `AL_FORMAT_STEREO16`
- **线程安全**：`m_buffers` / `m_alBufferToId` 由 `m_bufferMutex` 保护；`OpenALSource`/`OpenALBuffer` 非线程安全，必须在音频线程调用；`m_activeSourceCount` 使用 `std::atomic`，可跨线程安全读取
- **源数量追踪**：`m_activeSourceCount` 通过 `OpenALSource` 析构回调自动递减，`getAvailableSources()` 返回 `m_maxSources - m_activeSourceCount`。当源耗尽时 `createSource()` 返回 `ResourceExhausted` 错误
- **源数量上限**：`m_maxSources` 从设备属性 `ALC_MONO_SOURCES + ALC_STEREO_SOURCES` 查询，回退到 `MAX_CONCURRENT_SOURCES = 256`
- **流式播放 ID 映射**：`OpenALSource::queueBuffers` 接收应用层 `AudioBufferId`，需通过 `OpenALBackend::_lookupALBuffer` 翻译为 OpenAL `ALuint` 句柄；`unqueueBuffers` 反向通过 `_lookupBufferId` 翻译。两个回调在 `createSource()` 时绑定，OpenALSource 不直接持有 backend 指针，保持接口抽象。`createBuffer` / `destroyBuffer` 必须同步维护 `m_buffers`（正向）与 `m_alBufferToId`（反向）两张表的一致性
- **unqueueBuffers 出队数量**：使用 `alGetSourcei(AL_BUFFERS_PROCESSED)` 查询实际可出队数量，取 `min(processed, count)`，避免在 `processed < count` 时触发 `AL_INVALID_VALUE`
- **流式播放上层未集成**：`OpenALBackend` 层的 `queueBuffers` / `unqueueBuffers` 原语已可用，但 `SoundEngine` 尚未实现基于 `ActiveChannel` 的流式调度（分块解码、续灌、循环）。流式播放端到端打通需要：在 `SoundLoader` 增加分块解码接口、在 `SoundEngine::ActiveChannel` 增加 `OggStream` 与缓冲区池字段、在 `SoundEngine::play` 按 `SoundDefinition::stream` 分流、在 `SoundEngine::tick` 中续灌
