#音效系统公共模块

本模块定义了音效系统在客户端和服务端共享的基础类型、音效事件常量和唱片机歌曲系统。

##目录结构

```text src / common / sound /
├── SoundCategory.hpp / cpp #声音类别枚举（Master / Music / Blocks等）
├── SoundEvent.hpp / cpp #声音事件定义（资源位置 + 衰减距离）
├── SoundEvents.hpp / cpp #1000 +
    MC 1.16.5 音效事件常量
├── SoundTypes.hpp #基础类型定义（SoundType / AttenuationType等）
├── jukebox / #唱片机歌曲子系统
│   ├── JukeboxSong.hpp / cpp #歌曲数据定义（声音事件、时长、比较器输出）
│   ├── JukeboxSongs.hpp / cpp #原版歌曲注册表（21首歌曲）
│   ├── JukeboxSongPlayer.hpp / cpp #歌曲播放器（播放 / 停止 / 自动停止 / 粒子）
│   └── README.md #唱片机子系统文档
└── network /
    └── SoundPackets.hpp / cpp #音效网络数据包（PlaySound / StopSound / MovingSound /
        WorldEvent）
```

        ##内部模块关系

``` SoundTypes.hpp(基础类型)
      ↓ SoundCategory.hpp(类别枚举)
      ↓ SoundEvent.hpp(事件定义，依赖 ResourceLocation)
      ↓ SoundEvents.hpp(事件常量定义)
      ↓ jukebox
        / JukeboxSong.hpp(歌曲数据，依赖 ResourceLocation)
      ↓ jukebox / JukeboxSongs.hpp(注册表，依赖 SoundEvents)
      ↓ jukebox / JukeboxSongPlayer.hpp(播放器，依赖 JukeboxSong + IWorld)
      ↓ network /
        SoundPackets.hpp(网络数据包，依赖上述所有)
```

        ##上下游外部依赖关系

            ** 本模块依赖：* *
        - `common / core / Types.hpp` -
    基础类型定义 - `common / core / Result.hpp` - 结果类型 - `common / resource / ResourceLocation.hpp` - 资源位置标识
    - `common / network / packet / Packet.hpp` - 数据包基类 - `common / world / block / BlockPos.hpp` -
    方块位置

            ** 依赖本模块的地方：* *
        -服务端 `ServerWorld::playSound()` -
    播放音效 - 客户端 `SoundEngine` - 音效播放引擎 - 方块实现（门、压力板、发射器等） - 触发音效事件
    - 实体实现（玩家、生物等） - 触发音效事件 - `WorldEventPacket` 广播 - 世界事件（音效 + 粒子） - `JukeboxEntity` -
    唱片机方块实体（通过 JukeboxSongPlayer 播放 /
        停止歌曲）

        ##容易踩的坑

        ## #1. 音效需要客户端基础设施

        某些音效（如 `BLOCK_PORTAL_AMBIENT`、`BLOCK_CAMPFIRE_CRACKLE`）需要 `animateTick` 方法，这是 MC
        客户端专用方法，当前项目尚未实现。服务端应通过 `WorldEventPacket` 广播事件给客户端。

        ## #2. 音效事件常量是 ResourceLocation

`SoundEvents` 命名空间中的常量是 `ResourceLocation` 类型，不是 `SoundEvent` 类型。使用时直接传递给 `playSound()` 等方法即可，无需额外包装。

        ## #3. WorldEventPacket 与 PlaySoundPacket 的选择

    - `PlaySoundPacket`：播放任意位置的音效，指定音量和音调 - `WorldEventPacket`：播放预定义的世界事件（音效
    + 粒子），通过事件ID标识，带宽更小 - `MovingSoundPacket`：播放跟随实体的音效（如闪电、守卫者激光）

    ## #4. SoundCategory 的命名与 MC 原版略有不同

    MC
    原版使用 `block`、`player` 等小写名称，本模块使用 `Blocks`、`Players` 等首字母大写形式。`getSoundCategoryName()` 返回的是原版小写格式。

    ## #5. JukeboxSongs 必须在 SoundEvents 之后初始化

`JukeboxSongs::
        initialize()` 使用 `SoundEvents` 命名空间中的常量来注册歌曲，因此必须在程序启动时、`SoundEvents` 可用之后调用。当前在 `Items::
            initialize()` 之后调用。

    ## #6. 比较器输出信号值可能重复

    多个唱片可能共享相同的比较器输出值（如 `otherside` 和 `relic` 都是 14）。`JukeboxSongs::
        getSongByComparatorOutput()` 返回第一个匹配的歌曲，因此不能用于唯一标识唱片。应使用 `getSongBySoundEvent()` 通过声音事件ID唯一标识。
