# Network IR 模块

协议无关中间表示（"LLVM IR"）。IR 包是 plain struct，字段用 C++ 原生类型，与具体后端 wire 格式解耦。游戏逻辑用 std::visit 消费，零虚函数开销。

## 目录结构

```
src/common/network/ir/
├── IrPacketBase.hpp                 # 公共特征：kTerminal 约定 + BedrockMeta 基岩预留字段（非虚基类）
├── IrPacket.hpp                     # 阶段变体 HandshakePacket/StatusPacket/.../PlayPacket + 顶层 IrPacket{phase,variant}
└── packets/
    ├── handshake/HandshakePackets.hpp      # ClientIntention（terminal）
    ├── status/StatusPackets.hpp            # StatusRequest/Response, PingRequest/Response
    ├── login/LoginPackets.hpp              # Hello/HelloBound/Key/LoginFinished/LoginCompression/LoginAcknowledged/Disconnect
    ├── configuration/ConfigurationPackets.hpp # RegistryData/FinishConfiguration（terminal）
    └── play/PlayPackets.hpp                # KeepAlive/Disconnect/MovePlayerPos/Chat/ChatCommand（在用包子集，Phase3 补全）
```

## 内部模块关系

- `IrPacketBase` 约定 kTerminal 静态标志（terminal 包驱动状态机）与 BedrockMeta 预留字段，包 struct 按需组合进成员。
- `packets/*` 定义各阶段 plain struct，字段为 C++ 原生类型（i32/std::string/f64 等）。
- `IrPacket.hpp` 把每阶段 struct 聚成阶段变体（std::variant），再包成顶层 `IrPacket{phase, variant}`。阶段变体分开避免单 mega-variant 上百备选项。

## 上下游外部依赖关系

- **上游依赖**：`common/core/Types`、`common/network/protocol/ConnectionProtocol`（仅顶层 IrPacket 的 phase 标签）。
- **下游**：`backend/java/codecs/` 为每个 IR struct 写 `StreamCodec<RegistryByteBuf, IrStruct>`；`protocol/ProtocolInfo<B,阶段Variant>` 以阶段变体为分发 Variant；`pipeline/Connection` 收发 IrPacket；游戏逻辑 std::visit 消费。

## 容易踩的坑

1. **IR 包是 plain struct，不继承虚基类**：消费走 std::visit，引入虚函数会破坏零开销前提；公共特征（kTerminal/BedrockMeta）是约定 + 组合成员，非继承。
2. **terminal 包须声明 `static constexpr bool kTerminal = true`**：ProtocolSwapHandler 据此驱动阶段切换；漏声明会被当作普通包，状态机卡死。terminal 包：ClientIntention/Key/LoginFinished/LoginAcknowledged/FinishConfiguration。
3. **BedrockMeta 字段 Java 后端须忽略**：预留的 subclientSender/Target/runtimeBlockId 默认 nullopt，Java codec 读写时跳过；将来基岩后端填充，**不要**在 Java codec 里读写这些字段。
4. **阶段变体备选项顺序 = addPacket 的 altIndex**：altIndex 是 struct 在阶段变体中的下标，**不等于** wire packet id（两者独立登记）。altIndex 允许稀疏跳号（如 `ChatCommand` altIndex=108 而 wire id=6）。新增包 struct 须追加到变体（取末尾下一个下标作 altIndex）并在 `JavaProtocolTables` 登记 `addPacket<wireId, type, altIndex, codec>`，三者对齐。
5. **顶层 IrPacket 不直接参与 codec**：codec 编解码的是阶段变体（HandshakePacket 等），顶层 IrPacket 是 pipeline 层收发与游戏逻辑消费的载体；Connection 按当前阶段选对应阶段变体的 ProtocolInfo。
