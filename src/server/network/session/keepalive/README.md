# 会话心跳与超时判定

## 目录结构树

```text
keepalive/
├── KeepAliveManager.cpp  # 模块接口与实现
└── KeepAliveManager.hpp  # 心跳管理器
```

## 内部模块关系

ServerNetwork 管理连接集合；ClientSession/ClientSessionManager 维护协议会话；PlayerConnectionManager 发起关闭并委托 PlayerManager 移除玩家。被动断开同样进入统一玩家生命周期。

## 上下游外部依赖关系

连接实现来自 network/base，入站委派 handshake/play；玩家资源释放由应用装配到 PlayerManager。

## 容易踩的坑

- PacketSender 只投递包，PlayerConnectionManager 关闭连接，PlayerManager 统一移除记录。
- 玩家移除前释放菜单、实体和区块资源，移除后通知剩余玩家；重复移除不得重复通知。
- 玩家移动与区块订阅分开，订阅统一使用 ChunkSyncManager 持有的 tracker。
