# 命令实现子目录

这里放的是服务端内置命令的具体实现。这个目录只负责把解析后的命令树节点绑定到执行逻辑，不负责命令注册入口，也不负责服务器生命周期。

## 目录结构

```text
src/server/command/commands/
├── ClearCommand.hpp / ClearCommand.cpp           # 清空玩家背包命令
├── DefaultGameModeCommand.hpp / DefaultGameModeCommand.cpp  # 设置服务器默认游戏模式
├── DifficultyCommand.hpp / DifficultyCommand.cpp # 修改世界难度
├── SpreadPlayersCommand.hpp / SpreadPlayersCommand.cpp  # 随机分散玩家到区域内（支持 under <maxHeight> 子命令，维度感知高度）
├── ExecuteCommand.hpp / ExecuteCommand.cpp       # 执行嵌套命令，支持多种执行上下文修改
├── ExperienceCommand.hpp / ExperienceCommand.cpp # 管理经验值和等级
├── FillCommand.hpp / FillCommand.cpp             # 填充区域方块
├── GameModeCommand.hpp / GameModeCommand.cpp     # 切换玩家游戏模式
├── GiveCommand.hpp / GiveCommand.cpp             # 发放物品
├── HelpCommand.hpp / HelpCommand.cpp             # 展示命令帮助
├── KickCommand.hpp / KickCommand.cpp             # 踢出在线玩家
├── KillCommand.hpp / KillCommand.cpp             # 杀死实体或自己
├── ListCommand.hpp / ListCommand.cpp             # 列出在线玩家
├── SayCommand.hpp / SayCommand.cpp               # 广播聊天消息
├── ReplaceItemCommand.hpp / ReplaceItemCommand.cpp  # 替换物品栏/容器槽位物品（支持槽位名称解析）
├── ScoreboardCommand.hpp / ScoreboardCommand.cpp # 记分板目标管理
├── SeedCommand.hpp / SeedCommand.cpp             # 显示当前世界种子
├── SetBlockCommand.hpp / SetBlockCommand.cpp     # 放置单个方块
├── SetIdleTimeoutCommand.hpp / SetIdleTimeoutCommand.cpp  # 设置玩家挂机超时
├── SetWorldSpawnCommand.hpp / SetWorldSpawnCommand.cpp    # 设置世界出生点
├── SpawnPointCommand.hpp / SpawnPointCommand.cpp # 设置玩家个人重生点
├── StopCommand.hpp / StopCommand.cpp             # 请求服务器停机
├── TeamCommand.hpp / TeamCommand.cpp             # 队伍系统管理命令
├── TeleportCommand.hpp / TeleportCommand.cpp     # 处理 /tp 与 /teleport 传送逻辑
├── TimeCommand.hpp / TimeCommand.cpp             # 修改或查询游戏时间
├── TriggerCommand.hpp / TriggerCommand.cpp       # 触发器命令（权限等级0，所有玩家可用）
└── WeatherCommand.hpp / WeatherCommand.cpp       # 修改或查询天气状态
```

## 模块关系

- 这些实现都被 `CommandRegistry::registerDefaults()` 注册到命令树
- 每个命令通过 `ServerCommandSource` 读取权限、玩家、世界和反馈输出
- `support/` 目录提供共享的元数据、参数解析、玩家解析等辅助逻辑

## 依赖关系

**上游依赖（谁依赖了这个目录）：**
- `server/command/CommandRegistry` - 通过 `registerDefaults()` 调用各命令的 `registerTo()`

**下游依赖（这个目录依赖了谁）：**
- 内部依赖：`server/command/CommandRegistry`、`server/command/ServerCommandSource`、`server/command/support/*`、`server/application/IServer`、`server/core/*`、`server/player/ServerPlayer`
- 共享依赖：`common/command/*`、`common/entity/inventory/*`、`common/item/*`、`common/network/packet/*`
- 外部依赖：`spdlog`、`GTest`（测试场景）

## 容易踩的坑

- **不要直接 dynamic_cast 到 IntegratedServer**：统一通过 `IServer::playerInventory()` 取库存，通过 `IServer::sharedStorage()` 获取共享存储
- **命令元数据和实际语法要同步更新**：否则帮助输出和补全会偏离
- **别名命令使用重定向共享命令树**：`/tp` 和 `/teleport`、`/experience` 和 `/xp` 这类别名应使用重定向
- **目标选择器解析依赖 support 辅助函数**：不要在每个命令里重复实现
- **只读世界存储的保存命令处理**：当共享存储是外来只读世界时，`save-all` / `save-on` / `save-off` 必须显式提示"不会写入"，不能继续伪装成普通可写世界
- **天气命令统一通过 ServerCommandSource::world() 获取世界**：使用天气管理器进行操作
- **`/setblock destroy` 和 `/fill destroy`/`/fill hollow` 调用 spawnAfterBreak**：这些命令在替换方块时，先保存旧方块状态，设置新方块后调用 `spawnAfterBreak(nullptr, false)`，使得虫蚀方块等特殊方块能正确触发生成逻辑。`/clone move` 不调用 spawnAfterBreak（MC Java 行为：仅清空源区域）。
