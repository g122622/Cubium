# network/sync - 区块线格式（双向共用）

本目录只放**客户端与服务端都要用**的区块线格式翻译层。区块推送的**记账**（谁已收到哪个区块）
属于服务端，已迁至 `server/network/sync/chunk/`。

## 目录结构

```
src/common/network/sync/
├── README.md
├── ChunkSerializer.hpp/cpp   # 项目内部区块二进制格式：serialize/deserialize ChunkData 与 ChunkSection
└── VanillaChunkWire.hpp/cpp  # IR ↔ Java 1.21.11 LevelChunkWithLight 翻译
```

- `ChunkSerializer`：集成服 LocalTransport 直传所用的紧凑二进制格式。服务端序列化、客户端反序列化，
  两端必须字节级一致。
- `VanillaChunkWire`：Java 线格式翻译。`buildLevelChunkWithLightIR` 供服务端发送侧，
  `readLevelChunkWithLightIR` 供客户端接收侧，且 `backend/java/codecs/JavaPlayCodecs` 依赖它。

## 内部模块关系

```
        server/network/sync/ChunkSendManager（发送侧）
                     │
                     ├─ 集成服：ChunkSerializer::serializeChunk ──▶ LocalTransport 直传
                     └─ 远程服：VanillaChunkWire::buildLevelChunkWithLightIR ──▶ Java 客户端
                                                                                    │
        client/world/ClientWorld（接收侧）                                           │
                     ├─ 集成服：ChunkSerializer::deserializeChunk ◀──────────────────┘
                     └─ 远程服：VanillaChunkWire::readLevelChunkWithLightIR
```

两条通道与两种模式正交互补：`ChunkSerializer` 走 Local（同进程零拷贝前的字节化），
`VanillaChunkWire` 走 Wire（真 Java 协议）。

## 上下游外部依赖关系

### 上游依赖（本模块依赖的）

| 模块 | 用途 |
|---|---|
| `common/world/chunk/data/ChunkData.hpp` | `ChunkData` / `ChunkSection` |
| `common/world/chunk/data/{BiomeContainer,Heightmap}.hpp` | 生物群系与高度图 |
| `common/world/chunk/base/ChunkPos.hpp` | 坐标与 flow id |
| `common/util/NibbleArray.hpp` | 光照数据 |
| `common/network/codec/{PacketSerializer,PacketDeserializer}.hpp` | 字节读写原语 |
| `common/network/ir/packets/play/PlayPacketsExtended.hpp` | `LevelChunkWithLight` IR 包 |

### 下游依赖（依赖本模块的）

| 模块 | 用途 |
|---|---|
| `server/network/sync/ChunkSendManager` | 序列化区块后下发 |
| `client/world/ClientWorld` | `deserializeChunk` 接收区块 |
| `common/network/backend/java/codecs/JavaPlayCodecs` | 依赖 `VanillaChunkWire` 做 Java 线格式翻译 |

**注意**：`common` 不得依赖 `server`。服务端的区块推送记账（`ChunkView`/`PlayerChunkTracker`/
`ChunkSyncManager`）依赖本目录，反向不存在。

## 容易踩的坑

### 1. 反序列化会校验坐标

`deserializeChunk(x, z, data)` 会比对包内的坐标并拒绝不匹配的数据。必须传入**序列化时使用的**坐标。

### 2. 区块段位掩码只含非空段

`calculateSectionMask` 只置位非空 `ChunkSection`。空段（全空气）不进位掩码，反序列化时也不会创建。
新增段数据时若忘了同步 `calculateChunkSize` 的镜像逻辑，会出现"预测大小与实际写入不符"。

### 3. 光照数据固定 2048 字节/通道

`ChunkSection` 序列化包含天空光照与方块光照各 2048 字节（4096 方块 / 2，每方块 4 位）。
少读或多读会整体错位。

### 4. 高度图有新旧两块

`serializeChunk` 同时写向后兼容的有损 `u8` 高度图块与包尾的扩展 `i16` 无损块
（存在位掩码 + 每个已初始化 final 类型 256×2 字节）。反序列化优先用扩展块，缺失时才回退有损块
（负 Y / Y>255 会截断）。改动任一侧都要同步对方与 `calculateChunkSize`。

### 5. macOS 的 `BYTE_SIZE` 宏冲突

系统头文件里 `BYTE_SIZE` 是宏，会与 `NibbleArray::BYTE_SIZE` 冲突。`ChunkSerializer.cpp` 用
`#pragma push_macro("BYTE_SIZE")` / `#undef` 屏蔽，改文件头时不要删掉。

### 6. 不要把记账类搬回来

`common` 不得依赖 `server`。区块推送记账的三个类型（`ChunkView`/`PlayerChunkTracker`/
`ChunkSyncManager`）只在服务端使用，属于 `server/network/sync/chunk/`；
本目录保留的只有两端都需要的线格式翻译层。
