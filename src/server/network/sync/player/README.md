# 玩家实体、运行数据与生命周期

## 目录结构树

```text
player/
├── TeleportManager.cpp  # 模块接口与实现
└── TeleportManager.hpp  # 传送管理器
```

## 内部模块关系

TeleportManager 维护传送编号、待确认状态及回包校验，通过 PlayerMovementManager 更新玩家位置与区块中心；它不独立维护另一份坐标或区块订阅。

## 上下游外部依赖关系

应用层装配 PlayerManager 与 PlayerMovementManager；入站确认、旁观者传送和服务器位置同步使用本服务。出站位置使用公共 IR 包和玩家连接。

## 容易踩的坑

- PacketSender 只投递包，PlayerConnectionManager 关闭连接，PlayerManager 统一移除记录。
- 玩家移除前释放菜单、实体和区块资源，移除后通知剩余玩家；重复移除不得重复通知。
- 玩家移动与区块订阅分开，订阅统一使用 ChunkSyncManager 持有的 tracker。
