# 网络传输层

## 目录结构树

```text
transport/
├── ITransport.hpp            # 字节传输、串行任务执行和关闭契约
├── TcpTransport.hpp/cpp      # 共享事件循环、每连接串行执行及异步 TCP 收发
├── LocalTransport.hpp/cpp    # 同进程 IR 队列与配对传输
├── RakNetTransport.hpp       # 基岩传输接口
├── Endpoint.hpp              # 连接地址
└── DeliveryHint.hpp          # 传输语义
```

## 内部模块关系

`TcpTransport` 在进程共享的 NetworkIO 事件循环上运行，每连接拥有一个 strand。
`execute` 将编码与配置任务按提交顺序串行执行；接收回调、异步写入和关闭同属该 strand。
Local 传输由调用方在主线程 pump，不经过 TCP 或字节流水线。

## 上下游外部依赖关系

本层依赖 Asio、公共类型、Result 和性能追踪；`pipeline/Connection` 负责协议编解码、压缩和加密。
服务端 `ServerNetwork` 创建已连接的 TCP 传输，客户端 `ClientNetwork` 调用 connect。

## 容易踩的坑

- `send` 和 `execute` 成功表示成功入队，异步失败通过断开回调报告。
- 同一连接只允许一个异步写入在途；缓冲区由完成回调持有，调用方可立即释放原始字节。
- 编码前最多积压 4096 个任务，编码后最多积压 32 MiB 字节；每批写入使用公共写超时。
  超限或超时关闭连接，不能用阻塞写入把压力传给主线程。
- `closeAfterFlush` 停止接受新任务，按顺序完成此前数据；`close` 取消任务和写入，并等待当前回调结束。
  在 strand 内关闭直接执行，不等待自身。
- 接管 socket 时迁移原生句柄到共享事件循环，不能依赖 accept 所用的临时 io_context 驱动后续收发。
- TCP 只有字节流；VarInt 帧、压缩和加密均属于 Connection，不能放进传输层。
- 接收前必须装配完整字节回调。LocalTransport 的 pump 在锁外回调；成对传输的生命周期由调用方保证。
