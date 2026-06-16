# 声音实例定义

定义各类声音实例，继承自 `ISoundInstance` / `SoundInstance` / `TickableSound`。

## 目录结构树

```text
src/client/sound/instance/
├── ISoundInstance.hpp            # 声音实例接口（音量/音调/位置/衰减等）
├── SoundInstance.hpp/cpp         # 基础声音实现 + TickableSound 基类
├── EntitySoundInstance.hpp/cpp   # 跟随实体位置的声音实例
├── ElytraSound.hpp/cpp           # 鞘翅飞行声音
├── MinecartSound.hpp/cpp         # 矿车行驶声音（含骑乘声音）
├── MovingTickableSound.hpp/cpp   # 通用移动声音（可Tick，跟随实体位置）
└── UnderwaterLoopSound.hpp/cpp   # 水下循环声音
```

## 内部模块关系

```
ISoundInstance（接口）
  └── SoundInstance（基础实现）
        └── TickableSound（可Tick声音，tick() = 0 纯虚函数）
              ├── EntitySoundInstance（实体跟随声音）
              ├── MovingTickableSound（通用移动声音）
              ├── ElytraSound（鞘翅飞行声音）
              ├── MinecartSoundStateful（矿车声音，内嵌于 MinecartSound）
              ├── UnderwaterLoopSound（水下循环声音）
              └── [EntitySoundHandler.cpp 内嵌]
                    ├── BeeSoundBase → BeeFlightSoundStateful / BeeAngrySoundStateful
                    ├── GuardianSoundStateful
                    └── ElytraSoundStateful
```

## 上下游外部依赖关系

### 上游依赖

```
client/sound/SoundEngine.hpp          # 播放、停止、音量管理
client/sound/handler/EntitySoundHandler.hpp  # EntitySoundState 状态快照
common/sound/SoundEvents.hpp          # 音效事件 ID 常量
```

### 下游依赖

无（声音实例是终端消费者，由 SoundEngine 调用 tick()）

## 容易踩的坑

- **线程安全**：TickableSound 子类在音频线程中被 tick()，不得直接引用 ClientEntity 等 UI/主线程对象。需要通过 `EntitySoundState` 快照获取实体信息，由 `EntitySoundHandler` 从主线程同步状态
- **声音切换**：蜜蜂等实体的声音切换不在 TickableSound 内部完成（TickableSound 无 SoundEngine 引用），而是由 EntitySoundHandler::tick() 检测 isDone() 后重建声音
- **canBeSilent**：TickableSound 子类通常需要重写 `canBeSilent()` 返回 true，否则音量为 0 时声音会被 SoundEngine 跳过
