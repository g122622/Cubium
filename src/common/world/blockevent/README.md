# blockevent/

方块事件系统，用于服务端向客户端同步方块动画和状态变化。

## 目录结构

```
blockevent/
└── BlockEventData.hpp    # 方块事件数据结构（位置、方块类型、事件参数）
```

## 内部模块关系

- `BlockEventData` 被 `ServerWorld` 用于维护待处理方块事件队列
- 队列在每tick中处理：验证方块匹配后执行事件并广播 `BlockEventPacket` 给客户端

## 上下游外部依赖关系

- 上游：`ServerWorld::blockEvent()` 创建 `BlockEventData` 入队
- 下游：`BlockEventPacket` 网络包发送给客户端；客户端收到后调用 `BlockEntity::triggerEvent()`

## 容易踩的坑

- `BlockEventData` 的 `operator==` 和 `std::hash` 必须一致，否则去重队列会失效
- 事件参数 `paramA`/`paramB` 的含义因方块类型而异（如箱子用 paramA=1,paramB=打开人数；音符盒用 paramA=0,paramB=0）
