# 服务端记分板（ServerScoreboard）

`common/scoreboard/` 的服务端子类：把记分板变更广播给客户端，并负责持久化。

## 目录结构

```
src/server/scoreboard/
├── ServerScoreboard.hpp/cpp        # Scoreboard 子类：出站同步（SetObjective/SetScore/SetPlayerTeam 等）与存档接入
├── storage/                        # 持久化子系统（详见 storage/README.md）
│   ├── ScoreboardSaveData.hpp/cpp  # 存档数据模型与 NBT/二进制编解码
│   └── ScoreboardDataManager.hpp/cpp # 缓存 + 脏标记的存档门面
└── README.md
```

## 内部模块关系

`Scoreboard`（common）的回调（`onObjectiveAdded`/`onScoreChanged`/…）在这里被重写为「广播 IR 包 +
`markDirty()`」。`ServerScoreboard::save()`/`load()` 经 `ScoreboardDataManager` 落到世界存储。

数字格式（`text::numbers::NumberFormat`）的接线点：
- `_createObjectivePacket` 填 `SetObjective::numberFormat`（目标级格式）
- `_createSetScorePacket` 填 `SetScore::numberFormat`（单条分数的覆盖格式）
两者都经 `numberFormatToWireBytes` 编码；空 vector 表示 absent。

分数条目的自定义显示名（`Score::getDisplay`，`/scoreboard players display name`）与数字格式走同一条
`SetScore::displayName` 通道，存档里是 `Score.Packed` 的兄弟字段 `display`。

## 上下游外部依赖关系

**上游依赖**：`common/scoreboard/core`（模型）、`common/util/text/numbers`（数字格式编解码）、
`common/util/text/ComponentNbtSerialization`（Component NBT 线格式）、`server/network/outbound/PacketSender`、
`server/player/lifecycle/PlayerManager`、`server/scoreboard/storage`。

**被谁依赖**：`server/application/MinecraftServer`（持有实例）、`server/command/commands/ScoreboardCommand`、
`server/command/commands/TriggerCommand`、`server/command/support/PlayerResolver`（选择器分数条件）。

## 容易踩的坑

- **IR 里 `numberFormat` 的空 vector 表示 absent**，`blank` 也要占 1 字节（`VarInt(0)`）。把 blank 写成空
  vector 会被客户端当成「没有格式」，从而回退到默认格式。
- **发送时机**：分数值不变但格式变化（`Score::setNumberFormatOverride`）同样要重发 `SetScore`，否则客户端
  仍按旧格式渲染；不要只在 `setScorePoints` 里发。自定义显示名（`Score::setDisplay`）同理。
- **存档必须按整条 `ScoreData` 落盘**：`ScoreboardDataManager::saveScore` 曾把分数拆成
  `objectiveName/playerName/score/locked` 四个标量传，导致数字格式覆盖与自定义显示名被静默丢弃。
  新增分数条目字段时保持整结构传递，不要拆参。
- **无玩家时广播是空操作**：`sendToAllPlayers` 走 `PlayerManager::forEachPlayer`，单测里没有玩家也能安全调用。
- 新增出站字段时优先复用 `ComponentNbtSerialization` / `numbers::NumberFormatCodec`，不要在 `ServerScoreboard`
  里手写 NBT 结构。
- **队伍枚举字段解析失败要让整次读取失败**：`applyToScoreboard` 返回 `Result<void>`，
  `nameTagVisibility` / `deathMessageVisibility` / `collisionRule` 遇到未知字符串时必须返回
  `ErrorCode::InvalidData`（与原版 `CODEC.byName` 返回 null 一致）。静默降级成默认值会让损坏的存档
  以「队伍行为诡异」的形式很晚才暴露。缺失字段本身是合法的——`TeamData::fromNbt` 会把它填成 `"always"`。