# Network Transport 模块

传输抽象层：把 Java TCP / 基岩 RakNet / 同进程 Local 三种传输统一到接口，上层 Connection 不感知传输差异。

## 目录结构

```
src/common/network/transport/
├── DeliveryHint.hpp       # enum{Unreliable,UnreliableSequenced,Reliable,ReliableOrdered}（RakNet reliability 等级，TCP/Local 忽略）
├── Endpoint.hpp           # struct{host,port} 网络端点
├── ITransport.hpp         # 字节传输抽象（原始字节流，无消息边界）：send/onBytes/onDisconnect/isConnected/close
├── TcpTransport.hpp/cpp   # asio TCP 原始字节管道（同步 socket + 独立接收线程，沿用旧 NetworkClient 成熟模式；不做任何帧编解码）
├── LocalTransport.hpp/cpp # 同进程零拷贝：实现独立 ILocalTransport 接口直传 ir::IrPacket 对象不经序列化（LocalTransportPair 配对）
└── RakNetTransport.hpp    # 基岩 UDP stub：全部操作返回 NotInitialized，待基岩后端落地
```

## 内部模块关系

- `ITransport` 是 Wire 模式（字节传输）统一抽象；`ILocalTransport` 是 Local 模式（IR 包直传）独立接口。两者都由 Connection 持有，按模式分流。
- `TcpTransport` 实现 `ITransport`，**只做原始字节读写**，不做帧编解码——send 把上层字节写进 socket，onBytes 把 socket 读到的字节块回调给上层。VarInt 长度前缀帧编解码在 `pipeline/VarintFraming`（属 Java wire 格式细节，传输层不感知）。
- `LocalTransport` 实现 `ILocalTransport`，一对经 `LocalTransportPair::create` 互连，send 进对端 inbox，pump 取出回调。
- `RakNetTransport` 实现 `ITransport`，stub 全返回 NotInitialized。
- `DeliveryHint` 仅 RakNet 用，TCP 全按 ReliableOrdered，Local 直接同步投递。

## 上下游外部依赖关系

- **上游依赖**：`common/core/Result`、`common/core/Types`、`common/network/ir/IrPacket`（仅 LocalTransport）、`asio`（仅 TcpTransport）。
- **下游**：`pipeline/Connection` 持有 ITransport 或 ILocalTransport；`server/network/ServerNetwork` 与 `client/network/ClientNetwork` 创建 TcpTransport；`IntegratedServer` 用 LocalTransportPair（Phase7）。

## 容易踩的坑

1. **TcpTransport 不做帧编解码**：send 投递什么字节就写什么字节，onBytes 回调的是 socket 读到的原始字节块（可能含部分帧或多个帧）。VarInt 长度前缀切帧由 `pipeline/VarintFraming` 负责，传输层只当字节管道。Java 出站顺序是 compress→frame→encrypt，加密后的密文整帧经本类写 socket，入站密文字节经本类回调给 Connection 再 decrypt→frame→decompress。
2. **TcpTransport 是同步 socket + 独立接收线程**：io_context 不调 run()，仅作 resolver/socket 工厂；receiveThread 跑同步 read_some 循环。高并发需改 async（Phase7 评估）。
3. **LocalTransport 的 pump 须由对端驱动**：send 只把包放进对端 inbox，回调发生在对端 pump 时；集成服须在 tick 中调用 pump，否则包积压不送达。
4. **LocalTransport 回调前释放锁**：pump 先把 inbox swap 出来再回调，避免回调中再 send 导致死锁。
5. **LocalTransportPair 配对后 peer 是裸指针**：client/server 互持裸指针，须保证 pair 生命周期长于任一端使用；先销毁一端会留下悬垂 peer 指针。
6. **RakNetTransport 是 stub**：所有操作返回 NotInitialized，调用方须先检查 isConnected/supported；基岩后端落地前不要接入真实流量。
7. **DeliveryHint 对 TCP 无意义**：TCP 本身可靠有序，send 的 hint 参数在 TcpTransport 里被忽略；别依赖它改变 TCP 行为。RakNet 数据报天然有边界，每个数据报作为一次 onBytes。
