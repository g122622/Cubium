# 音效系统公共模块

本模块定义了音效系统在客户端和服务端共享的基础类型和音效事件常量。

## 目录结构

```text
src/common/sound/
├── SoundCategory.hpp/cpp    # 声音类别枚举（Master/Music/Blocks等）
├── SoundEvent.hpp/cpp       # 声音事件定义（资源位置+衰减距离）
├── SoundEvents.hpp/cpp      # 1000+ MC 1.16.5 音效事件常量
├── SoundTypes.hpp           # 基础类型定义（SoundType/AttenuationType等）
└── network/
    └── SoundPackets.hpp/cpp # 音效网络数据包（PlaySound/StopSound/MovingSound/WorldEvent）
```

## 内部模块关系

```
SoundTypes.hpp (基础类型)
      ↓
SoundCategory.hpp (类别枚举)
      ↓
SoundEvent.hpp (事件定义，依赖 ResourceLocation)
      ↓
SoundEvents.hpp (事件常量定义)
      ↓
network/SoundPackets.hpp (网络数据包，依赖上述所有)
```

## 上下游外部依赖关系

**本模块依赖：**
- `common/core/Types.hpp` - 基础类型定义
- `common/core/Result.hpp` - 结果类型
- `common/resource/ResourceLocation.hpp` - 资源位置标识
- `common/network/packet/Packet.hpp` - 数据包基类
- `common/world/block/BlockPos.hpp` - 方块位置

**依赖本模块的地方：**
- 服务端 `ServerWorld::playSound()` - 播放音效
- 客户端 `SoundEngine` - 音效播放引擎
- 方块实现（门、压力板、发射器等）- 触发音效事件
- 实体实现（玩家、生物等）- 触发音效事件
- `WorldEventPacket` 广播 - 世界事件（音效+粒子）

## 容易踩的坑

### 1. 音效需要客户端基础设施

某些音效（如 `BLOCK_PORTAL_AMBIENT`、`BLOCK_CAMPFIRE_CRACKLE`）需要 `animateTick` 方法，这是 MC 客户端专用方法，当前项目尚未实现。服务端应通过 `WorldEventPacket` 广播事件给客户端。

### 2. 音效事件常量是 ResourceLocation

`SoundEvents` 命名空间中的常量是 `ResourceLocation` 类型，不是 `SoundEvent` 类型。使用时直接传递给 `playSound()` 等方法即可，无需额外包装。

### 3. WorldEventPacket 与 PlaySoundPacket 的选择

- `PlaySoundPacket`：播放任意位置的音效，指定音量和音调
- `WorldEventPacket`：播放预定义的世界事件（音效+粒子），通过事件ID标识，带宽更小
- `MovingSoundPacket`：播放跟随实体的音效（如闪电、守卫者激光）

### 4. SoundCategory 的命名与 MC 原版略有不同

MC 原版使用 `block`、`player` 等小写名称，本模块使用 `Blocks`、`Players` 等首字母大写形式。`getSoundCategoryName()` 返回的是原版小写格式。
