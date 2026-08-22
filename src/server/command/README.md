# Server Command 模块

服务端命令系统，提供 Minecraft 风格的命令注册、解析、执行和建议功能。

## 目录结构

```
src/server/command/
├── CommandRegistry.hpp           # 命令注册表，管理命令分发器单例和默认命令注册
├── CommandRegistry.cpp
├── ServerCommandSource.hpp       # 服务端命令源，扩展 ICommandSource 提供服务器/玩家/世界访问
├── ServerCommandSource.cpp
├── README.md
├── support/                      # 命令支持工具
│   ├── PlayerResolver.hpp        # 玩家选择器解析器（@a, @p, @r 等解析）
│   ├── PlayerResolver.cpp
│   ├── EffectResolver.hpp        # 效果选择器解析器
│   ├── EffectResolver.cpp
│   ├── CommandMetadata.hpp       # 命令元数据定义
│   └── README.md
└── commands/                     # 具体命令实现
    ├── ClearCommand.hpp          # /clear - 清空玩家背包
    ├── ClearCommand.cpp
    ├── CloneCommand.hpp          # /clone - 复制方块区域
    ├── CloneCommand.cpp
    ├── ForceLoadCommand.hpp      # /forceload - 强制加载区块
    ├── ForceLoadCommand.cpp
    ├── GameModeCommand.hpp       # /gamemode - 设置游戏模式
    ├── GameModeCommand.cpp
    ├── GiveCommand.hpp           # /give - 给予玩家物品
    ├── GiveCommand.cpp
    ├── LootCommand.hpp           # /loot - 从战利品表生成物品
    ├── LootCommand.cpp
    ├── ExperienceCommand.hpp     # /experience / /xp - 管理经验值
    ├── ExperienceCommand.cpp
    ├── HelpCommand.hpp           # /help - 显示命令帮助
    ├── HelpCommand.cpp
    ├── KillCommand.hpp           # /kill - 杀死实体
    ├── KillCommand.cpp
    ├── ListCommand.hpp           # /list - 列出在线玩家
    ├── ListCommand.cpp
    ├── SeedCommand.hpp           # /seed - 显示世界种子
    ├── SeedCommand.cpp
    ├── TeleportCommand.hpp       # /tp / /teleport - 传送实体
    ├── TeleportCommand.cpp
    ├── TimeCommand.hpp           # /time - 控制游戏时间
    ├── TimeCommand.cpp
    ├── WeatherCommand.hpp        # /weather - 控制天气
    ├── WeatherCommand.cpp
    ├── BanCommand.hpp            # /ban - 封禁玩家
    ├── BanCommand.cpp
    ├── BanIpCommand.hpp          # /ban-ip - 封禁 IP
    ├── BanIpCommand.cpp
    ├── PardonCommand.hpp         # /pardon / /unban - 解除玩家封禁
    ├── PardonCommand.cpp
    ├── PardonIpCommand.hpp       # /pardon-ip / /unban-ip - 解除 IP 封禁
    ├── PardonIpCommand.cpp
    ├── BanListCommand.hpp        # /banlist - 显示封禁列表
    ├── BanListCommand.cpp
    ├── MessageCommand.hpp        # /msg / /tell / /w - 私聊消息
    ├── MessageCommand.cpp
    ├── TellRawCommand.hpp        # /tellraw - 发送 JSON 富文本消息
    ├── TellRawCommand.cpp
    ├── SaveOnCommand.hpp         # /save-on - 启用自动保存
    ├── SaveOnCommand.cpp
    ├── SaveOffCommand.hpp        # /save-off - 禁用自动保存
    ├── SaveOffCommand.cpp
    ├── MeCommand.hpp             # /me - 显示动作消息
    ├── MeCommand.cpp
    ├── ParticleCommand.hpp       # /particle - 显示粒子效果
    ├── ParticleCommand.cpp
    ├── SetBlockCommand.hpp       # /setblock - 放置或替换方块
    ├── SetBlockCommand.cpp
    ├── LocateCommand.hpp         # /locate - 定位建筑结构
    ├── LocateCommand.cpp
    ├── LocateBiomeCommand.hpp    # /locatebiome - 定位生物群系
    ├── LocateBiomeCommand.cpp
    ├── AttributeCommand.hpp      # /attribute - 查询/修改实体属性（get/base/modifier子命令）
    ├── AttributeCommand.cpp
    ├── EnchantCommand.hpp        # /enchant - 给手持物品添加附魔
    ├── EnchantCommand.cpp
    ├── AdvancementCommand.hpp    # /advancement - 管理玩家进度
    ├── AdvancementCommand.cpp
    ├── BossBarCommand.hpp        # /bossbar - 创建和管理 Boss 条
    ├── BossBarCommand.cpp
    ├── DataPackCommand.hpp       # /datapack - 管理数据包
    ├── DataPackCommand.cpp
    ├── PublishCommand.hpp        # /publish - 开放到局域网
    ├── PublishCommand.cpp
    ├── RecipeCommand.hpp         # /recipe - 管理配方解锁
    ├── RecipeCommand.cpp
    ├── ReloadCommand.hpp         # /reload - 重新加载数据包
    ├── ReloadCommand.cpp
    ├── ReplaceItemCommand.hpp    # /replaceitem - 替换实体或容器物品
    ├── ReplaceItemCommand.cpp
    ├── ScheduleCommand.hpp       # /schedule - 延迟执行函数
    ├── ScheduleCommand.cpp
    ├── ScoreboardCommand.hpp     # /scoreboard - 管理计分板
    ├── ScoreboardCommand.cpp
    ├── SpectateCommand.hpp       # /spectate - 旁观者模式观看实体
    ├── SpectateCommand.cpp
    ├── SpreadPlayersCommand.hpp  # /spreadplayers - 随机分散玩家
    ├── SpreadPlayersCommand.cpp
    ├── ExecuteCommand.hpp        # /execute - 修改执行上下文执行嵌套命令
    ├── ExecuteCommand.cpp
    ├── SpawnPointCommand.hpp     # /spawnpoint - 设置重生点
    ├── SpawnPointCommand.cpp
    ├── SetWorldSpawnCommand.hpp  # /setworldspawn - 设置世界出生点（支持朝向参数）
    ├── SetWorldSpawnCommand.cpp
    ├── TagCommand.hpp            # /tag - 管理实体标签
    ├── TagCommand.cpp
    ├── TeamCommand.hpp           # /team - 管理队伍
    ├── TeamCommand.cpp
    ├── TriggerCommand.hpp        # /trigger - 修改触发器计分板
    ├── TriggerCommand.cpp
    ├── WorldBorderCommand.hpp    # /worldborder - 管理世界边界
    └── WorldBorderCommand.cpp
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                     CommandRegistry                         │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              CommandDispatcher                       │   │
│  │  ┌─────────────────────────────────────────────┐    │   │
│  │  │           RootCommandNode                    │    │   │
│  │  │  ┌───────┬───────┬───────┬───────┬─────┐   │    │   │
│  │  │  │gamemode│ time  │ kill  │ list  │ ... │   │    │   │
│  │  │  └───┬───┴───┬───┴───┬───┴───┬───┴─────┘   │    │   │
│  │  └─────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   ServerCommandSource                       │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │  IServer*   │  │ServerPlayer*│  │   permissionLevel   │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │  Entity*    │  │ServerWorld* │  │   permissionLevel   │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │  position   │  │  rotation   │  │     playerId        │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      Command Handlers                        │
│  各命令实现类（GameModeCommand, TimeCommand, WeatherCommand  │
│  TeleportCommand, GiveCommand, ClearCommand, KillCommand 等）│
└─────────────────────────────────────────────────────────────┘
```

**核心组件职责：**

- `CommandRegistry` - 管理全局命令分发器单例，注册所有默认命令，提供命令执行和建议入口
- `ServerCommandSource` - 命令执行上下文，封装服务器、玩家、实体、世界、位置、权限等信息，支持派生创建。
  - 对齐 MC Java `CommandSourceStack`，包含 `entity()` 和 `withEntity()` 方法支持非玩家实体执行者。
  - `player()` 返回 `ServerPlayer*`（可为 nullptr），`entity()` 返回 `Entity*`（可为 nullptr）。
  - 玩家执行命令时 `entity()` 与 `player()` 一致；`/execute as @e` 后 `entity()` 可为非玩家实体。
- `support/` - 解析器工具，PlayerResolver 处理玩家选择器（@a/@p/@r），EntityResolver 处理实体选择器（@e/@s），EffectResolver 处理效果参数
- `commands/` - 各具体命令实现，每个命令类提供 `registerTo()` 方法注册到分发器

## 上下游依赖关系

**上游依赖（本模块使用的模块）：**

```
server/command
├── common/command/           # 命令框架核心（CommandDispatcher、CommandContext、CommandNode、参数类型、异常）
├── server/application/       # 服务器核心（IServer、MinecraftServer）
├── server/core/              # 核心管理器（PlayerManager、TimeManager、TeleportManager、GameModeManager）
├── server/player/            # 服务端玩家（ServerPlayer）
├── server/world/             # 服务端世界（ServerWorld、WeatherManager）
├── common/item/              # 物品系统（ItemStack）
├── common/world/             # 世界系统（IWorld、BlockState）
└── common/entity/            # 实体系统（Player、Entity）
```

**下游依赖（使用本模块的模块）：**

- `server/application/` - MinecraftServer 通过 CommandRegistry 处理聊天消息中的命令
- `server/network/` - 数据包处理器接收命令请求后调用 CommandRegistry 执行

## 容易踩的坑

1. **权限等级检查遗漏**
   - 问题：忘记为命令节点设置权限要求，导致普通玩家可执行管理员命令
   - 解决：始终为命令节点设置 `setRequirement` 检查权限

2. **命令源类型检查**
   - 问题：在非玩家命令源上调用 `assertPlayer()` 导致崩溃
   - 解决：先使用 `isPlayer()` 检查，或捕获 `CommandException`

3. **服务器指针为空**
   - 问题：命令执行时 `source.server()` 返回 nullptr
   - 解决：始终检查服务器指针有效性

4. **玩家命令反馈依赖**
   - 问题：`ServerCommandSource::sendMessage()` 在只有 `playerId` 时需要发消息给在线连接
   - 解决：不要默认依赖 `ServerPlayer*`，`sendMessage()` 必须能在只有 `playerId` 时发消息

5. **参数类型不匹配**
   - 问题：使用 `getArgument<T>()` 时模板参数与注册时不一致
   - 解决：确保模板参数与 `ArgumentCommandNode` 的类型一致

6. **EntitySelector 已完整实现**
   - `EntityArgument` 返回的 `EntitySelector` 已包含完整的选择器解析逻辑
   - `EntityResolver` 支持 @e/@p/@a/@r/@s 等选择器及全部过滤条件（type/name/tag/team/distance/volume/level/gamemode/scores/advancements/nbt/predicate）
   - 注意：`PredicateCondition::hasCondition()` 检查 `predicate.path()` 非空，因为 `ResourceLocation` 默认构造函数设置 namespace 为 "minecraft"

7. **天气命令世界获取**
   - 问题：天气命令不应通过 `getOverworld()` 获取天气管理器
   - 解决：通过 `ServerCommandSource::world()` 获取当前命令上下文的世界

8. **ClearCommand 库存获取**
   - 问题：命令层直接依赖 `IntegratedServer`
   - 解决：通过 `IServer::playerInventory()` 统一获取单机/联机库存

9. **帮助信息同步**
   - 问题：`HelpCommand` 中的帮助信息是硬编码的
   - 解决：添加新命令时需同步更新 `s_commandHelp` 数组

10. **命令反馈未发送**
    - 问题：命令执行后玩家看不到反馈
    - 解决：确保通过 `source.sendMessage()` 发送反馈消息

11. **SetWorldSpawnCommand 朝向参数（对齐 MC 1.21.11）**
    - `/setworldspawn` 支持三种语法：无参数（pos=floor(玩家位置)，rotation=ZERO_ROTATION(0,0)）、仅位置（rotation=ZERO_ROTATION）、位置+旋转
    - pos 用 `BlockPosArgumentType`（整数 floor，对齐 vanilla `BlockPosArgument`），非 `Vec3ArgumentType`（centerCorrect 会给绝对整数加 0.5 偏移致出生点偏 0.5）
    - rotation 用 `RotationArgumentType`（接 yaw pitch 两值，对齐 vanilla `RotationArgument`）；yaw 存 `ServerWorld::m_spawnAngle`，pitch 暂丢弃（TODO 完整建模）
    - yaw 通过 `math::wrapDegrees()` 归一化到 [-180, 180]
    - 修改出生点后需同时更新 `ServerWorld::setWorldSpawnPoint(pos, angle)` 和广播 `SpawnPositionPacket(angle)`
