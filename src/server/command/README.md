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
└── commands/                  # 具体命令实现
    ├── ClearCommand.hpp       # /clear 命令
    ├── ClearCommand.cpp
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
    Result<i32> execute(const String& input, ServerCommandSource& source);

    // 获取建议
    std::future<Suggestions> getSuggestions(const String& input, ServerCommandSource& source);

    // 注册默认命令
    void registerDefaults();

    // 命令查询
    std::vector<String> getCommandNames() const;
    bool hasCommand(const String& name) const;

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
        String playerName = ""
    );

    // ICommandSource 接口
    void sendMessage(const String& message, const std::optional<Uuid>& senderUuid = std::nullopt) override;
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
    const String& name() const noexcept;

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

**权限等级：** 2

#### ListCommand - /list 命令

列出在线玩家。

**用法：**

- `/list` - 显示当前服务器上的玩家数量

**权限等级：** 0（所有玩家可用）

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

**权限等级：** 2

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

### 测试用例

相关测试位于 `tests/common/command/test_command_dispatcher.cpp`：

**测试覆盖：**

- `StringReader` - 字符串解析（读取字符串、整数、浮点数、布尔值）
- `CommandNode` - 节点创建、子节点管理、权限检查
- `ArgumentType` - 各类型参数解析（字符串、整数、浮点、布尔、枚举）
- `CommandResult` - 成功/失败结果处理
- `CommandException` - 异常创建和传递
- `Suggestions` - 自动补全建议
- `CommandDispatcher` - 命令注册、解析、执行

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
