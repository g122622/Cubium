# 唱片机歌曲系统 (Jukebox Song System)

唱片机歌曲注册和播放管理，提供歌曲元数据、自动停止和粒子效果。

## 目录结构

```
jukebox/
├── JukeboxSong.hpp/cpp           # 唱片机歌曲定义（声音事件、时长、比较器输出）
├── JukeboxSongs.hpp/cpp          # 原版歌曲注册表（21首歌曲）
├── JukeboxSongPlayer.hpp/cpp     # 歌曲播放器（播放/停止/tick/自动停止/粒子）
└── README.md
```

## 核心类

### JukeboxSong
歌曲数据定义，存储：
- `m_soundEventId` - 声音事件ID（如 "minecraft:music_disc.13"）
- `m_descriptionKey` - 描述翻译键（如 "jukebox_song.minecraft.13"）
- `m_lengthInSeconds` - 歌曲长度（秒）
- `m_comparatorOutput` - 比较器输出信号强度 (1-15)

关键方法：
- `lengthInTicks()` - 将秒转换为tick（ceil(length * 20)）
- `hasFinished(ticksSinceSongStarted)` - 判断歌曲是否播放完毕（>= lengthInTicks + 20）

### JukeboxSongs
原版歌曲注册表，在 `initialize()` 中注册21首歌曲。支持按ID、比较器输出或声音事件查找歌曲。

### JukeboxSongPlayer
歌曲播放状态机，管理播放/停止/自动停止逻辑：
- `play()` - 开始播放，广播 `PLAY_RECORD_SOUND` 事件
- `stop()` - 停止播放，广播 `STOP_RECORD_SOUND` 事件
- `tick()` - 每tick调用，检测自动停止，每20tick生成音符粒子
- `setSongWithoutPlaying()` - 从存档恢复播放状态

## 上下游依赖

### 上游依赖（谁使用了这个模块）
- `world/blockentity/interactive/JukeboxEntity` - 唱片机方块实体，持有 JukeboxSongPlayer 实例

### 下游依赖（这个模块依赖了谁）
- `common/resource/ResourceLocation` - 资源位置标识
- `common/sound/SoundEvents` - 声音事件常量
- `common/world/IWorld` - 世界接口（playEvent、addParticle）
- `common/world/WorldEvents` - 世界事件常量

## 与 MC 原版的对应关系

| 本项目 | MC 1.21.11 |
|--------|------------|
| `JukeboxSong` | `net.minecraft.world.item.JukeboxSong` |
| `JukeboxSongs` | `net.minecraft.world.item.JukeboxSongs` |
| `JukeboxSongPlayer` | `net.minecraft.world.item.JukeboxSongPlayer` |

## 设计差异

1. **注册表**：MC 使用动态注册表系统（Registry/Holder），本项目使用静态 `JukeboxSongs` 类。
2. **数据组件**：MC 1.21+ 使用 `DataComponents.JUKEBOX_PLAYABLE` 附加到物品上关联歌曲，本项目通过 `MusicDiscItem::getSoundEventId()` + `JukeboxSongs::getSongBySoundEvent()` 查找。
3. **游戏事件**：MC 的 `GameEvent.JUKEBOX_PLAY/JUKEBOX_STOP_PLAY` 在本项目中标记为 TODO，待游戏事件系统实现后补充。
4. **播放事件**：MC 使用歌曲注册表ID作为 `levelEvent(1010, pos, id)` 的 data 参数，本项目使用比较器输出信号强度（兼容旧版协议）。
