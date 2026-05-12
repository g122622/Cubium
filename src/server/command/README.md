# Server Command 模块

服务端命令系统，提供 Minecraft 风格的命令注册、解析、执行和建议功能。

## 目录结构

```
src/server/command/
├── CommandRegistry.hpp        # 命令注册表头文件
├── CommandRegistry.cpp        # 命令注册表实现
├── ServerCommandSource.hpp    # 服务端命令源头文件
├── ServerCommandSource.cpp    # 服务端命令源实现
├── README.md                  # 本文档
├── support/                   # 命令支持工具
│   ├── PlayerResolver.hpp     # 玩家选择器解析器
│   ├── PlayerResolver.cpp     # 玩家选择器解析实现
│   ├── EffectResolver.hpp     # 效果选择器解析器
│   ├── EffectResolver.cpp     # 效果选择器解析实现
│   ├── CommandMetadata.hpp    # 命令元数据定义
│   └── README.md              # support 模块文档
└── commands/                  # 具体命令实现
    ├── ClearCommand.hpp       # /clear 命令
    ├── ClearCommand.cpp
    ├── CloneCommand.hpp       # /clone 命令
    ├── CloneCommand.cpp
    ├── ForceLoadCommand.hpp   # /forceload 命令
    ├── ForceLoadCommand.cpp
    ├── GameModeCommand.hpp    # /gamemode 命令
    ├── GameModeCommand.cpp
    ├── GiveCommand.hpp        # /give 命令
    ├── GiveCommand.cpp
    ├── ExperienceCommand.hpp  # /experience / /xp 命令
    ├── ExperienceCommand.cpp
    ├── HelpCommand.hpp        # /help 命令
    ├── HelpCommand.cpp
    ├── KillCommand.hpp        # /kill 命令
    ├── KillCommand.cpp
    ├── ListCommand.hpp        # /list 命令
    ├── ListCommand.cpp
    ├── SeedCommand.hpp        # /seed 命令
    ├── SeedCommand.cpp
    ├── TeleportCommand.hpp    # /tp 命令
    ├── TeleportCommand.cpp
    ├── TimeCommand.hpp        # /time 命令
    ├── TimeCommand.cpp
    ├── WeatherCommand.hpp     # /weather 命令
    └── WeatherCommand.cpp
    ├── BanCommand.hpp         # /ban 命令
    ├── BanCommand.cpp
    ├── BanIpCommand.hpp       # /ban-ip 命令
    ├── BanIpCommand.cpp
    ├── PardonCommand.hpp      # /pardon 命令
    ├── PardonCommand.cpp
    ├── PardonIpCommand.hpp    # /pardon-ip 命令
    ├── PardonIpCommand.cpp
    ├── BanListCommand.hpp     # /banlist 命令
    └── BanListCommand.cpp
```

## 文件详解

### 核心文件

#### CommandRegistry.hpp / CommandRegistry.cpp

命令注册表，管理所有命令的注册和分发。

**职责：**

- 维护全局命令分发器实例
- 注册所有默认命令
- 提供命令执行入口
- 提供命令查询和建议接口
- 命令名称直接从命令树派生，别名与重定向节点会一并反映出来

**主要接口：**

```cpp
class CommandRegistry {
public:
    // 获取分发器
    Dispatcher& dispatcher() noexcept;

    // 执行命令
    Result<i32> execute(const std::string& input, ServerCommandSource& source);

    // 获取建议
    std::future<Suggestions> getSuggestions(const std::string& input, ServerCommandSource& source);

    // 注册默认命令
    void registerDefaults();

    // 命令查询
    std::vector<std::string> getCommandNames() const;
    bool hasCommand(const std::string& name) const;

    // 全局单例
    static CommandRegistry& getGlobal();
};
```

**使用示例：**

```cpp
// 获取全局注册表
auto& registry = CommandRegistry::getGlobal();

// 执行命令
ServerCommandSource source = ...;
auto result = registry.execute("/gamemode creative", source);
if (result.success()) {
    spdlog::info("命令执行成功，结果: {}", result.value());
}
```

#### ServerCommandSource.hpp / ServerCommandSource.cpp

服务端命令源，扩展 `ICommandSource` 接口，提供服务端特有功能。

**职责：**

- 表示命令执行的来源（玩家或控制台）
- 提供服务器、玩家、世界访问接口
- 管理权限等级
- 支持创建派生命令源
- 支持静默输出和权限上限派生，便于实现更接近原版的命令上下文切换

**主要接口：**

```cpp
class ServerCommandSource : public ICommandSource {
public:
    // 构造函数
    ServerCommandSource(
        server::IServer* server,
        ServerPlayer* player = nullptr,
        server::ServerWorld* world = nullptr,
        const Vector3d& position = Vector3d(0, 0, 0),
        const Vector2f& rotation = Vector2f(0, 0),
        i32 permissionLevel = 0,
        PlayerId playerId = 0,
        std::string playerName = ""
    );

    // ICommandSource 接口
    void sendMessage(const std::string& message, const std::optional<Uuid>& senderUuid = std::nullopt) override;
    bool shouldReceiveFeedback() const override;
    bool shouldReceiveErrors() const override;
    bool allowLogging() const override;

    // 访问器
    server::IServer* server() const noexcept;
    ServerPlayer* player() const noexcept;
    PlayerId playerId() const noexcept;
    server::ServerWorld* world() const noexcept;
    const Vector3d& position() const noexcept;
    const Vector2f& rotation() const noexcept;
    i32 permissionLevel() const noexcept;
    const std::string& name() const noexcept;

    // 权限检查
    bool hasPermission(i32 level) const noexcept;
    bool isPlayer() const noexcept;
    ServerPlayer& assertPlayer() const;

    // 派生命令源
    ServerCommandSource withPlayer(ServerPlayer* player) const;
    ServerCommandSource withPosition(const Vector3d& pos) const;
    ServerCommandSource withRotation(const Vector2f& rot) const;
    ServerCommandSource withWorld(server::ServerWorld* world) const;
    ServerCommandSource withFeedbackDisabled() const;
    ServerCommandSource withSuppressedOutput() const;
    ServerCommandSource withPermissionLevel(i32 level) const;
    ServerCommandSource withMaximumPermission(i32 level) const;

    // 静态工厂
    static ServerCommandSource forConsole(server::IServer* server);
};
```

**权限等级：**
| 等级 | 描述 | 典型命令 |
|------|------|----------|
| 0 | 所有玩家 | `/list`, `/help` |
| 1 | MOD 管理 | - |
| 2 | 游戏管理 | `/gamemode`, `/tp`, `/time`, `/weather` |
| 3 | 服务器管理 | - |
| 4 | 控制台/OP | 所有命令 |

### commands/ 子目录

#### GameModeCommand - /gamemode 命令

设置玩家的游戏模式。

**用法：**

- `/gamemode <mode>` - 设置自己的游戏模式
- `/gamemode <mode> <target>` - 设置指定玩家的游戏模式

**模式：**

- `survival` / `0` - 生存模式
- `creative` / `1` - 创造模式
- `adventure` / `2` - 冒险模式
- `spectator` / `3` - 旁观者模式

**权限等级：** 2

#### TimeCommand - /time 命令

控制游戏时间。

**用法：**

- `/time set <value>` - 设置时间（0-24000）
- `/time add <value>` - 增加时间
- `/time query <day|daytime|gametime>` - 查询时间

**权限等级：** 2

#### KillCommand - /kill 命令

杀死实体。

**用法：**

- `/kill` - 杀死自己
- `/kill <target>` - 杀死指定实体

**实现状态：** ✅ 完整实现

**实现细节：**

- 使用 `Entity::onKillCommand()` 方法处理实体死亡
- `LivingEntity` 重写 `onKillCommand()` 使用虚空伤害（`Float.MAX_VALUE`）确保完整死亡流程
- 通过 `ServerPlayerEntityManager` 获取玩家实体

**权限等级：** 2

#### ListCommand - /list 命令

列出在线玩家。

**用法：**

- `/list` - 显示当前服务器上的玩家数量

**权限等级：** 0（所有玩家可用）

#### MessageCommand - /msg 命令

发送私聊消息给其他玩家。

**用法：**

- `/msg <target> <message>` - 发送私聊消息
- `/tell <target> <message>` - `/msg` 的别名
- `/w <target> <message>` - `/msg` 的别名

**实现状态：** ✅ 完整实现

**实现细节：**

- 通过 `ServerPlayerEntityManager` 获取目标玩家的 `Player` 实体
- 使用 `ConnectionManager::sendPacketToPlayer()` 发送私聊消息
- 消息格式符合 MC 1.16.5 规范：`§7§o<sender> whispers to you: <message>`
- 发送者收到格式：`§7§oYou whisper to <target>: <message>`

**权限等级：** 0（所有玩家可用）

#### TellRawCommand - /tellraw 命令

发送原始 JSON 消息给玩家。

**用法：**

- `/tellraw <target> <json message>` - 发送 JSON 格式的富文本消息

**实现状态：** ✅ 完整实现

**实现细节：**

- 使用 `nlohmann::json` 解析 JSON 消息内容
- 通过 `ChatMessagePacket` 发送 JSON 格式消息给目标玩家
- 支持完整的 ITextComponent JSON 格式（颜色、样式、翻译键等）
- JSON 解析失败时返回错误信息
- 只允许指定单个玩家（不支持多玩家选择器如 @a）

**JSON 格式示例：**

```json
/tellraw Steve {"text":"Hello","color":"red","bold":true}
/tellraw @p {"translate":"chat.type.announcement","with":[{"text":"Server"},{"text":"Welcome!"}]}
```

**权限等级：** 2

#### SaveOnCommand / SaveOffCommand - /save-on 与 /save-off 命令

切换服务器自动保存开关。

**用法：**

- `/save-on` - 启用自动保存
- `/save-off` - 禁用自动保存

**权限等级：** 4

**实现状态：**

- 已接入 `ServerWorld` 内部的 `SaveManager`
- 命令层只负责调用保存协调器的 `startAutoSave()` / `stopAutoSave()`

#### HelpCommand - /help 命令

显示命令帮助。

**用法：**

- `/help` - 显示所有可用命令
- `/help <command>` - 显示指定命令的详细帮助

**权限等级：** 0（所有玩家可用）

#### SeedCommand - /seed 命令

显示世界种子。

**用法：**

- `/seed` - 显示当前世界的种子

**权限等级：** 2

#### TeleportCommand - /tp 命令

传送实体。

**用法：**

- `/tp <target>` - 传送到目标实体
- `/tp <x> <y> <z>` - 传送到坐标
- `/tp <target> <destination>` - 将目标传送到目的地
- `/tp <target> <x> <y> <z>` - 将目标传送到坐标
- `/teleport` 是 `tp` 的重定向别名，避免重复维护同一棵子树

**权限等级：** 2

#### GiveCommand - /give 命令

给予玩家物品。

**用法：**

- `/give <player> <item> [count]` - 给予指定玩家物品

**权限等级：** 2

#### ExperienceCommand - /experience 命令

管理玩家经验值。

**用法：**

- `/experience add <targets> <amount> [points|levels]` - 增加经验
- `/experience set <targets> <amount> [points|levels]` - 设置经验
- `/experience query <targets> [points|levels]` - 查询经验
- `/xp` 是 `/experience` 的重定向别名，避免重复维护同一棵树

**实现状态：** ✅ 完整实现

**实现细节：**

- 通过 `ServerPlayerEntityManager` 获取目标玩家的 `Player` 实体
- 支持 `add`、`set`、`query` 子命令
- 支持 `points` 和 `levels` 两种经验单位
- `add` 支持负数值（减少经验）
- `set` 仅允许非负值

**权限等级：** 2

#### ClearCommand - /clear 命令

清空玩家背包。

**用法：**

- `/clear` - 清空自己的背包
- `/clear <player>` - 清空指定玩家的背包
- `/clear <player> <item>` - 清空指定玩家的指定物品
- `/clear <player> <item> <maxCount>` - 清空指定物品，限制数量

清空逻辑现在通过 `IServer::playerInventory()` 统一获取单机/联机库存，命令层不再直接依赖 `IntegratedServer`。

**权限等级：** 2

#### CloneCommand - /clone 命令

复制方块区域到另一个位置。

**用法：**

- `/clone <begin> <end> <destination>` - 复制区域（默认 replace + normal 模式）
- `/clone <begin> <end> <destination> replace` - 复制所有方块
- `/clone <begin> <end> <destination> masked` - 只复制非空气方块
- `/clone <begin> <end> <destination> filtered <block>` - 只复制指定方块
- `/clone <begin> <end> <destination> [filter] force` - 强制复制（允许重叠）
- `/clone <begin> <end> <destination> [filter] move` - 移动模式（复制后清空源区域）

**参数：**

- `begin` - 源区域起始坐标
- `end` - 源区域结束坐标
- `destination` - 目标位置坐标
- `filter` - 过滤模式：`replace`（全部）、`masked`（非空气）、`filtered`（指定方块）
- `mode` - 执行模式：`normal`（禁止重叠）、`force`（允许重叠）、`move`（移动）

**过滤模式说明：**

| 模式 | 说明 |
|------|------|
| `replace` | 复制所有方块（包括空气） |
| `masked` | 只复制非空气方块 |
| `filtered` | 只复制匹配指定方块的方块 |

**执行模式说明：**

| 模式 | 说明 |
|------|------|
| `normal` | 源区域和目标区域不能重叠（默认） |
| `force` | 允许源区域和目标区域重叠 |
| `move` | 复制后清空源区域 |

**实现状态：** ✅ 完整实现

**实现细节：**

- 使用 `IWorld::hasChunk()` 检查区域是否已加载
- 使用 `IWorld::getBlockState()` 和 `IWorld::setBlockState()` 进行方块操作
- 使用 `BlockEntity::save()` 和 `BlockEntity::load()` 保存/恢复方块实体数据
- 使用 `IInventory::clear()` 在 move 模式下清空容器内容
- 支持方块状态过滤（filtered 模式）
- 限制最多复制 32768 个方块（MC 1.16.5 标准）
- 方块放置顺序：普通方块 → 方块实体 → 透明方块（反转顺序避免更新问题）

**权限等级：** 2

#### WeatherCommand - /weather 命令

控制天气。

**用法：**

- `/weather clear [duration]` - 设置晴天
- `/weather rain [duration]` - 设置降雨
- `/weather thunder [duration]` - 设置雷暴
- `/weather query` - 查询当前天气

**参数：**

- `duration` - 持续时间（ticks），1秒 = 20 ticks
- 不指定 duration 时默认为 6000 ticks（5分钟）

**权限等级：** 2

#### MeCommand - /me 命令

显示玩家动作消息。

**用法：**

- `/me <action>` - 在聊天中显示动作消息

**权限等级：** 0（所有玩家可用）

#### ParticleCommand - /particle 命令

显示粒子效果。

**用法：**

- `/particle <name>` - 在当前位置显示粒子
- `/particle <name> <pos>` - 在指定位置显示粒子

**支持参数（MC 1.16.5 完整参数待实现）：**

当前实现支持粒子名称和位置参数，广播范围为 256 格。

**粒子类型：**

支持 60+ 种粒子类型，包括：flame, smoke, lava, portal, explosion, crit, heart, redstone 等。

**权限等级：** 2

**实现状态：** ✅ 完整实现

#### SetBlockCommand - /setblock 命令

在指定位置放置或替换方块。

**用法：**

- `/setblock <pos> <block>` - 在指定位置放置方块（默认 replace 模式）
- `/setblock <pos> <block> destroy` - 破坏原有方块并掉落物品，然后放置新方块
- `/setblock <pos> <block> keep` - 仅当目标位置为空气时放置
- `/setblock <pos> <block> replace` - 直接替换目标位置的方块

**模式说明：**

| 模式 | 说明 |
|------|------|
| `destroy` | 破坏原有方块，触发掉落物和经验，播放破坏效果，然后放置新方块 |
| `keep` | 仅当目标位置为空气时才放置新方块 |
| `replace`（默认） | 直接替换目标位置的方块，不掉落物品 |

**destroy 模式行为：**

1. 检查目标位置是否为空气（空气不触发任何效果）
2. 播放方块破坏效果（WorldEvents::BREAK_BLOCK_EFFECTS，事件 ID 2001）
3. 从方块掉落表生成物品实体
4. 处理矿石经验掉落（煤矿、钻石矿、绿宝石矿等）
5. 放置新方块

**权限等级：** 2

**实现状态：** ✅ 完整实现

#### LocateCommand - /locate 命令

定位最近的建筑结构。

**用法：**

- `/locate <structure>` - 定位指定类型的建筑结构

**权限等级：** 0（所有玩家可用）

#### LocateBiomeCommand - /locatebiome 命令

定位最近的生物群系。

**用法：**

- `/locatebiome <biome>` - 定位指定类型的生物群系

**权限等级：** 0（所有玩家可用）

#### AttributeCommand - /attribute 命令

查询或修改实体属性。

**用法：**

- `/attribute <target> <attribute> get` - 获取属性值
- `/attribute <target> <attribute> set <value>` - 设置属性基础值

**实现状态：** ✅ 完整实现

**实现细节：**

- 通过 `ServerPlayerEntityManager` 获取目标玩家的 `Player` 实体
- 从 `Player::attributes()` 获取 `AttributeMap` 进行属性读写
- 支持 `generic.` 前缀属性（如 `generic.max_health`）
- 支持简写属性名（如 `max_health` 自动添加 `generic.` 前缀）
- 支持 `horse.jump_strength` 等特殊属性
- 属性值范围验证（如 `knockback_resistance` 限制在 0-1）
- 未知属性返回错误信息

**权限等级：** 2

#### EnchantCommand - /enchant 命令

给玩家手持物品添加附魔。

**用法：**

- `/enchant <target> <enchantment> [level]` - 给目标玩家的手持物品添加附魔

**参数说明：**

- `<target>` - 目标玩家选择器（仅支持单个玩家，如 `@p` 或玩家名）
- `<enchantment>` - 附魔名称（支持简写如 `sharpness` 或完整名称如 `minecraft:sharpness`）
- `[level]` - 附魔等级（可选，默认为 1，范围为 0-32767）

**附魔类型：**

| 类型 | 附魔 |
|------|------|
| 武器 | sharpness, smite, bane_of_arthropods, knockback, fire_aspect, looting, sweeping |
| 工具 | efficiency, silk_touch, fortune, unbreaking |
| 护甲 | protection, fire_protection, blast_protection, projectile_protection, feather_falling, thorns, respiration, depth_strider, aqua_affinity |
| 弓 | power, punch, flame, infinity |
| 弩 | multishot, piercing, quick_charge |
| 三叉戟 | loyalty, riptide, channeling, impaling |
| 钓鱼竿 | luck_of_the_sea, lure |
| 宝藏附魔 | mending, frost_walker, soul_speed |
| 诅咒 | binding_curse, vanishing_curse |

**实现状态：** ✅ 完整实现

**实现细节：**

- 通过 `ServerPlayerEntityManager` 获取目标玩家的 `Player` 实体
- 使用 `PlayerInventory::getSelectedStack()` 获取主手物品
- 使用 `EnchantmentRegistry::get()` 查找附魔实例
- 检查附魔是否可应用于物品（`Enchantment::canApply()`）
- 检查与现有附魔的兼容性（`Enchantment::isCompatibleWith()`）
- 检查附魔等级是否有效（不超过最大等级）
- 使用 `ItemStack::addEnchantment()` 应用附魔
- 支持简写附魔名称（自动添加 `minecraft:` 前缀）
- 错误反馈：无物品、附魔不兼容、附魔不存在、等级过高

**权限等级：** 2

#### BanCommand - /ban 命令

封禁玩家。

**用法：**

- `/ban <target> [reason]` - 封禁指定玩家
- `/ban <target> [reason] [time]` - 封禁指定玩家一段时间

**参数说明：**

- `<target>` - 目标玩家名称
- `[reason]` - 封禁原因（可选）
- `[time]` - 封禁时长（可选，如 `1d`、`7d`、`30d` 等）

**实现状态：** ✅ 完整实现

**实现细节：**

- 通过 `IServer::bannedPlayerList()` 访问封禁列表管理器
- 支持在线玩家封禁（自动踢出）
- 支持离线玩家封禁（按名称封禁）
- 封禁信息持久化到 `banned-players.json`
- 自动记录封禁时间、执行者等信息

**权限等级：** 3

#### BanIpCommand - /ban-ip 命令

封禁 IP 地址。

**用法：**

- `/ban-ip <target> [reason]` - 封禁指定 IP 地址
- `/ban-ip <player> [reason]` - 封禁指定在线玩家的 IP

**参数说明：**

- `<target>` - IP 地址或在线玩家名称
- `[reason]` - 封禁原因（可选）

**实现状态：** ✅ 完整实现

**实现细节：**

- 支持 IP 地址格式验证
- 支持通过玩家名称获取 IP 地址
  - 使用 `PlayerManager::findByUsername()` 按用户名查找玩家（大小写不敏感）
  - 从 `ServerPlayerData::ipAddress` 获取玩家的 IP 地址
  - 使用 `PlayerManager::getPlayerIdsByAddress()` 获取同一 IP 的所有在线玩家
- 本地连接（集成服务器）的玩家 IP 地址为空字符串
- 封禁信息持久化到 `banned-ips.json`
- 自动记录封禁时间、执行者等信息

**权限等级：** 3

#### PardonCommand - /pardon 命令

解除玩家封禁。

**用法：**

- `/pardon <target>` - 解除指定玩家的封禁
- `/unban <target>` - `/pardon` 的别名

**参数说明：**

- `<target>` - 目标玩家名称

**实现状态：** ✅ 完整实现

**实现细节：**

- 通过 `IServer::bannedPlayerList()` 访问封禁列表管理器
- 名称匹配不区分大小写（MC 1.16.5 行为）
- 自动保存封禁列表

**权限等级：** 3

#### PardonIpCommand - /pardon-ip 命令

解除 IP 封禁。

**用法：**

- `/pardon-ip <ip>` - 解除指定 IP 的封禁
- `/unban-ip <ip>` - `/pardon-ip` 的别名

**参数说明：**

- `<ip>` - IP 地址

**实现状态：** ✅ 完整实现

**实现细节：**

- 通过 `IServer::bannedIpList()` 访问 IP 封禁列表管理器
- 自动保存封禁列表

**权限等级：** 3

#### BanListCommand - /banlist 命令

显示封禁列表。

**用法：**

- `/banlist` - 显示所有封禁（玩家和 IP）
- `/banlist players` - 仅显示玩家封禁
- `/banlist ips` - 仅显示 IP 封禁

**实现状态：** ✅ 完整实现

**实现细节：**

- 通过 `IServer::bannedPlayerList()` 和 `IServer::bannedIpList()` 获取封禁列表
- 按名称/IP 排序后显示
- 返回封禁数量作为命令结果

**权限等级：** 3

### P3 命令（高级功能）

以下命令实现了骨架框架，带有 TODO 标记的功能需要后续集成：

#### AdvancementCommand - /advancement 命令

管理玩家进度。

**用法：**

- `/advancement grant <targets> everything` - 授予所有进度
- `/advancement grant <targets> only <advancement>` - 授予特定进度
- `/advancement revoke <targets> everything` - 撤销所有进度
- `/advancement revoke <targets> only <advancement>` - 撤销特定进度
- `/advancement test <targets> <advancement>` - 测试进度完成状态

**权限等级：** 2

#### BossBarCommand - /bossbar 命令

创建和管理 Boss 生命条。

**用法：**

- `/bossbar add <id> <name>` - 创建新的 Boss 条
- `/bossbar remove <id>` - 移除 Boss 条
- `/bossbar list` - 列出所有 Boss 条
- `/bossbar set <id> <property> <value>` - 设置属性
- `/bossbar get <id> <property>` - 获取属性值

**权限等级：** 2

#### DataPackCommand - /datapack 命令

管理数据包。

**用法：**

- `/datapack list` - 列出已加载的数据包
- `/datapack enable <name>` - 启用数据包
- `/datapack disable <name>` - 禁用数据包

**权限等级：** 2

#### ForceLoadCommand - /forceload 命令

强制加载区块，使其始终加载而不受玩家视距影响。

**用法：**

- `/forceload add <from> [to]` - 添加强制加载区块（支持范围选择）
- `/forceload remove <from> [to]` - 移除强制加载区块（支持范围选择）
- `/forceload remove all` - 移除当前维度所有强制加载区块
- `/forceload query [<pos>]` - 查询单个区块是否强制加载，或列出所有强制加载区块

**参数：**

- `from` - 起始坐标（方块坐标）
- `to` - 结束坐标（可选，用于范围选择）
- `pos` - 查询坐标（可选，不指定则列出所有）

**限制：**

- 单次操作最多 256 个区块
- 坐标必须在世界边界内 [-30000000, 30000000)
- 每个维度独立管理强制加载区块

**示例：**

- `/forceload add 0 64 0` - 强制加载区块 (0, 0)
- `/forceload add 0 64 0 100 64 100` - 强制加载从 (0, 0) 到 (6, 6) 的区块范围
- `/forceload remove 0 64 0` - 移除区块 (0, 0) 的强制加载
- `/forceload remove all` - 移除当前维度所有强制加载区块
- `/forceload query` - 列出当前维度所有强制加载区块
- `/forceload query 0 64 0` - 查询区块 (0, 0) 是否被强制加载

**权限等级：** 2

**实现状态：** ✅ 完整实现

**注意：** 强制加载区块在服务器重启后会丢失（当前未实现持久化）

#### LootCommand - /loot 命令

从战利品表生成物品。

**用法：**

- `/loot give <targets> <loot_table>` - 给予玩家战利品
- `/loot spawn <pos> <loot_table>` - 在位置生成战利品
- `/loot insert <pos> <loot_table>` - 插入战利品到容器
- `/loot replace <entity|block> <target> <slot> <loot_table>` - 替换物品

**权限等级：** 2

#### PublishCommand - /publish 命令

将单人世界开放到局域网。

**用法：**

- `/publish [port] [allowCheats]` - 开放到局域网

**权限等级：** 4

#### RecipeCommand - /recipe 命令

管理玩家配方解锁。

**用法：**

- `/recipe give <targets> <recipe|*>` - 授予配方
- `/recipe take <targets> <recipe|*>` - 移除配方

**权限等级：** 2

#### ReloadCommand - /reload 命令

重新加载数据包和资源。

**用法：**

- `/reload` - 重新加载所有数据

**权限等级：** 2

#### ReplaceItemCommand - /replaceitem 命令

替换实体或容器的物品。

**用法：**

- `/replaceitem entity <targets> <slot> <item> [count]` - 替换实体物品
- `/replaceitem block <pos> <slot> <item> [count]` - 替换方块容器物品

**权限等级：** 2

#### ScheduleCommand - /schedule 命令

延迟执行函数。

**用法：**

- `/schedule function <function> <time> [append|replace]` - 调度函数
- `/schedule clear <function>` - 清除调度

**权限等级：** 2

#### ScoreboardCommand - /scoreboard 命令

管理计分板。

**用法：**

- `/scoreboard objectives <add|remove|list|setdisplay> ...` - 管理目标
- `/scoreboard players <add|remove|set|reset|get|list> ...` - 管理分数

**权限等级：** 2

#### SpectateCommand - /spectate 命令

让玩家以旁观者模式观看其他实体。

**用法：**

- `/spectate <target> [player]` - 开始旁观
- `/spectate stop [player]` - 停止旁观

**权限等级：** 2

#### SpreadPlayersCommand - /spreadplayers 命令

将玩家随机分散到区域内。

**用法：**

- `/spreadplayers <center> <spreadDistance> <maxRange> <respectTeams> <targets>` - 分散玩家

**参数：**

- `center` - 分散区域的中心坐标（Vec2）
- `spreadDistance` - 玩家之间的最小距离
- `maxRange` - 从中心到边缘的最大范围
- `respectTeams` - 是否保持队伍成员在一起
- `targets` - 要分散的目标玩家选择器

**权限等级：** 2

**实现状态：** ✅ 基本实现

**实现细节：**

- 使用 `IWorld::getHeight()` 获取分散位置的地面高度
- 随机分散玩家到指定区域内
- 支持 @a、@p 等选择器

#### SpawnPointCommand - /spawnpoint 命令

设置玩家的重生点。

**用法：**

- `/spawnpoint` - 设置自己的重生点到当前位置
- `/spawnpoint <player>` - 设置指定玩家的重生点到其当前位置
- `/spawnpoint <player> <pos>` - 设置指定玩家的重生点到指定位置

**参数：**

- `player` - 目标玩家选择器（可选，默认为执行者）
- `pos` - 重生点坐标（可选，默认为玩家当前位置）

**权限等级：** 2

**实现状态：** ✅ 完整实现

**实现细节：**

- 通过 `ServerPlayerEntityManager::getPlayerEntity()` 获取任意玩家的实体
- 支持为任意目标玩家设置重生点，不再限于只能设置自己
- 使用 `Player::setSpawnPoint()` 设置维度和坐标
- 支持 @a、@p、玩家名等选择器

#### TagCommand - /tag 命令

管理实体标签。

**用法：**

- `/tag <targets> add <tag>` - 添加标签
- `/tag <targets> remove <tag>` - 移除标签
- `/tag <targets> list` - 列出标签

**权限等级：** 2

**实现状态：** ✅ 完整实现

**标签系统**：
- 每个实体最多 1024 个标签（参考 MC 1.16.5）
- 标签存储在 `Entity::m_tags` 中（`std::set<std::string>`）
- 支持玩家实体，未来可扩展支持所有实体

#### TeamCommand - /team 命令

管理队伍。

**用法：**

- `/team add <team> [displayName]` - 创建队伍
- `/team remove <team>` - 移除队伍
- `/team list [team]` - 列出队伍
- `/team join <team> <members>` - 加入队伍
- `/team leave <members>` - 离开队伍
- `/team modify <team> <property> <value>` - 修改属性

**权限等级：** 2

#### TriggerCommand - /trigger 命令

修改触发器计分板目标。

**用法：**

- `/trigger <objective>` - 触发目标
- `/trigger <objective> add <value>` - 增加值
- `/trigger <objective> set <value>` - 设置值

**权限等级：** 0（所有玩家可用）

#### WorldBorderCommand - /worldborder 命令

管理世界边界。

**用法：**

- `/worldborder set <size> [time]` - 设置边界大小
- `/worldborder center <pos>` - 设置边界中心
- `/worldborder add <size> [time]` - 增加边界大小
- `/worldborder get` - 获取边界大小

**权限等级：** 2

## 模块整体分析

### 整体职责

`server/command` 模块负责：

1. **命令注册** - 管理所有服务端命令的注册
2. **命令分发** - 解析命令字符串并路由到对应的处理器
3. **命令执行** - 执行命令逻辑并返回结果
4. **权限控制** - 检查命令执行者的权限等级
5. **反馈发送** - 向命令源发送执行结果消息

### 输入和输出

**输入：**

- 命令字符串（如 `/gamemode creative`）
- `ServerCommandSource`（命令执行者信息）

**输出：**

- `Result<i32>` - 命令执行结果（成功返回结果码，失败返回错误）
- 通过 `ServerCommandSource::sendMessage()` 发送给玩家的消息

### 依赖项

**内部依赖：**

```
server/command
├── common/command/           # 命令框架核心
│   ├── CommandDispatcher.hpp # 命令分发器
│   ├── CommandContext.hpp    # 命令上下文
│   ├── CommandNode.hpp       # 命令节点
│   ├── CommandResult.hpp     # 执行结果
│   ├── StringReader.hpp      # 字符串解析器
│   ├── ICommandSource.hpp    # 命令源接口
│   ├── arguments/            # 参数类型
│   │   ├── ArgumentType.hpp  # 基础参数类型
│   │   ├── EntityArgument.hpp# 实体选择器
│   │   ├── GameModeArgument.hpp # 游戏模式参数
│   │   └── ItemArgument.hpp  # 物品参数
│   └── exceptions/           # 命令异常
│       └── CommandExceptions.hpp
├── server/application/       # 服务器核心
│   ├── IServer.hpp          # 服务器接口
│   └── MinecraftServer.hpp  # 服务器实现
├── server/core/             # 核心管理器
│   ├── PlayerManager.hpp    # 玩家管理
│   ├── TimeManager.hpp      # 时间管理
│   ├── TeleportManager.hpp  # 传送管理
│   └── GameModeManager.hpp  # 游戏模式管理
├── server/player/           # 玩家
│   └── ServerPlayer.hpp     # 服务端玩家
├── server/world/            # 世界
│   ├── ServerWorld.hpp      # 服务端世界
│   └── weather/
│       └── WeatherManager.hpp # 天气管理
└── common/item/
    └── ItemStack.hpp        # 物品堆
```

### 使用方法

**1. 获取命令注册表：**

```cpp
auto& registry = mc::command::CommandRegistry::getGlobal();
```

**2. 创建命令源：**

玩家命令源：

```cpp
mc::command::ServerCommandSource source(
    server,          // IServer 指针
    player,          // ServerPlayer 指针
    world,           // ServerWorld 指针
    player->position(),
    player->rotation(),
    2,               // 权限等级
    player->playerId(),
    player->username()
);
```

控制台命令源：

```cpp
auto source = mc::command::ServerCommandSource::forConsole(server);
```

**3. 执行命令：**

```cpp
auto result = registry.execute("/gamemode creative", source);
if (result.success()) {
    spdlog::info("命令执行成功，结果码: {}", result.value());
} else {
    spdlog::error("命令执行失败: {}", result.error().message());
}
```

**4. 注册自定义命令：**

```cpp
class MyCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
        auto node = std::make_shared<LiteralCommandNode<ServerCommandSource>>("mycommand");
        node->setRequirement([](const ServerCommandSource& source) {
            return source.hasPermission(2);
        });
        node->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            ctx.getSource().sendMessage("My command executed!");
            return 1;
        });
        dispatcher.registerCommand(node);
    }
};

// 在 CommandRegistry::registerDefaults() 中添加
MyCommand::registerTo(m_dispatcher);
```

### 容易踩的坑

1. **权限等级检查遗漏**
   - 问题：忘记设置命令的权限要求
   - 解决：始终为命令节点设置 `setRequirement` 检查权限

2. **命令源类型检查**
   - 问题：在非玩家命令源上调用 `assertPlayer()`
   - 解决：先使用 `isPlayer()` 检查，或捕获 `CommandException`

3. **参数类型不匹配**
   - 问题：使用 `getArgument<T>()` 时类型与注册时不一致
   - 解决：确保模板参数与 `ArgumentCommandNode` 的类型一致

4. **服务器指针为空**
   - 问题：在命令执行时 `source.server()` 返回 nullptr
   - 解决：始终检查服务器指针是否有效

5. **EntitySelector 未实现**
   - 问题：`EntityArgument` 返回选择器但解析逻辑未完成
   - 解决：当前使用占位符实现，完整实现需要实体选择器解析系统

6. **命令反馈未发送**
   - 问题：命令执行后玩家看不到反馈
   - 解决：确保通过 `source.sendMessage()` 发送反馈消息

7. **帮助信息硬编码**
   - 问题：`HelpCommand` 中的帮助信息是硬编码的
   - 解决：添加新命令时需同步更新 `s_commandHelp` 数组

8. **玩家命令反馈依赖**

**问题**：`ServerCommandSource::sendMessage()` 在只有 `playerId` 的情况下需要能够发送消息给在线连接。

**解决方案**：玩家命令反馈不要默认依赖 `ServerPlayer*`。`ServerCommandSource::sendMessage()` 现在必须在只有 `playerId` 的情况下也能把消息发回在线连接，不能只写日志。

### 测试用例

**命令框架测试** 位于 `tests/common/command/test_command_dispatcher.cpp`：

- `StringReader` - 字符串解析（读取字符串、整数、浮点数、布尔值）
- `CommandNode` - 节点创建、子节点管理、权限检查
- `ArgumentType` - 各类型参数解析（字符串、整数、浮点、布尔、枚举）
- `CommandResult` - 成功/失败结果处理
- `CommandException` - 异常创建和传递
- `Suggestions` - 自动补全建议
- `CommandDispatcher` - 命令注册、解析、执行

**命令测试** 位于 `tests/server/command/`：

- `CommandRegistryTest.cpp` - 命令注册表测试
- `PlayerResolverTest.cpp` - 玩家选择器解析测试（IntRange、游戏模式过滤、距离过滤等）
- `SetBlockCommandTest.cpp` - /setblock 命令测试
- `CloneCommandTest.cpp` - /clone 命令测试（命令注册、权限检查、过滤模式、执行模式）
- `GiveCommandTest.cpp` - /give 命令测试
- `MessageCommandTest.cpp` - /msg 命令测试（命令注册、权限检查、别名）
- `TellRawCommandTest.cpp` - /tellraw 命令测试（命令注册、JSON 解析、消息发送）
- `ExperienceCommandTest.cpp` - /experience 命令测试（add/set/query 语法、xp 别名）
- `AttributeCommandTest.cpp` - /attribute 命令测试（get/set 语法、属性名解析）
- `EnchantCommandTest.cpp` - /enchant 命令测试（命令注册、权限检查、附魔解析、附魔兼容性、等级边界）

**运行测试：**

```powershell
./build/bin/Release/mc_tests.exe --gtest_filter="Command*"
```

## 架构图

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
│  │  │      │       │       │       │             │    │   │
│  │  │  ┌───┴───┐   │       │       │             │    │   │
│  │  │  │ mode  │   │       │       │             │    │   │
│  │  │  │  arg  │   │       │       │             │    │   │
│  │  │  └───────┘   │       │       │             │    │   │
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
│  │ServerWorld* │  │  position   │  │     rotation        │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      Command Handlers                        │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────┐ │
│  │GameModeCommand│ │ TimeCommand │ │   WeatherCommand     │ │
│  └──────────────┘ └──────────────┘ └──────────────────────┘ │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────┐ │
│  │TeleportCommand│ │ GiveCommand │ │    ClearCommand      │ │
│  └──────────────┘ └──────────────┘ └──────────────────────┘ │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────┐ │
│  │ KillCommand  │ │ ListCommand │ │ SeedCommand/HelpCmd  │ │
│  └──────────────┘ └──────────────┘ └──────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## 命令执行流程

```
玩家输入 "/gamemode creative"
         │
         ▼
┌─────────────────────┐
│  ChatMessagePacket  │
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│  MinecraftServer    │
│  handleChatMessage()│
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│  CommandRegistry    │
│  execute()          │
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│  CommandDispatcher  │
│  parse()            │──── 解析命令字符串
│  execute()          │──── 执行命令节点
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│  GameModeCommand    │
│  setGameModeSelf()  │──── 实际命令逻辑
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│  GameModeManager    │
│  setGameMode()      │──── 更新玩家游戏模式
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│ ServerCommandSource │
│ sendMessage()       │──── 发送反馈消息
└─────────────────────┘
```

## 参考

本模块参考 Minecraft Java Edition 1.16.5 的命令系统设计：

- `CommandDispatcher` - 对应 MC 的 `com.mojang.brigadier.CommandDispatcher`
- `CommandNode` - 对应 MC 的 `com.mojang.brigadier.tree.CommandNode`
- `CommandContext` - 对应 MC 的 `com.mojang.brigadier.context.CommandContext`
- `ServerCommandSource` - 对应 MC 的 `net.minecraft.command.CommandSource`
- 各命令类 - 对应 MC 的 `net.minecraft.command.impl.*`

## 扩展指南

### 添加新命令

1. 在 `commands/` 目录创建新的命令类：

```cpp
// MyCommand.hpp
#pragma once
#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

class MyCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 execute(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
```

2. 实现命令逻辑：

```cpp
// MyCommand.cpp
#include "MyCommand.hpp"
#include "common/command/CommandContext.hpp"

namespace mc {
namespace command {

void MyCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto node = std::make_shared<LiteralCommandNode<ServerCommandSource>>("mycommand");
    node->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);  // 权限等级 2
    });
    node->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return execute(ctx);
    });
    dispatcher.registerCommand(node);
}

i32 MyCommand::execute(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    source.sendMessage("My command executed!");
    return 1;  // 成功结果码
}

} // namespace command
} // namespace mc
```

3. 在 `CommandRegistry.cpp` 中注册：

```cpp
#include "commands/MyCommand.hpp"

void CommandRegistry::registerDefaults() {
    // ... 其他命令
    MyCommand::registerTo(m_dispatcher);

    m_commandNames.push_back("mycommand");
    m_commandNameSet.insert("mycommand");
}
```

4. 在 `HelpCommand.cpp` 的 `s_commandHelp` 数组中添加帮助信息：

```cpp
{"mycommand", "Description of my command", "/mycommand [args]"},
```
