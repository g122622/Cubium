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
- **线程安全**：`m_buffers` 由 `m_bufferMutex` 保护；`OpenALSource`/`OpenALBuffer` 非线程安全，必须在音频线程调用
- **源数量上限**：`MAX_SOURCES = 256` 硬编码，`getAvailableSources()` 未动态追踪实际已用数量
- **流式播放未完成**：`queueBuffers`/`unqueueBuffers` 为占位实现，流式播放逻辑将在 `SoundEngine` 层完成
