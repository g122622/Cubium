# 命令实现子目录

这里放的是服务端内置命令的具体实现。这个目录只负责把解析后的命令树节点绑定到执行逻辑，不负责命令注册入口，也不负责服务器生命周期。

## 目录结构

```text
src/server/command/commands/
├── ClearCommand.hpp / ClearCommand.cpp
├── DefaultGameModeCommand.hpp / DefaultGameModeCommand.cpp
├── DifficultyCommand.hpp / DifficultyCommand.cpp
├── ExperienceCommand.hpp / ExperienceCommand.cpp
├── GameModeCommand.hpp / GameModeCommand.cpp
├── GiveCommand.hpp / GiveCommand.cpp
├── HelpCommand.hpp / HelpCommand.cpp
├── KickCommand.hpp / KickCommand.cpp
├── KillCommand.hpp / KillCommand.cpp
├── ListCommand.hpp / ListCommand.cpp
├── SayCommand.hpp / SayCommand.cpp
├── SeedCommand.hpp / SeedCommand.cpp
├── SetIdleTimeoutCommand.hpp / SetIdleTimeoutCommand.cpp
├── SetWorldSpawnCommand.hpp / SetWorldSpawnCommand.cpp
├── SpawnPointCommand.hpp / SpawnPointCommand.cpp
├── StopCommand.hpp / StopCommand.cpp
├── TeleportCommand.hpp / TeleportCommand.cpp
├── TimeCommand.hpp / TimeCommand.cpp
└── WeatherCommand.hpp / WeatherCommand.cpp
```

## 文件介绍

- `ClearCommand.*`：清空背包，按玩家查询库存时走 `IServer::playerInventory()`。
- `DefaultGameModeCommand.*`：设置服务器默认游戏模式。
- `DifficultyCommand.*`：修改世界难度。
- `ExperienceCommand.*`：管理经验值和等级。
- `GameModeCommand.*`：切换玩家游戏模式。
- `GiveCommand.*`：发放物品。
  - 背包满时在玩家位置掉落物品实体（参考 MC 1.16.5）
  - 设置掉落物品的 owner UUID 和无拾取延迟
  - 播放拾取音效，音调公式：`((random - random) * 0.7 + 1.0) * 2.0`
- `HelpCommand.*`：展示命令帮助。
- `KickCommand.*`：踢出在线玩家。
- `KillCommand.*`：杀死实体或自己。
- `ListCommand.*`：列出在线玩家。
- `SayCommand.*`：广播聊天消息。
- `SeedCommand.*`：显示当前世界种子。
- `SetIdleTimeoutCommand.*`：设置玩家挂机超时。
- `SetWorldSpawnCommand.*`：设置世界出生点（指南针指向位置）。
  - 设置 Dimension 和 ServerWorld 的出生点
  - 广播 SpawnPositionPacket 给所有在线玩家
- `SpawnPointCommand.*`：设置玩家个人重生点。
- `StopCommand.*`：请求服务器停机。
- `TeleportCommand.*`：处理 `/tp` 与 `/teleport` 的传送逻辑。
- `TimeCommand.*`：修改或查询游戏时间。
- `WeatherCommand.*`：修改或查询天气状态。

## 模块关系

- 这些实现都被 `CommandRegistry::registerDefaults()` 注册到命令树。
- 每个命令通过 `ServerCommandSource` 读取权限、玩家、世界和反馈输出。
- `support/` 目录提供共享的元数据、参数解析、玩家解析等辅助逻辑。
- `/clear` 通过 `IServer` 的抽象库存接口工作，不再直接绑定具体服务器实现。

## 整体职责

该目录负责把 Minecraft 风格的命令行为落到具体执行逻辑上，包括参数解析、权限校验、目标解析、消息反馈和对服务器状态的修改。

## 输入 / 输出

- 输入：命令字符串、命令源、目标选择器、参数节点、服务器状态。
- 输出：聊天反馈、玩家状态变更、时间/天气/游戏模式修改、背包修改、传送请求和服务器停机请求。

## 依赖项

- 内部依赖：`server/command/CommandRegistry`、`server/command/ServerCommandSource`、`server/command/support/*`、`server/application/IServer`、`server/core/*`、`server/player/ServerPlayer`。
- 共享依赖：`common/command/*`、`common/entity/inventory/*`、`common/item/*`、`common/network/packet/*`。
- 外部依赖：`spdlog`、`GTest`（测试场景）。

## 使用方法

```cpp
ClearCommand::registerTo(dispatcher);
```

通常不直接调用具体命令函数，而是通过 `CommandRegistry::registerDefaults()` 或单独的 `registerTo()` 把子树挂到总命令树上。

## 容易踩的坑

- 不要在命令里直接 `dynamic_cast` 到 `IntegratedServer`，统一通过 `IServer::playerInventory()` 取库存。
- 命令元数据和实际语法要同步更新，否则帮助输出和补全会偏离。
- `/tp` 和 `/teleport`、`/experience` 和 `/xp` 这类别名应继续使用重定向共享同一棵树。
- 目标选择器解析要依赖共享的 support 辅助函数，不要在每个命令里重复实现。

## 测试用例

- `tests/server/command/CommandRegistryTest.cpp`：覆盖命令注册、解析、执行和部分具体命令行为。

## Mermaid 图表

```mermaid
flowchart LR
    Input["命令字符串"] --> Registry["CommandRegistry"]
    Registry --> Tree["命令树 / 参数节点"]
    Tree --> Command["commands/*.cpp"]
    Command --> Source["ServerCommandSource"]
    Command --> Server["IServer / MinecraftServer"]
    Command --> World["世界 / 玩家 / 库存"]
    Command --> Output["消息 / 状态变更"]

    style Input fill:#ffd166,stroke:#b7791f,color:#111
    style Registry fill:#8ecae6,stroke:#1d4ed8,color:#111
    style Tree fill:#90be6d,stroke:#2f6f3e,color:#111
    style Command fill:#f4a261,stroke:#b45309,color:#111
    style Source fill:#cdb4db,stroke:#6d28d9,color:#111
    style Server fill:#e9c46a,stroke:#a16207,color:#111
    style World fill:#a7f3d0,stroke:#047857,color:#111
    style Output fill:#f1f5f9,stroke:#475569,color:#111
```
