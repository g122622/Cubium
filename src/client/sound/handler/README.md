# 环境音效处理器

客户端环境音效处理器，由 `SoundEngine` 每 tick 调度。所有处理器实现 `IAmbientSoundHandler` 接口。

## 目录结构树

```text
src/client/sound/handler/
├── IAmbientSoundHandler.hpp            # 环境音处理器接口（tick 方法）
├── BiomeAmbientHandler.hpp/cpp         # 群系环境音（心境音、循环音）
├── UnderwaterAmbientHandler.hpp/cpp     # 水下环境音
├── BubbleColumnAmbientHandler.hpp/cpp   # 气泡柱音效
├── EntitySoundHandler.hpp/cpp           # 实体音效（蜜蜂、守卫者、鞘翅、矿车）
└── WeatherSoundHandler.hpp/cpp          # 天气音效（雨声、雷声）
```

## 内部模块关系

```
AudioService（主线程入口）
    │
    └── 命令队列 ──→ SoundEngine::tick()
                        │
                        ├── BiomeAmbientHandler::tick()
                        ├── UnderwaterAmbientHandler::tick()
                        ├── BubbleColumnAmbientHandler::tick()
                        ├── EntitySoundHandler::tick()
                        └── WeatherSoundHandler::tick()
```

## 上下游外部依赖关系

### 上游依赖

```
AudioService                          # 状态更新入口（主线程 → 命令队列 → 音频线程）
client/sound/SoundEngine.hpp          # 声音播放引擎
common/sound/SoundEvents.hpp          # 音效事件 ID 定义
```

### 下游依赖

无（处理器是终端消费者）

## 容易踩的坑

- **线程安全**：处理器只在音频线程中执行 `tick()`，状态更新由 `AudioService` 通过命令队列传递，不要在主线程直接修改处理器状态。
- **WeatherSoundHandler 雨声类型判断**：使用 `canSeeSky`（天空光照判断）区分 RAIN 和 RAIN_ABOVE。canSeeSky=false 表示玩家在遮挡物下方（屋顶、洞穴等），此时播放闷雨声 RAIN_ABOVE（音量 0.1、音调 0.5）。雷声只在 canSeeSky=true 时播放。
- **EntitySoundHandler 跨线程**：不直接引用 ClientEntity，通过 `EntitySoundState` 状态快照获取实体信息。
- **蜜蜂声音切换**：由 `BeeSoundBase`（内嵌于 EntitySoundHandler.cpp）实现。愤怒状态变化时 `markDone()` 标记当前声音完成，`EntitySoundHandler::tick()` 同帧检测并重建对应类型声音，实现无缝切换。
