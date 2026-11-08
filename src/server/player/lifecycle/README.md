# 玩家记录、资源释放与退出通知

## 目录结构树

```text
lifecycle/
├── PlayerManager.cpp  # 模块接口与实现
└── PlayerManager.hpp  # 玩家管理器
```

## 内部模块关系

PlayerManager 是在线玩家记录入口。移除前调用资源释放钩子，之后删除会话映射、记录和区块 tracker，最后以快照参数发布移除通知。按会话移除复用同一入口。

## 上下游外部依赖关系

依赖 runtime 数据与区块同步；应用装配资源释放/离场通知，网络会话、命令和登录失败路径调用移除操作。

## 容易踩的坑

- PacketSender 只投递包，PlayerConnectionManager 关闭连接，PlayerManager 统一移除记录。
- 玩家移除前释放菜单、实体和区块资源，移除后通知剩余玩家；重复移除不得重复通知。
- 玩家移动与区块订阅分开，订阅统一使用 ChunkSyncManager 持有的 tracker。
