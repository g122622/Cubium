# 玩家位置和朝向更新

## 目录结构树

```text
movement/
├── PlayerMovementManager.cpp  # 模块接口与实现
└── PlayerMovementManager.hpp  # 位置追踪器
```

## 内部模块关系

PlayerMovementManager 维护位置、朝向与落地状态，位置变化通知 ChunkSyncManager 更新区块中心；视距、区块订阅和发送标记由同步组件负责。

## 上下游外部依赖关系

依赖 PlayerManager 与坐标值类型；入站移动处理、服务器位置同步及测试消费本服务。

## 容易踩的坑

- PacketSender 只投递包，PlayerConnectionManager 关闭连接，PlayerManager 统一移除记录。
- 玩家移除前释放菜单、实体和区块资源，移除后通知剩余玩家；重复移除不得重复通知。
- 玩家移动与区块订阅分开，订阅统一使用 ChunkSyncManager 持有的 tracker。
