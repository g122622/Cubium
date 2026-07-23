# Network Protocol 模块

协议类型标识与阶段包表注册。协议无关——描述"有哪些阶段、每阶段每流向有哪些包"，与具体后端 codec 解耦。

## 目录结构

```
src/common/network/protocol/
├── PacketFlow.hpp           # enum{Serverbound,Clientbound} + isServerbound/isClientbound
├── ConnectionProtocol.hpp   # enum{Handshaking,Status,Login,Configuration,Play}（Java 1.21.11 五阶段）
├── PacketType.hpp           # struct{flow,id字符串} + operator== + PacketTypeHash
├── ProtocolInfo.hpp         # 一个(阶段,流向)包表：持有 IdDispatchCodec<B,Variant>，提供 encode/decode
└── ProtocolInfoBuilder.hpp  # 链式 addPacket(type,altIndex,codec)...build()（addPacket 顺序=packet id）
```

## 内部模块关系

- `PacketFlow`/`ConnectionProtocol` 是纯枚举，无依赖。
- `PacketType` 是逻辑标识，整数 id 隐式（由 `IdDispatchCodec` 注册顺序分配），用 `PacketTypeHash` 入 unordered_map。
- `ProtocolInfo<B,Variant>` 持有 `IdDispatchCodec<B,Variant>`，是 Connection 在某阶段某流向的编解码入口。
- `ProtocolInfoBuilder` 链式构建 `ProtocolInfo`，每包登记 matches/encodePayload/decode 闭包，codec 用 shared_ptr 在编/解码闭包间共享。

## 上下游外部依赖关系

- **上游依赖**：`common/core/Result`、`common/core/Types`、`common/network/codec/IdDispatchCodec`、`common/network/codec/StreamCodec`。
- **下游**：`backend/java/JavaProtocolTables` 用 `ProtocolInfoBuilder` 构建 5 阶段 × 2 流向共 10 张包表；`pipeline/Connection` 持有当前阶段的 `ProtocolInfo` 做 codec 交换。

## 容易踩的坑

1. **整数 packet id 不在 PacketType 里硬编码**：`PacketType.id` 是逻辑名（如 "keep_alive"），真正的 wire id 由 `ProtocolInfoBuilder::addPacket` 顺序决定（0 起递增）。改 addPacket 顺序 = 改 wire id，破坏网络兼容。
2. **addPacket 的 altIndex 必须与 Variant 备选项下标一致**：`matches` 用 `value.index()==altIndex` 判定，`std::get_if<PacketStruct>` 取值；altIndex 给错会静默匹配错包。Java 后端构建表时 altIndex 须与 IR 变体定义顺序对齐。
3. **ProtocolInfo 按 Variant 模板化**：每阶段 Variant 不同（HandshakePacket/PlayPacket 等），不能用单一 Variant 类型跨阶段；Connection 切阶段时连 Variant 类型一起换。
4. **codec 在编/解码闭包间共享**：`ProtocolInfoBuilder::addPacket` 把 codec 存 shared_ptr，encode/decode 闭包各持一份；不要把 codec move 进单个闭包导致另一侧悬空。
5. **10 张包表**：5 阶段 × {Serverbound, Clientbound} = 10 张表，Java 后端须全部构建（哪怕某表为空，如 Handshaking-Clientbound 通常空）。
