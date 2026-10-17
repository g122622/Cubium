# 网络连接流水线

## 目录结构树

```text
pipeline/
├── Connection.hpp/inl        # 连接门面与共享协议状态，支持 Wire/Local
├── OutboundPacket.hpp/cpp    # 不可变出站快照及加密前帧缓存
├── ProtocolTableSet.hpp      # 五阶段双向包表
├── VarintFraming.hpp/cpp     # VarInt 长度帧
├── CompressionHandlers.hpp/cpp # 包级压缩与解压
├── CipherHandlers.hpp/cpp    # 连接级流式加解密
└── ProtocolSwapHandler.hpp/cpp # 终端包对应的阶段切换
```

## 内部模块关系

Wire 出站由连接执行器串行执行：IR 编码、压缩、组帧、加密、异步写入。
入站在同一执行器解密、切帧、解压和解码，业务监听器只向主线程投递 IR。
Local 模式直接投递 IR，由主线程 pump。

`OutboundPacket` 冻结共享 NBT；同次广播或区块下推可以共享快照。
加密前的帧按协议表身份、方向和压缩阈值缓存，流式加密状态保留在各连接内。
`Connection` 的异步任务持有共享协议状态，字节回调使用弱引用，避免与传输形成所有权环。

## 上下游外部依赖关系

本层依赖 IR、协议表、缓冲区、压缩加密原语和 ITransport；Java 后端提供包表与 codec。
服务端 ServerClientConnection、客户端 ClientNetwork 和协议模糊测试通过 Connection 收发。

## 容易踩的坑

- Wire 的 send/setupCompression/setupEncryption/阶段切换必须共用连接的有序执行器。
  压缩通知按旧格式发出，后续包才使用新压缩配置；不能在调用线程直接修改 handler。
- 成功入队不等于写入完成；断开原因和状态查询回复发送后应使用 closeAfterFlush。
- 重配置期间的迟到 Play 广播会被丢弃；其他阶段不匹配和编码失败仍会断开连接。
  断开原因包通过 sendDisconnect 在执行器内按实际出站阶段构造，避免与排队的阶段切换竞争。
- 终端包按自身阶段编码，发送后只推进出站阶段；入站阶段由收到的终端包独立推进。
- 入站和出站阶段的查询是原子快照，主线程不应据此读取或修改流水线内部状态。
- CFB8 字节按流顺序处理，密文与切帧后的明文残留必须分开保存。
- 缓存不包含密文，不能跨连接复用加密结果；快照的生命周期同时限定缓存生命周期。
- 门面析构同步停止回调并取消在途任务；业务回调不得递归 pump 或 tick。
- Connection.inl 由头文件末尾包含，不能作为独立翻译单元。
