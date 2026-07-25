#命令实现子目录

这里放的是服务端内置命令的具体实现。这个目录只负责把解析后的命令树节点绑定到执行逻辑，不负责命令注册入口，也不负责服务器生命周期。

##目录结构

```text src / server / command / commands /
├── AttributeCommand.hpp /
        AttributeCommand.cpp #查询和修改活体实体属性（支持所有 LivingEntity，使用 EntityResolver）
├── ClearCommand.hpp / ClearCommand.cpp #清空玩家背包命令
├── DefaultGameModeCommand.hpp / DefaultGameModeCommand.cpp #设置服务器默认游戏模式
├── DifficultyCommand.hpp / DifficultyCommand.cpp #修改世界难度
├── SpreadPlayersCommand.hpp /
        SpreadPlayersCommand.cpp #随机分散玩家到区域内（支持 under<maxHeight> 子命令，维度感知高度）
├── ExecuteCommand.hpp / ExecuteCommand.cpp #执行嵌套命令，支持多种执行上下文修改（as / at / in / positioned / run /
        if / unless）
├── ExperienceCommand.hpp / ExperienceCommand.cpp #管理经验值和等级
├── FillCommand.hpp / FillCommand.cpp #填充区域方块
├── GameModeCommand.hpp / GameModeCommand.cpp #切换玩家游戏模式
├── GiveCommand.hpp / GiveCommand.cpp #发放物品
├── HelpCommand.hpp / HelpCommand.cpp #展示命令帮助
├── KickCommand.hpp / KickCommand.cpp #踢出在线玩家
├── KillCommand.hpp / KillCommand.cpp #杀死实体或自己（使用 EntityResolver 支持所有实体类型）
├── ListCommand.hpp / ListCommand.cpp #列出在线玩家
├── ReloadCommand.hpp / ReloadCommand.cpp #重新加载战利品表、配方、函数、谓词和进度
├── SayCommand.hpp / SayCommand.cpp #广播聊天消息
├── ReplaceItemCommand.hpp / ReplaceItemCommand.cpp #替换物品栏 /
        容器槽位物品（支持玩家背包、装备、末影箱、光标槽位，方块容器槽位）
├── ScoreboardCommand.hpp / ScoreboardCommand.cpp #记分板目标管理
├── SeedCommand.hpp / SeedCommand.cpp #显示当前世界种子
├── SetBlockCommand.hpp / SetBlockCommand.cpp #放置单个方块
├── SetIdleTimeoutCommand.hpp / SetIdleTimeoutCommand.cpp #设置玩家挂机超时
├── SetWorldSpawnCommand.hpp / SetWorldSpawnCommand.cpp #设置世界出生点
├── SpawnPointCommand.hpp / SpawnPointCommand.cpp #设置玩家个人重生点
├── StopCommand.hpp / StopCommand.cpp #请求服务器停机
├── TeamCommand.hpp / TeamCommand.cpp #队伍系统管理命令
├── TeleportCommand.hpp / TeleportCommand.cpp #处理 / tp 与 / teleport 传送逻辑
├── TimeCommand.hpp / TimeCommand.cpp #修改或查询游戏时间
├── TriggerCommand.hpp / TriggerCommand.cpp #触发器命令（权限等级0，所有玩家可用）
├── WardenSpawnTrackerCommand.hpp / WardenSpawnTrackerCommand.cpp #监守者警告追踪器命令（clear / set子命令）
└── WeatherCommand.hpp /
        WeatherCommand.cpp #修改或查询天气状态
```

        ##模块关系

    - 这些实现都被 `CommandRegistry::registerDefaults()` 注册到命令树
    - 每个命令通过 `ServerCommandSource` 读取权限、玩家、世界和反馈输出
    - `support /` 目录提供共享的元数据、参数解析、玩家解析等辅助逻辑

        ##依赖关系

            ** 上游依赖（谁依赖了这个目录）：* *
        - `server / command / CommandRegistry` -
    通过 `registerDefaults()` 调用各命令的 `registerTo()`

            ** 下游依赖（这个目录依赖了谁）：* *
        -内部依赖：`server / command / CommandRegistry`、`server / command / ServerCommandSource`、`server / command
        / support/*`、`server/application/IServer`、`server/core/*`、`server/player/ServerPlayer`
- 共享依赖：`common/command/*`、`common/entity/inventory/*`、`common/item/*`、`common/network/ir/*`（命令反馈/广播走 `ir::play::SystemChat`/`SetActionBarText` 等 IR 包，经 `connection.send(ir::IrPacket{...})` 发送；旧 `common/network/packet/*` 已删除）
- 外部依赖：`spdlog`、`GTest`（测试场景）

## 容易踩的坑

- **不要直接 dynamic_cast 到 IntegratedServer**：统一通过 `IServer::playerInventory()` 取库存，通过 `IServer::sharedStorage()` 获取共享存储
- **命令元数据和实际语法要同步更新**：否则帮助输出和补全会偏离
- **别名命令使用重定向共享命令树**：`/tp` 和 `/teleport`、`/experience` 和 `/xp` 这类别名应使用重定向
- **目标选择器解析依赖 support 辅助函数**：不要在每个命令里重复实现
- **EntityResolver vs PlayerResolver 选择**：需要 @e 选择器支持的命令（如 KillCommand、TagCommand、DataCommand）应使用 EntityResolver，仅处理玩家的命令可使用 PlayerResolver
- **LootCommand kill 源**：`/loot kill <target>` 使用 `EntityResolver::resolve()` 解析任意实体（包括非玩家实体如僵尸、动物等），对齐 MC Java 行为。`generateFromKill()` 通过 `target->getLootTableId()` 获取战利品表ID，使用 `LootParameterSets::entity()` 参数集构建上下文，以魔法伤害（`DamageSources::magic()`）作为伤害源、命令执行者实体（`source.entity()`）作为击杀者（KILLER_ENTITY/DIRECT_KILLER）。如果执行者是玩家则额外设置 KILLER_PLAYER 参数。非玩家实体执行者（如通过 `/execute as @e` 指定）也已支持。
- **只读世界存储的保存命令处理**：当共享存储是外来只读世界时，`save-all` / `save-on` / `save-off` 必须显式提示"不会写入"，不能继续伪装成普通可写世界
- **天气命令统一通过 ServerCommandSource::world() 获取世界**：使用天气管理器进行操作
- **`/setblock destroy` 和 `/fill destroy`/`/fill hollow` 调用 spawnAfterBreak**：这些命令在替换方块时，先保存旧方块状态，设置新方块后调用 `spawnAfterBreak(nullptr, false)`，使得虫蚀方块等特殊方块能正确触发生成逻辑。`/clone move` 不调用 spawnAfterBreak（MC Java 行为：仅清空源区域）。
