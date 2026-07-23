# Network Pipeline 模块

Connection 状态机门面 + 编解码 pipeline。游戏逻辑经 Connection 收发 IR 包，与 codec/传输解耦。

## 目录结构

```
src/common/network/pipeline/
├── ProtocolTableSet.hpp        # 五阶段包表集合（5 阶段 × 2 流向 = 10 张 ProtocolInfo）+ TerminalCheck
├── Connection.hpp/Connection.inl  # 统一门面（模板 Connection<B>）：双模式 wire/local；encode→压缩→帧→加密→transport；terminal 包驱动阶段切换
├── VarintFraming.hpp/.cpp      # Java VarInt21 长度前缀帧编解码（帧层）
├── CompressionHandlers.hpp/.cpp  # 压缩层 handler（CompressionEncoder/Decoder，zlib + threshold）
├── CipherHandlers.hpp/.cpp     # 加密层 handler（CipherEncoder/Decoder，AES-CFB8）
└── ProtocolSwapHandler.hpp/.cpp  # terminal 包判定 + 阶段切换（Handshaking→Login→Configuration→Play）
```

## pipeline 顺序（对齐 Java 出站 compress→frame→encrypt）

```
出站: ir → ProtocolInfo.encode → [packetID+payload]
       → CompressionEncoder → [VarInt(数据长度)+data]   (threshold<0 跳过)
       → VarintFraming       → [VarInt(帧长)+前述]
       → CipherEncoder       → AES-CFB8 密文            (离线模式跳过)
       → ITransport.send     → TCP 原始字节
入站: ITransport.onBytes → CipherDecoder → VarintFraming(切帧) → CompressionDecoder
       → ProtocolInfo.decode → ir → 监听器 → ProtocolSwapHandler(terminal 切阶段)
```

## 内部模块关系

- `ProtocolTableSet<B>` 集中持有 10 张 `ProtocolInfo<B, 阶段Variant>`，由后端（JavaProtocolTables）构建注入。
- `Connection<B>` 是门面：Wire 模式经 ITransport 收发字节（encode→pipeline→transport / transport→pipeline→decode），Local 模式经 ILocalTransport 直传 IrPacket。
- `VarintFraming` 属 Java wire 格式（不在通用 ITransport 里——LocalTransport 零拷贝、RakNet 自有分帧都不需要它）。
- `CompressionHandlers` 包装 `crypto/ZlibCodec`，按包压缩/解压；threshold<0 时 Connection 不装本层。
- `CipherHandlers` 包装 `crypto/AesCfb8`，流式加解密跨包保持状态；离线模式不装本层。
- `ProtocolSwapHandler` 用 `IsTerminal` 特征（struct 含 `static constexpr bool kTerminal`）判定 terminal 包，给出下一阶段。
- Connection.inl 是 `Connection<B>` 的模板实现（模板类实现分离到 .inl 由头文件末尾 include）。

## 上下游外部依赖关系

- **上游依赖**：`common/core/Result`、`common/network/ir/IrPacket`、`common/network/protocol/{ConnectionProtocol,PacketFlow,ProtocolInfo}`、`common/network/transport/{ITransport,LocalTransport}`、`common/network/crypto/{AesCfb8,Crypt,ZlibCodec}`。
- **下游**：`server/network/ServerNetwork` 与 `client/network/ClientNetwork` 创建 Connection；后端（JavaBackend）提供 ProtocolTableSet 注入 Connection。

## 容易踩的坑

1. **Connection 按缓冲类型 B 模板化**：Java 后端用 `Connection<RegistryByteBuf>`；B 须支持 bytes()/read*/write* + `(const u8*, usize)` 构造（满足 buffer::ByteBuf 接口）。
2. **Wire 模式 send 按 (phase, flow) 选 ProtocolInfo**：用 `std::get<阶段Variant>(packet.packet)` 取阶段变体再 encode；phase/flow 必须与 IrPacket.packet 实际持有的阶段变体一致，否则 std::get 抛 bad_variant_access。
3. **Local 模式 send 零序列化**：直接 move IrPacket 到对端 inbox，不经 codec/压缩/加密；Local 模式下 ProtocolTableSet 仅用于阶段校验。
4. **入站双缓冲分离**：`m_encryptedIn`（喂 CipherDecoder 的原始字节）与 `m_plainIn`（解密后喂 VarintFraming 的明文）必须分离——CFB8 流式要求加密字节按到达顺序逐字节喂入，切帧后的明文残留不能与下次加密字节混存（否则二次解密损坏数据）。
5. **terminal 包驱动 setPhase**：send 侧发出 terminal 包后切阶段（对齐 Java 出站 setupOutboundProtocol）；recv 侧收到 terminal 包回调监听器后切阶段。
6. **加密/压缩层按需装入**：`setupCompression(threshold)` 在收到 LoginCompression 后调；`setupEncryption(secret)` 在登录握手双方确认共享密钥后调。离线模式两者都不装（明文+不压缩 wire）。
7. **Connection.inl 须被 Connection.hpp 末尾 include**：模板实现分离文件，使用方只 include .hpp，.inl 由 .hpp 末尾引入，不能单独编译。
