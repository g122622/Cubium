# Network Pipeline 模块

Connection 状态机门面 + 编解码 pipeline。游戏逻辑经 Connection 收发 IR 包，与 codec/传输解耦。

## 目录结构

```
src/common/network/pipeline/
├── ProtocolTableSet.hpp   # 五阶段包表集合（5 阶段 × 2 流向 = 10 张 ProtocolInfo）+ TerminalCheck
└── Connection.hpp/Connection.inl  # 统一门面（模板 Connection<B>）：持有 ITransport/ILocalTransport + 当前阶段包表 + 监听器；双模式 wire/local；terminal 包驱动阶段切换
```

## 内部模块关系

- `ProtocolTableSet<B>` 集中持有 10 张 `ProtocolInfo<B, 阶段Variant>`，由后端（JavaProtocolTables）构建注入。
- `Connection<B>` 是门面：Wire 模式经 ITransport 收发字节（encode→pipeline→transport / transport→pipeline→decode），Local 模式经 ILocalTransport 直传 IrPacket。
- Connection.inl 是 `Connection<B>` 的模板实现（模板类实现分离到 .inl 由头文件末尾 include）。

## 上下游外部依赖关系

- **上游依赖**：`common/core/Result`、`common/network/ir/IrPacket`、`common/network/protocol/{ConnectionProtocol,PacketFlow,ProtocolInfo}`、`common/network/transport/{ITransport,LocalTransport}`。
- **下游**：`server/network/ServerNetwork` 与 `client/network/ClientNetwork` 创建 Connection；后端（JavaBackend）提供 ProtocolTableSet 注入 Connection。

## 容易踩的坑

1. **Connection 按缓冲类型 B 模板化**：Java 后端用 `Connection<RegistryByteBuf>`；B 须支持 bytes()/read*/write* 等（满足 buffer::ByteBuf 接口）。
2. **Wire 模式 send 按 (phase, flow) 选 ProtocolInfo**：用 `std::get<阶段Variant>(packet.packet)` 取阶段变体再 encode；phase/flow 必须与 IrPacket.packet 实际持有的阶段变体一致，否则 std::get 抛 bad_variant_access（Phase3 接入后注意）。
3. **Local 模式 send 零序列化**：直接 move IrPacket 到对端 inbox，不经 codec；Local 模式下 ProtocolTableSet 仅用于阶段校验，不参与编解码。
4. **Wire 模式接收靠 ITransport.onMessage 驱动**：异步回调 `_handleWireBytes`；Local 模式接收靠对端 pump 驱动 `ILocalTransport.onPacket`。两种模式驱动方式不同，别混用。
5. **terminal 包驱动 setPhase**：当前骨架未实现自动阶段切换（Phase3 接入 codec 后补 ProtocolSwapHandler）；手动调 `setPhase` 切阶段。
6. **Connection.inl 须被 Connection.hpp 末尾 include**：模板实现分离文件，使用方只 include .hpp，.inl 由 .hpp 末尾引入，不能单独编译。
7. **骨架阶段 _handleWireBytes 是空实现**：Phase1 不接真实 codec，收到字节暂不 decode；Phase3 接入 Java codec 后补全 decode + 阶段切换。
