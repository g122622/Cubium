# structures 子模块说明

本目录存放具体结构实现类，负责把抽象结构规则转化为可写入世界的方块布局。

## 目录结构树

```text
structures/
├── BastionRemnantStructure.hpp/.cpp  # 堡垒遗迹 (Jigsaw)
├── BuriedTreasureStructure.hpp/.cpp  # 埋藏宝藏
├── DesertPyramidStructure.hpp/.cpp   # 沙漠神殿
├── EndCityStructure.hpp/.cpp         # 末地城 (递归生成)
├── FortressStructure.hpp/.cpp        # 下界要塞
├── IglooStructure.hpp/.cpp           # 雪屋
├── JungleTempleStructure.hpp/.cpp    # 丛林神庙
├── MineshaftStructure.hpp/.cpp       # 废弃矿井
├── NetherFossilStructure.hpp/.cpp    # 下界化石
├── OceanMonumentStructure.hpp/.cpp   # 海洋纪念碑
├── OceanRuinStructure.hpp/.cpp       # 海底废墟
├── PillagerOutpostStructure.hpp/.cpp # 掠夺者前哨站 (Jigsaw)
├── RuinedPortalStructure.hpp/.cpp    # 废弃传送门
├── ShipwreckStructure.hpp/.cpp       # 沉船
├── StrongholdStructure.hpp/.cpp      # 要塞 (递归生成)
├── StrongholdPieces.hpp/.cpp         # 要塞片段
├── SwampHutStructure.hpp/.cpp        # 沼泽小屋
├── VillageStructure.hpp/.cpp         # 村庄 (Jigsaw)
├── WoodlandMansionStructure.hpp/.cpp # 林地府邸 (模板+程序化)
```

## 内部模块关系

结构类型按生成方式分为三类：

1. **模板化结构**：使用 `TemplateManager` 加载 NBT 模板
   - `IglooStructure`、`ShipwreckStructure`、`OceanRuinStructure`、`NetherFossilStructure`、`RuinedPortalStructure`

2. **递归生成结构**：程序化递归生成片段
   - `EndCityStructure`、`StrongholdStructure`（配合 `StrongholdPieces`）

3. **Jigsaw 结构**：使用 Jigsaw 拼图系统
   - `VillageStructure`、`BastionRemnantStructure`、`PillagerOutpostStructure`

4. **程序化结构**：完全程序化生成，无模板依赖
   - `BuriedTreasureStructure`、`DesertPyramidStructure`、`FortressStructure`、`JungleTempleStructure`、`MineshaftStructure`、`OceanMonumentStructure`、`SwampHutStructure`

5. **混合结构**：模板化片段 + 程序化布局网格
   - `WoodlandMansionStructure`（使用 `MansionGrid` 递归走廊算法生成布局，`MansionPlacer` 放置模板片段）

所有结构类继承自 `Structure` 基类，通过 `ResourceLocation` 标识，使用 `biomeTag()` 进行生物群系匹配，通过 `StructureStart` 管理生成起点。

## 上下游外部依赖关系

**上游依赖（本目录依赖的外部模块）**：
- `world/gen/structure/Structure.hpp` - 结构基类
- `world/gen/structure/StructureStart.hpp` - 结构起点管理
- `world/gen/chunk/IChunkGenerator.hpp` - 高度/海平面/生物群系查询
- `world/block/VanillaBlocks.hpp` - 方块状态定义
- `world/IWorldWriter.hpp` - 世界写入接口
- `world/gen/feature/template/TemplateManager.hpp` - 模板加载和缓存
- `world/gen/feature/template/Template.hpp` - 模板定义
- `world/gen/feature/template/IntegrityProcessor.hpp` - 完整度处理器

**下游依赖（依赖本目录的外部模块）**：
- `StructureRegistry` - 注册和查询所有结构类型
- `StructureManager` - 统一调度结构生成
- 各生物群系的 `StructureProvider` - 决定哪些结构在哪些生物群系生成

## 容易踩的坑

- **方块未注册**：结构使用的新方块若未在 `VanillaBlocks` 注册，会出现空指针或降级方块
- **阶段顺序错误**：结构过早直接写入世界可能被后续地形阶段覆盖，需结合当前生成链路验证阶段顺序
- **模板管理器未设置**：模板化结构必须在使用前调用 `setTemplateManager()`，否则模板加载会失败
- **随机源错误**：`IntegrityProcessor` 完整度处理器使用位置种子随机，确保使用 `math::getPositionRandom()` 而非简单的哈希值
- **Jigsaw 系统配置**：Jigsaw 结构需要正确配置拼图池（`JigsawPool`）和起始模板，否则无法生成或生成异常
- **递归生成终止条件**：递归生成结构（末地城、要塞）需要正确实现终止条件，否则可能导致无限递归或生成失败
- **沙漠神殿考古集成**：`DesertPyramidStructure` 在宝藏室地板四角（相对坐标 (2,2)/(4,2)/(2,4)/(4,4)）放置可疑沙方块，通过 `dynamic_cast<IWorld*>(&world)` 获取 `BrushableBlockEntity` 并调用 `setLootTable("minecraft:archaeology/desert_pyramid", seed)` 挂载考古战利品表。玩家使用刷子刷扫 10 次后掉落考古物品并将方块转换为普通沙。注意 `IWorldWriter` 接口不提供方块实体访问能力，必须 downcast 到 `IWorld`
- **林地府邸房间位标志**：`_identifyRooms()` 在房间网格中设置位标志：0x10000(1x1)/0x20000(1x2)/0x40000(2x2)为房间类型，0x100000为门位置，0x200000为走廊入口标志（门位置与走廊value=1相邻时设置），0x400000为楼梯标志，0x800000为楼梯入口。0x200000标志是三楼走廊生成的关键前提——`_setupThirdFloor()` 仅选择有0x200000标志的1x2房间作为楼梯房间
- **废弃传送门处理器链**：`RuinedPortalPiece::generate()` 按顺序组装 6 类结构处理器，对应 MC 1.21.11 `RuinedPortalPiece#makeSettings`：
  1. `BlockIgnoreStructureProcessor`：airPocket 时只忽略 STRUCTURE_BLOCK，否则忽略 STRUCTURE_AND_AIR
  2. `RuleStructureProcessor`：金块→空气(0.3)、岩浆规则(按位置/寒冷分支)、非寒冷时下界岩→岩浆块(0.07)
  3. `BlockAgeProcessor`：按 `mossiness` 苔藓化石砖/楼梯/台阶/墙/黑曜石
  4. `ProtectedBlocksProcessor`：保护 `#minecraft:features_cannot_replace` 标签方块不被覆盖（依赖 `PlacementSettings::setWorld()`）
  5. `LavaSubmergingProcessor`：岩浆海中非固体方块替换为岩浆（依赖 `PlacementSettings::setWorld()`）
  6. `BlackstoneReplacementProcessor`：仅 `replaceWithBlackstone=true`（下界变体）时添加，将石质方块替换为黑石变体
