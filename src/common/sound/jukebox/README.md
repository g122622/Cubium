# 唱片机歌曲

## 目录结构树

```text
jukebox/
├── JukeboxSong.hpp/cpp  # 歌曲声音、时长与比较器输出
├── JukeboxSongs.hpp/cpp  # 歌曲查询与同步注册顺序
└── JukeboxSongPlayer.hpp/cpp  # 播放状态、停止及周期事件
```

## 内部模块关系

JukeboxSongs 持有歌曲定义；registryKeys 给出登录注册表和播放事件共用的编号顺序。JukeboxSongPlayer 引用歌曲并推进播放时间，方块实体负责物品和状态。

## 上下游外部依赖关系

依赖 SoundEvents、IWorld 与 LevelEventId；JukeboxEntity、唱片物品及登录 RegistryDataBuilder 消费本模块。

## 容易踩的坑

- 播放事件 1010 的 data 是歌曲注册编号，第一首歌编号为 0；停止使用独立事件 1011。
- 比较器输出可以重复，不能用它作为歌曲编号或网络播放标识。
- 初始化完成后歌曲地址与同步顺序必须稳定。
- 停止先触发模拟 GameEvent，再发送客户端停止效果。
