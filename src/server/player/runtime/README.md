# 玩家运行数据与连接关联

## 目录结构树

```text
runtime/
└── ServerPlayerData.hpp  # 服务端玩家数据
```

## 内部模块关系

ServerPlayer 承接实体玩法；runtime 保存会话关联数据；lifecycle 管理在线记录与退出顺序；movement 更新位置；gamemode 管理模式及能力；重生流程由 PlayerRespawnManager 协调。

## 上下游外部依赖关系

应用层装配服务，网络处理器调用领域入口；世界玩家实体管理和区块同步分别处理实体可见性与订阅。

## 容易踩的坑

- PacketSender 只投递包，PlayerConnectionManager 关闭连接，PlayerManager 统一移除记录。
- 玩家移除前释放菜单、实体和区块资源，移除后通知剩余玩家；重复移除不得重复通知。
- 玩家移动与区块订阅分开，订阅统一使用 ChunkSyncManager 持有的 tracker。
