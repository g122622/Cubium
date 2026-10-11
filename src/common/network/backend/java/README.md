# Java 协议后端

## 目录结构

```text
java/
├── JavaBackend.hpp/cpp         # 提供 Java 协议表
├── JavaProtocolTables.hpp/cpp  # 各阶段、各方向的 wire ID 与 codec 登记
├── codecs/                    # 各阶段字段的线格式编解码
├── handshake/                 # 握手协商
├── mappings/                  # 物品和方块状态的内部 ID 翻译
└── generated/                 # 构建时生成的映射表
```

## 内部模块关系

JavaBackend 委托 JavaProtocolTables 构建协议表；协议表组合 codecs 的编解码器。

## 上下游外部依赖关系

依赖公共 IR、RegistryByteBuf 和 ProtocolInfoBuilder；连接管线消费协议表。

## 容易踩的坑

- wire ID、包方向、PlayPacket 变体下标分别登记，三者不可混用。
- 双向使用同一 IR 的包必须分别登记，如 ContainerClose 的上行 18、下行 17。
- 新包必须同时接通业务收发；只有 codec 不能保证真实客户端互通。
