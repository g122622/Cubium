# IR 包构造、投递与接收者筛选

## 目录结构树

```text
outbound/
├── AttributeSnapshotBuilder.hpp  # 把生物的当前属性收集成属性同步包负载
├── CommandTreeEncoder.hpp  # 只编码参数节点的 properties 部分（不含类型数值 id，也不含建议提供器）
├── DialogPacketBuilder.cpp  # 模块接口与实现
├── MapPacketBuilder.cpp  # 模块接口与实现
├── MapPacketBuilder.hpp  # 地图数据周期推送构造器（无状态静态类）
├── PacketBuilders.cpp  # 模块接口与实现
├── PacketBuilders.hpp  # 构造当前商人菜单的完整报价包，保留物品组件与价格、经验信息。
├── PacketSender.cpp  # 模块接口与实现
├── PacketSender.hpp  # 连接管理器（新网络层 IR 版本）
├── PlayerBroadcaster.cpp  # 模块接口与实现
└── PlayerBroadcaster.hpp  # 玩家广播门面
```

## 内部模块关系

PacketBuilders 构造 IR 包；PacketSender 提供单播和共享包广播；PlayerBroadcaster 根据世界位置筛选接收者。连接关闭与玩家移除不在本目录执行。

## 上下游外部依赖关系

依赖公共 IR、玩家记录和连接视图；世界绑定、命令、记分板、首领条及游戏模式服务使用投递入口。

## 容易踩的坑

- PacketSender 只投递包，PlayerConnectionManager 关闭连接，PlayerManager 统一移除记录。
- 玩家移除前释放菜单、实体和区块资源，移除后通知剩余玩家；重复移除不得重复通知。
- 玩家移动与区块订阅分开，订阅统一使用 ChunkSyncManager 持有的 tracker。

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

### 共享广播快照

接收者筛选和距离计算在主线程完成；相同内容冻结为一个 OutboundPacket，通过 sendShared 投递。
相同协议表、流向和压缩阈值复用加密前帧；含逐玩家位置或击退向量的包仍按玩家构造。

### 命令图编号与引用

重定向在命令节点中使用弱引用，避免子节点指回父节点形成所有权环。快照保留每个节点的执行修改器信息；编码器只合并网络上相等的无回调字面量与 Brigadier 基础参数，再按 BFS 重编号。建议提供器必须携带实际注册名，不能统一替换为 ask_server。
