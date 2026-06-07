# structures 子模块说明

本目录存放具体结构实现类，负责把抽象结构规则转化为可写入世界的方块布局。

## 1. 目录结构树

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

## 2. 内部模块关系

结构类型按生成方式分为三类：

1. **模板化结构**：使用 `TemplateManager` 加载 NBT 模板
   - `IglooStructure`、`ShipwreckStructure`、`OceanRuinStructure`、`NetherFossilStructure`、`RuinedPortalStructure`
   
2. **递归生成结构**：程序化递归生成片段
   - `EndCityStructure`、`StrongholdStructure`（配合 `StrongholdPieces`）
   
3. **Jigsaw 结构**：使用 Jigsaw 拼图系统
   - `VillageStructure`、`BastionRemnantStructure`、`PillagerOutpostStructure`
   
4. **程序化结构**：完全程序化生成，无模板依赖
   - `BuriedTreasureStructure`、`DesertPyramidStructure`、`FortressStructure`、`JungleTempleStructure`、`MineshaftStructure`、`OceanMonumentStructure`、`SwampHutStructure`

所有结构类继承自 `Structure` 基类，通过 `StructureStart` 管理生成起点。

## 3. 上下游外部依赖关系

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

## 4. 容易踩的坑

- **枚举注册遗漏**：仅在 `StructureType` 增加枚举但未在 `StructureRegistry::initialize()` 注册，运行时不会生成该结构
- **方块未注册**：结构使用的新方块若未在 `VanillaBlocks` 注册，会出现空指针或降级方块
- **阶段顺序错误**：结构过早直接写入世界可能被后续地形阶段覆盖，需结合当前生成链路验证阶段顺序
- **模板管理器未设置**：模板化结构必须在使用前调用 `setTemplateManager()`，否则模板加载会失败
- **随机源错误**：`IntegrityProcessor` 完整度处理器使用位置种子随机，确保使用 `math::getPositionRandom()` 而非简单的哈希值
- **Jigsaw 系统配置**：Jigsaw 结构需要正确配置拼图池（`JigsawPool`）和起始模板，否则无法生成或生成异常
- **递归生成终止条件**：递归生成结构（末地城、要塞）需要正确实现终止条件，否则可能导致无限递归或生成失败
