# 出生数据（world/spawn）

世界出生点与玩家个人重生点共用的数据模型。

## 目录结构

```
world/spawn/
├── RespawnData.hpp/cpp   # 出生数据：维度 + 方块位置 + yaw/pitch（含 NBT 编解码、LevelRespawnData 存档容器）
├── RespawnConfig.hpp/cpp # 玩家个人重生点配置：RespawnData + forced（含 NBT 编解码）
└── README.md
```

## 内部模块关系

`RespawnConfig` 由 `RespawnData` 与 `forced` 组成。`LevelRespawnData` 是存档级单例容器
（整个存档只有一份世界出生数据）。落点搜索（`DismountHelper`、`BedBlock::findStandUpPosition`、
`RespawnAnchorBlock::findStandUpPosition`）与本模块无直接依赖，由服务端重生流程串联。

## 上下游外部依赖关系

上游：`GlobalPos`、`BlockPos`、`NbtHelper`、`MapDimensionId`、`MathUtils`。

下游：`Player`（个人重生点）、`ServerWorld`（世界出生数据）、`LevelDatCodec`、
`PlayerSaveData`、`ServerPlayer::findRespawnPositionAndUseSpawnBlock`、
`SetWorldSpawnCommand` / `SpawnPointCommand`、`RespawnAnchorBlock`。

## 容易踩的坑

1. **位置是"方块坐标"语义**：床/重生锚保存的是方块本身；世界出生点保存的也是出生方块，
   玩家脚部在 `pos.y + 1`（`ServerWorld::spawnPosition()` 已换算）。
2. **NBT 键固定为 `dimension`/`pos`/`yaw`/`pitch`/`forced`**：世界出生点是 level.dat 根下的
   `spawn` 复合标签，个人重生点是玩家存档根下的 `respawn` 复合标签；`forced=false` 时不写该键。
3. **朝向在构造时归一化**：`RespawnData::of` 会把 yaw 折到 `[-180, 180]`、pitch 夹到 `[-90, 90]`，
   直接改字段则不会。
4. **`isSamePosition` 只比较维度与方块位置**：朝向或 forced 变化不会触发"重生点已设置"提示。
5. **`pos` 以 IntArray 写出、同时兼容 Int List 读入**：历史数据/外部存档两种形式都要能读。
