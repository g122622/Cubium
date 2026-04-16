# 命令系统与 Minecraft 1.21.11 对比

## 参考来源

- Java 版命令注册入口：`D:\Minecraft\MC研究\Minecraft1.21.11源码\net\minecraft\commands\Commands.java`
- 当前项目命令注册入口：`Z:\mc_dev\branch2\minecraft-reborn\src\server\command\CommandRegistry.cpp`

本文档只记录当前 C++ 项目服务端命令系统与 Java 版 1.21.11 的能力对比，重点关注：

- 命令覆盖率
- 命令语义完整度
- 当前缺口依赖的底层子系统
- 后续建议的实现优先级

## 当前架构进展

本轮已经补齐并稳定下来的基础设施：

- 命令节点元数据：`description`、`usage`、`permissionLevel`、`implemented`、`aliases`
- 动态 `/help`：直接读取 dispatcher 根节点与 metadata，而非维护静态帮助表
- 玩家选择解析支持层：`src/server/command/support/PlayerResolver.*`
- `EntitySelector` 基础增强：支持用户名、`name=`、`@s/@p/@a/@r`
- `IServer` 命令所需接口扩展：难度、默认游戏模式、挂机超时、广播、停服���求
- 独立命令测试目标：`mc_command_tests`

这些改动的意义不是单纯“补几个命令”，而是先把命令系统的可扩展骨架立稳，后续继续补命令时不需要一遍遍复制样板代码。

## 已注册命令

当前项目默认注册 19 个命令：

- `clear`
- `defaultgamemode`
- `difficulty`
- `experience`
- `gamemode`
- `give`
- `help`
- `kick`
- `kill`
- `list`
- `say`
- `seed`
- `setidletimeout`
- `stop`
- `teleport`
- `time`
- `tp`
- `weather`
- `xp`

其中 `teleport` 与 `xp` 是别名节点，不计作独立语义实现。

## 与 1.21.11 的主要差距

Java 版 `Commands.java` 当前注册了大量命令，远超本项目现状。高频缺失命令包括但不限于：

- `execute`
- `effect`
- `enchant`
- `fill`
- `fillbiome`
- `function`
- `gamerule`
- `item`
- `kick` 之外的管理命令：`op`、`deop`、`ban`、`pardon`、`whitelist`、`save-all`、`save-on`、`save-off`
- `locate`
- `loot`
- `particle`
- `place`
- `playsound`
- `random`
- `ride`
- `rotate`
- `schedule`
- `scoreboard`
- `setblock`
- `setspawn`
- `setworldspawn`
- `spectate`
- `spreadplayers`
- `stopsound`
- `summon`
- `tag`
- `team`
- `teammsg`
- `tellraw`
- `title`
- `trigger`
- `worldborder`

结论很明确：当前项目距离“命令覆盖接近 Java 正式版”还有非常大差距，现阶段更多是打下可持续演进的基础。

## 当前状态分层

### 已有稳定主路径实现

以下命令已经接入当前服务端主路径，且具备明确测试覆盖：

- `difficulty`
- `defaultgamemode`
- `gamemode`
- `help`
- `kick`
- `list`
- `say`
- `seed`
- `setidletimeout`
- `stop`
- `tp`
- `weather`
- `time`

其中本轮重点补实：

- `gamemode`
  - 已支持 `/gamemode <mode>`
  - 已支持 `/gamemode <mode> <target>`
  - 实际写入通过 `GameModeManager` 落到 `ServerPlayerData`
- `tp`
  - 已支持 `/tp <x> <y> <z>`
  - 已支持 `/tp <destinationPlayer>`
  - 已支持 `/tp <targets> <x> <y> <z>`
  - 已支持 `/tp <targets> <destinationPlayer>`
  - 实际传送通过 `TeleportManager`
- `kick`
  - 已支持 `/kick <targets>`
  - 已支持 `/kick <targets> <reason...>`
  - 实际断线通过 `ConnectionManager::disconnectPlayer()`

### 部分实现 / 语义占位

这些命令目前已经注册并可解析，但语义仍然明显不完整：

- `clear`
  - 当前只具备参数解析与反馈框架，尚未真正操作库存容器
- `experience`
  - 当前实现仍耦合 `ServerPlayer*` 假设，不适合作为真实服务端主路径语义依据
- `give`
  - 当前只做到物品参数解析与反馈，尚未真正写入玩家背包
- `kill`
  - 当前仍停留在占位反馈层，未真正作用到统一实体/玩家生命系统

这里要特别强调：

- `experience` 不能简单“缝一下能跑就算完”。
- 当前项目服务端在线玩家主数据是 `ServerPlayerData`。
- 但经验系统真实语义仍主要挂在 `ServerPlayer` / `Player` 实体能力层上。
- 在没有设计清晰的“命令运行时玩家访问面”之前，继续硬糊会制造更大的架构债。

### 已实现但仍与 Java 版有细节差距

即便是已经打通主路径的命令，距离 Java 版体验一致仍有差距：

- `gamemode`
  - 尚未补齐更细的反馈语义、命令统计与客户端同步细节
- `tp`
  - 尚未实现相对坐标、朝向、维度、实体锚点等 Java 版高级语义
- `kick`
  - 还没有对单人世界 owner、局域网公开模式等 Java 版特例做分支
- `time`
  - 还没有 `day/noon/night/midnight` 这类字面值子命令
- `weather`
  - 反馈内容与 Java 版格式并未完全对齐

## 为什么先补这些命令

当前优先级不是盲目追着 `execute` 这种最复杂命令跑，而是遵循两个标准：

- 先补已有底层支撑的命令，保证每条命令落地后都是真实现，不是演示代码
- 先把命令系统骨架、测试与元数据体系立稳，后续新增命令成本才会下降

按这个标准，`gamemode`、`tp`、`kick` 都是高性价比目标：

- `gamemode` 已有 `GameModeManager`
- `tp` 已有 `TeleportManager`
- `kick` 已有 `ConnectionManager`

而 `experience`、`effect`、`summon`、`setblock` 之类虽然重要，但要么依赖玩家运行时层，要么依赖更完整的世界/实体子系统，不适合用脏桥接硬做。

## 后续建议优先级

### 第一优先级

这些命令要么底层支撑已存在，要么只需少量补面：

- `effect`
  - `ServerPlayerData` 已具备效果集合与增删查基础能力
  - 可先实现玩家目标版本的 `give/clear`
- `time` 细化
  - 补 `day/noon/night/midnight` 字面值子命令
- `weather` 细化
  - 收拢默认时长与反馈格式
- `kick` 细化
  - 增加 Java 版风格默认消息与更准确的失败反馈

### 第二优先级

这些命令价值很高，但依赖更完整的底层：

- `setblock`
- `fill`
- `summon`
- `gamerule`
- `function`

### 暂不建议优先实现

以下命令复杂度高、耦合面大，不建议在当前阶段抢先做：

- `execute`
- `scoreboard`
- `team`
- `title`
- `tellraw`
- `schedule`
- `worldborder`

## 当前测试基线

当前建议将命令系统改动的主验证链固定为：

```powershell
cmake --build build --config RelWithDebInfo --target mc_command_tests -- /m:1
./build/bin/RelWithDebInfo/mc_command_tests.exe --gtest_brief=1
```

当前该目标已通过，覆盖了至少以下关键路径：

- 动态帮助
- 难度切换
- 默认游戏模式切换
- `say`
- `stop`
- `setidletimeout`
- `kick`
- `gamemode`
- `tp`
- 命令树 metadata 快照

## 小结

和 Java 版 1.21.11 相比，当前项目命令覆盖率依然很低，但命令系统已经不再是“零散命令 + 静态帮助 + 一堆占位逻辑”的状态了。

目前更合理的推进方式是：

- 持续补那些已有底层支撑的命令
- 先把 partial 命令做成真实主路径实现
- 对依赖未就绪子系统的命令保持诚实，不做假完整实现
- 让每次新增命令都同时具备 metadata、doc 注释、断言与独立测试
