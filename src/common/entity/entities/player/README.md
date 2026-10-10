# 玩家实体模块

## 目录结构

```text
player/
├── ChatVisibility.hpp       # 聊天可见性枚举
├── GameModeUtils.hpp/cpp    # 游戏模式能力映射
├── Player.hpp/cpp           # 玩家状态、能力、移动、背包与战斗
├── PlayerModelPart.hpp      # 皮肤部件位掩码
└── SpawnLocationHelper.hpp  # 重生位置辅助工具
```

## 内部模块关系

Player 继承 LivingEntity，组合物品栏、饥饿、经验和统计状态；游戏模式工具决定能力与建造权限。

## 上下游外部依赖关系

依赖公共实体、物品、世界、碰撞、文本和存档类型；客户端持有本地 Player，服务端 ServerPlayer 扩展网络同步与玩家管理。

## 容易踩的坑

- 玩家 ID 与实体 ID 不同，通过 ServerPlayerEntityManager 解析，不能强制转换。
- setPrimaryHand 与 setPlayerModelParts 同时更新实体元数据；仅改本地字段不会通知其他客户端。
- 离开旁观模式要恢复摄像机和 noclip；摄像机实体消失或潜行时停止跟踪。
- 恢复站立姿态前检查碰撞；建造权限由 allowEdit 与冒险模式物品标签共同决定。
- 死亡前归还菜单物品；经验和物品掉落走统一死亡链路，避免重复掉落。
- 最后死亡位置包含维度，NBT 与 PlayerSaveData 均需保存；恢复指南针依赖该值。
- PvP 判定由攻击者检查目标，ServerPlayer 额外检查世界 PvP 规则与队伍友伤。
- 立即发送击退速度后清除 hurtMarked，避免追踪器重复应用；重锤与风弹冲量的坠落免疫要保留宽限期。
- 横扫静止判定使用客户端上报的 knownMovement，在 client_tick_end 收口，不能在服务端 tick 清零。
