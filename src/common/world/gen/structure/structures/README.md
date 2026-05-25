#structures 子模块说明

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

## 2. 文件介绍

### 基于模板的结构

以下结构使用 NBT 模板系统，支持完整的 MC 1.16.5 结构变体：

- **OceanRuinStructure**: 海底废墟，使用 Template + IntegrityProcessor
  - 支持暖海（砂岩）和冷海（石砖）两种材质
  - 支持大型/小型废墟变体
  - 支持集群生成（多个小废墟围绕大废墟）
  - 模板路径：`underwater_ruin/brick_*.nbt`, `underwater_ruin/warm_*.nbt` 等
  
- **ShipwreckStructure**: 沉船，使用 Template 系统
  - 支持搁浅和水下两种类型
  - 支持 20 种模板变体（正常/翻转/侧翻，完整/半截，正常/破损）
  - 模板路径：`shipwreck/with_mast.nbt`, `shipwreck/rightsideup_full.nbt` 等

- **NetherFossilStructure**: 下界化石，使用 Template 系统
  - 支持 14 种化石模板变体
  - 在灵魂沙峡谷生物群系生成
  - 模板路径：`nether_fossils/fossil_1.nbt` ~ `fossil_14.nbt`

- **IglooStructure**: 雪屋，使用 Template 系统堆叠
  - 支持地上部分（top）和地下室（middle + bottom）
  - 50% 概率生成地下室，地下室有 1-2 层中间层
  - 模板路径：`igloo/top.nbt`, `igloo/middle.nbt`, `igloo/bottom.nbt`

- **RuinedPortalStructure**: 废弃传送门，使用 Template 系统
  - 支持 10 种普通传送门和 3 种巨型传送门模板
  - 根据生物群系自动配置属性（寒冷、苔藓、空气口袋、藤蔓等）
  - 支持下界变体（黑石替换）
  - 模板路径：`ruined_portal/portal_1.nbt` ~ `portal_10.nbt`, `ruined_portal/giant_portal_1.nbt` ~ `giant_portal_3.nbt`

- **EndCityStructure**: 末地城，使用递归模板生成
  - 基础塔楼 + 房屋 + 桥 + 末地船
  - 支持多层塔楼和胖塔变体
  - 模板路径：`end_city/base_floor.nbt`, `end_city/tower_piece.nbt` 等

- **WoodlandMansionStructure**: 林地府邸，使用递归模板生成
  - 程序化房间布局 + 模板放置
  - 支持 1x1、1x2、2x2 房间类型
  - 模板路径：`woodland_mansion/entrance.nbt`, `woodland_mansion/1x1_a1.nbt` 等

### 程序化生成的结构

以下结构使用程序化生成，不依赖 NBT 模板：

- **BuriedTreasureStructure**: 沙滩藏宝点，低概率单点结构
- **DesertPyramidStructure**: 沙漠神殿体块与地下室逻辑
- **FortressStructure**: 下界要塞主体结构生成
- **JungleTempleStructure**: 丛林神庙外壳与陷阱布局
- **MineshaftStructure**: 废弃矿井片段化生成
- **OceanMonumentStructure**: 深海纪念碑大型体块布局
- **StrongholdStructure**: 要塞骨架与关键房间入口
- **VillageStructure**: 村庄起始点与拼图池衔接

## 3. 模板系统使用

### IglooStructure 示例

```cpp
#include "world/gen/feature/template/TemplateManager.hpp"
#include "world/gen/structure/structures/IglooStructure.hpp"

// 创建模板管理器
feature::template_::TemplateManager templateManager;
templateManager.setResourcePack(&resourcePack);

// 创建结构并设置模板管理器
IglooStructure structure;
structure.setTemplateManager(&templateManager);

// 生成时会自动加载 igloo/top, igloo/middle, igloo/bottom 模板
// 并根据概率决定是否有地下室
```

    ## #RuinedPortalStructure 示例

```cpp
#include "world/gen/feature/template/TemplateManager.hpp"
#include "world/gen/structure/structures/RuinedPortalStructure.hpp"

        feature::template_::TemplateManager templateManager;
templateManager.setResourcePack(&resourcePack);

RuinedPortalStructure structure;
structure.setTemplateManager(&templateManager);

// 生成时会根据生物群系自动选择变体和配置属性
// - 沙漠: 部分掩埋，无苔藓
// - 丛林: 高苔藓，藤蔓，过度生长
// - 沼泽: 海底位置，中等苔藓，藤蔓
// - 下界: 黑石替换，无苔藓
```

    ## #OceanRuinStructure 示例

```cpp
#include “world / gen / structure / structures / OceanRuinStructure.hpp”
#include “world / gen / feature / template / TemplateManager.hpp”

        // 创建模板管理器
        feature::template_::TemplateManager templateManager;
templateManager.setResourcePack(&resourcePack);

// 创建结构并设置模板管理器
OceanRuinStructure structure;
structure.setTemplateManager(&templateManager);

// 设置配置
OceanRuinConfig config;
config.biomeType = OceanRuinType::Cold;
config.largeProbability = 0.3f;
config.clusterProbability = 0.9f;
structure.setConfig(config);
```

    ## #ShipwreckStructure 示例

```cpp
#include “world / gen / structure / structures / ShipwreckStructure.hpp”
#include “world / gen / feature / template / TemplateManager.hpp”

        feature::template_::TemplateManager templateManager;
templateManager.setResourcePack(&resourcePack);

ShipwreckStructure structure;
structure.setTemplateManager(&templateManager);
```

    ##4. 模块关系

    - 上游依赖 : StructureManager 负责统一注册与调度 - 横向依赖 : -IChunkGenerator 提供高度 / 生物群系查询 -
                 VanillaBlocks 提供方块状态 - TemplateManager 提供模板加载和缓存 -
                 下游输出 : 通过 IWorldWriter::setBlockState 写入世界，或将结果挂入 StructureStart

                            ##5. 整体职责

                            -
                            以结构类型为边界，封装各结构的间距、概率、生物群系门槛与具体摆放算法 -
                            模板化结构支持从资源包加载 NBT 模板，确保与 MC 1.16.5 兼容 -
                            维持”可注册、可查询、可生成”的最小闭环，避免仅有枚举无实现的孤岛状态

                                ##6. 输入 /
                                输出

                            -
                            输入 : -世界写入器 IWorldWriter - 生成器 IChunkGenerator（高度 / 海平面 / 生物群系） -
                                   随机源 Random 与区块坐标 - 模板管理器 TemplateManager（用于模板化结构） -
                                   输出
    : -StructureStart（结构起点） -
      对世界区块的方块写入副作用

      ##7. 依赖项

      -
      内部依赖 : -world / gen / structure / Structure.hpp - world / gen / chunk / IChunkGenerator.hpp -
      world / block / VanillaBlocks.hpp - world / IWorldWriter.hpp - world / gen / feature / template / Template.hpp -
      world / gen / feature / template / TemplateManager.hpp - world / gen / feature / template / TemplateLoader.hpp -
      标准库依赖 : -memory -
                   vector

                   ##8. 使用方法

```cpp
#include “world / gen / structure / StructureManager.hpp”

                   mc::world::gen::structure::StructureRegistry::initialize();
const auto* shipwreck = mc::world::gen::structure::StructureRegistry::get(“shipwreck”);
const auto* oceanRuin = mc::world::gen::structure::StructureRegistry::get(“ocean_ruin”);
```

            ##9. 容易踩的坑

            - 仅在 StructureType 增加枚举但未在 StructureRegistry::initialize 注册，运行时不会生成 -
            结构使用的新方块若未在 VanillaBlocks 注册，会出现空指针或降级方块 -
            结构过早直接写入世界可能被后续地形阶段覆盖，需结合当前生成链路验证阶段顺序 -
            **模板化结构** : 必须在使用前设置 TemplateManager，否则模板加载会失败 -
            **IntegrityProcessor** : 完整度处理器使用位置种子随机，确保使用 `math::getPositionRandom()` 而非简单的哈希值

                                     ##10. 测试用例

            - tests / common / world / gen / WaterContentRegistryTest.cpp - 校验 shipwreck / ocean_ruin 已注册 -
            校验水域补齐方块（气泡柱、海龟蛋、死亡珊瑚、海晶楼梯台阶等）已注册

            ##11. Mermaid 图表

```mermaid graph LR subgraph Template System
            TM[TemplateManager] TL[TemplateLoader] TP[Template] IP[IntegrityProcessor] end

            SM[StructureManager] SR[StructureRegistry] SW[ShipwreckStructure] OR[OceanRuinStructure] OM
                [OceanMonumentStructure] RP[RuinedPortalStructure]

            SM-- >
        SR SR-- > SW SR-- > OR SR-- > OM SR-- > RP

        SW-- > TM OR-- > TM OR-- > IP TM-- > TL TL-- > TP
```

        ##12. 实现状态

        ## #已完成对齐的结构

    | 结构 | 类型 | 完成度 | 说明 | | -- -- --| -- -- --| -- -- -- --| -- -- --| | **StrongholdStructure** | 递归生成 |
    70 % | ✅ 已落地片段类型、PieceWeight、buildComponent 骨架、门类型、石砖变体、PortalRoom 基础布局；⚠️
    结构入口流程、权重消耗、片段递归驱动和若干细节仍未对齐 MC 1.16.5 |
    | VillageStructure | Jigsaw | 90 % | 正确使用 JigsawManager | | BastionRemnantStructure | Jigsaw |
    90 % | 正确使用 Jigsaw 系统 | | IglooStructure | Template | 90 % | 正确使用 TemplateManager | | ShipwreckStructure
    | Template | 90 % | 使用模板系统 | | EndCityStructure | 递归模板 | 85 % | 使用递归生成器 | | OceanRuinStructure
    | Template | 85 % | 使用 IntegrityProcessor | | MineshaftStructure | 程序化 | 80 % | 基本完整 |

    ## #需要继续完善的结构

    | 结构 | 类型 | 完成度 | 主要问题 | | -- -- --| -- -- --| -- -- -- --| -- -- -- -- --| |
    **OceanMonumentStructure** | 房间图系统 | 75 %
    | ✅ 已切换到 `OceanMonumentPieces` 入口，已落地 `RoomDefinition`、基础房间图、房间匹配 helper 和 Elder Guardian
    生成入口；⚠️ 结构入口生命周期、朝向随机化、room claim 顺序、helper 优先级和实体初始化仍需继续对齐 Java 版
    | | FortressStructure | Jigsaw + 硬编码 | 40 % | 存在硬编码回退逻辑 | | WoodlandMansionStructure | 程序化 |
    70 % | 房间布局简化 | | DesertPyramidStructure | 硬编码 | 60 % | 细节简化 | | SwampHutStructure | 硬编码 |
    50 % | 简化实现 |

    ## #海洋纪念碑当前状态

        MC 1.16.5 的 OceanMonument 使用复杂的房间图系统。当前仓库已完成以下基础对齐：

        1. *
        *RoomDefinition 类** : 已落地 75 个房间的 5x5x3 网格连接关系 2. * *房间类型** : -DoubleXRoom,
    DoubleXYRoom, DoubleYRoom, DoubleYZRoom, DoubleZRoom - SimpleRoom, SimpleTopRoom - EntryRoom,
    MonumentCoreRoom - Penthouse,
    WingRoom 3. * *IMonumentRoomFitHelper 接口** : 已落地基础匹配逻辑 4. *
        *generateRoomGraph() 算法** : 已建立基础网格，但 `source
        / core` claim 顺序、特殊连接房语义与 helper 优先级仍未完全复刻 Java 版 5. *
        *实体生成** : 已支持在 Penthouse 和 WingRoom 路径上生成 Elder
                      Guardian，但当前仅完成基础 `spawnEntity()`，尚未补齐 Java 版结构生成初始化
        /
        持久化细节

        仍需继续完善：

        1. `OceanMonumentStructure::generate()` 的生命周期对齐，避免在生成起点阶段直接 eager placement
        2. Java 版 `generateRoomGraph()` 的 claim 顺序、随机断连和 source
    -
    path 约束 3. 随机水平朝向与更精确的外墙 / 屋顶 /
        翼楼局部样式 4. Elder Guardian 的结构生成初始化、持久化与测试覆盖

        ## #要塞当前状态

        当前仓库中的要塞实现明显强于“空实现”，但也还没有达到 README 先前描述的 95 %
        完成度。

        已完成：

        1. `StrongholdPieces` 片段类型、`PieceWeight`、`buildComponent()` 骨架、门类型和石砖变体选择器已落地。
        2. `PortalRoom` 已放置传送门框架、末地传送门、熔岩池、铁栏杆与刷怪笼方块。
        3. `INFESTED_STONE_BRICKS` 与 `CAVE_AIR` 已接入要塞生成逻辑。

        仍存在的关键偏差：

        1. `StrongholdStructure::canGenerate()` 当前恒为 `true`，没有按 MC 1.16.5 强要塞位置分布做严格判定。
        2. `StrongholdStructure::generate()` 没有像 Java 一样循环重试直到生成出包含 `portalRoom` 的有效起点。
        3. 主流程没有真正消费 `StrongholdStartStairs::
            pendingChildren()`，而是通过自定义 `generateCorridor()` 旁路递归，导致权重状态与扩展顺序偏离原版。
        4. `generatePieceFromSmallDoor()` 未在权重耗尽后移除片段项，`canAddStructurePieces()` 语义也与原版不完全一致。
        5. `PortalRoom` 末地传送门框架眼睛概率当前写反了，实际行为偏向 90 %
        有眼，而 MC 1.16.5 为每个框 10 %
        有眼。 6. 刷怪笼虽然已放置 `SPAWNER` 方块，但对应的蠹虫实体类型配置仍是未完成项。

            * 最后更新 : 2026 -
    05 - 25 *
