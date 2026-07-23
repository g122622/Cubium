# Network Backend 模块

IR ↔ wire 后端。把协议无关的 ir::IrPacket 编解码成具体后端的 wire 字节，是"LLVM IR"架构里的后端。

## 目录结构

```
src/common/network/backend/
├── IProtocolBackend.hpp        # 后端接口（模板 IProtocolBackend<B>）：name/supportedProtocolVersions/provideProtocolTables
├── java/
│   ├── JavaBackend.hpp/cpp     # name="java",supported={774},provideProtocolTables 委托 JavaProtocolTables
│   └── JavaProtocolTables.hpp/cpp  # 5 阶段包表构建器（addPacket 顺序对齐 GameProtocols.java；Phase3 填充）
└── bedrock/
    └── BedrockBackend.hpp      # stub：name="bedrock",supported={685..898},provideProtocolTables 返回空表
```

## 内部模块关系

- `IProtocolBackend<B>` 是后端接口；`provideProtocolTables()` 返回 `ProtocolTableSet<B>` 注入 Connection。
- `JavaBackend` 委托 `JavaProtocolTables::build()` 构建包表；`BedrockBackend` 是 stub 返回空表。
- 后端只产出 ProtocolTableSet（包表 + codec），不碰传输；传输由 transport/ 层负责。

## 上下游外部依赖关系

- **上游依赖**：`common/core/Result`、`common/network/ir/IrPacket`、`common/network/pipeline/ProtocolTableSet`、`common/network/buffer/RegistryByteBuf`（Java）、`common/network/codec/{StreamCodec,StreamCodecs,ProtocolInfoBuilder}`（Phase3 codec）。
- **下游**：`server/network/ServerNetwork` 与 `client/network/ClientNetwork` 创建后端、取包表注入 Connection。

## 容易踩的坑

1. **addPacket 顺序 = wire packet id**：JavaProtocolTables 的 addPacket 顺序必须严格对齐 GameProtocols.java，顺序错位会导致包 id 漂移、协议不兼容。对照 Java 源码逐包核对。
2. **基岩后端是 stub**：provideProtocolTables 返回空表（所有 ProtocolInfo nullptr），别接入真实流量；supportedProtocolVersions 仅常量预留。
3. **后端只管 codec 不管传输**：后端产出 ProtocolTableSet（包表），传输（TcpTransport/RakNetTransport/LocalTransport）由 Connection/上层选；别在后端里 new 传输。
4. **B 模板参数绑定缓冲类型**：JavaBackend 用 `IProtocolBackend<RegistryByteBuf>`；B 决定 codec 的缓冲类型，与 Connection<B> 须一致。
5. **JavaBackend.cpp 须与 JavaProtocolTables.cpp 分离**：JavaBackend 委托构建，避免循环；Phase3 各阶段 codec 在 backend/java/codecs/ 子目录按阶段组织。
