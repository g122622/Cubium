# network/outbound - 出站构造与广播

服务端**主动发起**的所有出站包构造与投递。与入站处理（`play/`）正交：这里没有入站状态机，
只有「构造 IR 包 → 按距离/订阅关系投递」。

本目录是 `server/network` **被其他子系统直接 include 的公共出站 ABI**——`server/command/commands/*`
经 `PacketBuilders` 构造命令回包，`server/world/ServerWorld` 经 `MapPacketBuilder` 推送地图。
因此它的头文件**不得**反向依赖 `play/` 或 `session/` 的内部类型。

## 目录结构

```
src/server/network/outbound/
├── README.md
├── PacketBuilders.hpp/cpp         # 纯自由函数：buildPlaySoundIr / buildPermissionLevelChangeIr
│                                  #   / buildCommandsIr / buildLevelParticlesIr
├── DialogPacketBuilder.cpp        # PacketBuilders 声明的对话框 JSON → IR 构造，供握手 fuzz 共用
├── CommandTreeEncoder.hpp         # 命令树快照 → ClientboundCommandsPacket 包体字节（header-only）
├── PlayerBroadcaster.hpp/cpp      # 距离过滤 + 多态广播（声音/粒子/实体事件/世界事件/爆炸/光照）
└── MapPacketBuilder.hpp/cpp       # 脏地图周期推送（pushDirtyMaps）
```

## 内部模块关系

- `PacketBuilders`：**无状态纯函数**，只返回 `IrPacket` / `optional<IrPacket>`，**不发包**，零
  `MinecraftServer` 依赖。是唯一的单包构造点。
- `CommandTreeEncoder`：被 `PacketBuilders::buildCommandsIr` 调用，把 `CommandTreeSnapshot` 编码为
  二进制 `ClientboundCommandsPacket` 包体（`VarInt(nodeCount) + nodes + VarInt(rootIndex)`）。
  自 `common/network/backend/java/codecs/` 迁入——命令树只由服务端下发，客户端只解码不编码。
- `PlayerBroadcaster`：持 `MinecraftServer&`，构造函数在内部完成包构造后按距离过滤投递。
- `MapPacketBuilder`：唯一的 `pushDirtyMaps(ServerWorld&)` 静态方法，遍历在线玩家背包里的脏
  `MapData`，构造 `MapItemData` 并**自行下发**。

> 分工约定：`PacketBuilders` 产包，`PlayerBroadcaster` / `MapPacketBuilder` 产包**并**发包。
> 后两者内部的包构造应尽量复用 `PacketBuilders` 的原语，避免同一编码出现两份实现。

## 上下游外部依赖关系

### 本目录依赖

| 依赖 | 用途 |
|---|---|
| `common/network/ir/IrPacket.hpp`、`ir/packets/play/*` | IR 包定义 |
| `common/command/{CommandTreeSnapshot,CommandNode}.hpp` | 命令树编码输入 |
| `common/network/buffer/ByteBuf.hpp` | `CommandTreeEncoder` 的字节输出 |
| `server/application/MinecraftServer.hpp` | `PlayerBroadcaster` 的距离过滤与投递 |
| `server/core/ConnectionManager.hpp`、`server/world/ServerWorld.hpp` | 投递与地图数据来源 |

### 依赖本目录

| 模块 | 用途 |
|---|---|
| `server/command/commands/{Give,Loot,Op,DeOp,Particle}Command.cpp` | 经 `PacketBuilders` 构造回包 |
| `server/world/ServerWorld.cpp` | `MapPacketBuilder::pushDirtyMaps` |
| `server/application/MinecraftServer.cpp` | 构造并持 `PlayerBroadcaster` |
| `server/network/handshake/LoginFlow.cpp` | 经 `PacketBuilders::buildCommandsIr` 发命令树 |

## 容易踩的坑

### 1. 同一编码只允许一份实现

历史上 `PlaySound` 与 `LevelParticles` 的包构造在 `PacketBuilders` 与 `PlayerBroadcaster` 中各有一份，
两处漂移后极难发现。新增出站包时：先在 `PacketBuilders` 建原语，再由 broadcaster/命令复用它，
不要就地内联构造。

### 2. 签名冲突会让重载悄悄选错

匿名 namespace 内的同名函数不会违反 ODR，但会遮蔽外层同名重载，导致调用点静默走到错误的实现。
`PacketBuilders` 中的函数名必须全局唯一且语义明确。

### 3. `CommandTreeEncoder` 的 `serializeCap` 用数值 id

参数节点的 `serializeCap` 写的是 `VarInt(numericRegistryId)` 而非 Identifier 字符串。数值 id 必须与
客户端注册表一致，错一个数字客户端就会在补全时崩溃或静默丢弃该参数。

### 4. `MapPacketBuilder` 是唯一调用点

`pushDirtyMaps` 只应由 `ServerWorld::tick` 调用。地图脏标记的去重与周期刷新集中在那里，
别处再调一次会打乱刷新节奏。

### 5. 投递过滤不要下沉到 `PacketBuilders`

`PacketBuilders` 保持零 `MinecraftServer` 依赖，才能被命令系统等上游安全复用。距离过滤、订阅关系、
在线判定一律留在 `PlayerBroadcaster` / `MapPacketBuilder` / `ConnectionManager`。

商人报价由 buildMerchantOffersIr 构造，容器打开和内容变化时发送。交易成本是 ItemCost（物品 ID、数量、精确组件谓词），结果是 ItemStack；两者字段顺序及组件格式不同。使用次数、需求、特殊价格和经验均来自当前报价状态，不得重复计算价格后再作为基础成本发送。

对话框直接定义使用严格整数 NBT 转换；命名资源解析在 handshake/RegistryDataBuilder 完成，PacketBuilders 仅接收已解析的 JSON 构造直接 holder。
对话框实现单独编译进服务端、客户端内置服务端、单元测试、benchmark 和 mc_fuzz_net；拆分构造器时必须同步各目标的源码清单，不要让握手 fuzz 引入含命令系统依赖的整个 PacketBuilders.cpp。
