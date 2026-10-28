# 出生数据与实体生成（world/spawn）

世界出生与个人重生的数据模型，以及实体放置规则、群系生成配置和生成成本。

## 目录结构

```
world/spawn/
├── EntitySpawnPlacementRegistry.hpp/cpp # 实体放置类型、高度图、位置检查与生成谓词
├── IWorldSpawnAdapter.hpp # 将运行中的世界提供给生成检查
├── MobSpawnInfo.hpp/cpp # 各生物分类的生成条目、概率与成本
├── MobSpawnInfoJson.hpp/cpp # 群系生成配置的严格 JSON 解码
├── SlimeChunkChecker.hpp/cpp # 由世界种子与区块坐标确定史莱姆区块
├── RespawnData.hpp/cpp   # 出生数据：维度 + 方块位置 + yaw/pitch（含 NBT 编解码、LevelRespawnData 存档容器）
├── RespawnConfig.hpp/cpp # 玩家个人重生点配置：RespawnData + forced（含 NBT 编解码）
└── README.md
```

## 内部模块关系

`RespawnConfig` 由 `RespawnData` 与 `forced` 组成。`LevelRespawnData` 是存档级单例容器
（整个存档只有一份世界出生数据）。落点搜索（`DismountHelper`、`BedBlock::findStandUpPosition`、
`RespawnAnchorBlock::findStandUpPosition`）与本模块无直接依赖，由服务端重生流程串联。

`EntitySpawnPlacementRegistry` 通过 `ISpawnWorldReader` 同时服务运行期世界和生成期区域。
位置检查读取方块的支撑形状、碰撞、流体、信号源及标签；危险方块判断复用 `DismountHelper`，
再应用实体类型的免疫。`MobSpawnInfoJson` 为群系加载器提供统一解码。

## 上下游外部依赖关系

上游：`GlobalPos`、`BlockPos`、`NbtHelper`、`MapDimensionId`、`MathUtils`。
实体生成还依赖方块与实体注册表、标签、随机数、光照和高度图。

下游：`Player`（个人重生点）、`ServerWorld`（世界出生数据）、`LevelDatCodec`、
`PlayerSaveData`、`ServerPlayer::findRespawnPositionAndUseSpawnBlock`、
`SetWorldSpawnCommand` / `SpawnPointCommand`、`RespawnAnchorBlock`。
实体生成的下游包含 `NaturalSpawner`、`WorldGenSpawner`、刷怪笼和流浪商人生成器。

## 容易踩的坑

1. **位置是"方块坐标"语义**：床/重生锚保存的是方块本身；世界出生点保存的也是出生方块，
   玩家脚部在 `pos.y + 1`（`ServerWorld::spawnPosition()` 已换算）。
2. **NBT 键固定为 `dimension`/`pos`/`yaw`/`pitch`/`forced`**：世界出生点是 level.dat 根下的
   `spawn` 复合标签，个人重生点是玩家存档根下的 `respawn` 复合标签；`forced=false` 时不写该键。
3. **朝向在构造时归一化**：`RespawnData::of` 会把 yaw 折到 `[-180, 180]`、pitch 夹到 `[-90, 90]`，
   直接改字段则不会。
4. **`isSamePosition` 只比较维度与方块位置**：朝向或 forced 变化不会触发"重生点已设置"提示。
5. **`pos` 以 IntArray 写出、同时兼容 Int List 读入**：历史数据/外部存档两种形式都要能读。
6. **边界与高度分开**：放置检查使用可配置的水平世界边界；自然生成的建筑高度校验由调用方完成。
7. **位置与谓词分开**：`canSpawnAtLocation` 不消耗随机数；`canSpawnEntity` 先检查位置，再运行注册谓词。
8. **水中放置不检查脚下水深**：当前位置含水、头顶不导红石即可；海带、海草和含水方块必须读取流体状态。
9. **支撑面不等于碰撞面**：泥巴、灵魂沙的碰撞面较低，支撑形状仍完整；脚部与头部只拒绝完整碰撞、信号源、流体、禁入标签及该实体的危险方块。
