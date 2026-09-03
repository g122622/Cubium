# 集成测试结构（`.mcstructure`）图鉴

Cubium 集成测试（GameTest）通过 `.structureName("gametests:xxx")` 指定一个 `.mcstructure` 结构文件作为测试场景。每个结构定义了一组方块的三维排布，测试在此场景内 spawn 实体、放置方块、触发行为并断言结果。

本文档逐一解析 `tests/integrated` 下全部 18 个唯一结构文件，给出：

- **尺寸**（X × Y × Z）与 palette（方块调色板）。
- **每层二维图**（俯视：列=X，行=Z；Y=0 为最底层）。
- **特征**：结构的核心布局与用途。
- **适用情况**：引用该结构的测试文件与使用场景。

> 图例说明：`·`=minecraft:air（空），`G`=glass，`g`=grass_block，`c`=cobblestone，`S`=stone，`n`=sand，`r`=gravel，`P`=planks，`F`=fence，`R`=rail，`>`=command_block，`A`=acacia_button，`L`=log，`l`=leaves，`v`=lava，`X`=obsidian，`B`=blue_ice，`b`=basalt，`C`=cactus，`#`=snow，`p`=grass_path，`,`=grass，`M`=moss_carpet，`V`=vine，`W`=wooden_door，`D`=diamond_block，`K`=coal_block，`m`=concrete，`z`=terracotta，`u`=furnace，`h`=chest，`t`=torch，`H`=ladder，`j`=glow_lichen，`@`=spawner，`T`=structure_block，`?`=barrier。图中未列出的方块用其 `minecraft:` 后首字母大写表示。

---

## 结构命名空间与查找

所有结构以 `structureName("gametests:xxx")` 引用，前缀 `gametests` 对应包内 `structures/gametests/` 子目录。`BehaviorPackStructureSource` 按各启用行为包查找 `structures/<namespace>/<path>.mcstructure`，返回首个命中。

部分结构有命名空间变体：
- `gametests:` —— 主命名空间，绝大多数测试用此。
- `startertests:` —— `starter` 包独有命名空间，仅 `StarterTests` 内部用（如 `mediumglass`）。

---

## 共享结构库

5 个跨包共享结构统一存放于 `tests/integrated/structures/gametests/`，由 `build.mjs` 在构建期复制到每个需要它的包的 `structures/gametests/` 下（与 `utils/` 复用同策略）。这 5 个结构是：

| 结构 | 尺寸 | 主要用途 |
|---|---|---|
| `glass_pit` | 7×5×7 | 通用方块交互测试坑（最常用，397 处引用） |
| `grass_pen` | 9×5×9 | 草地围栏，生物 AI 与植被测试（113 处） |
| `fall_tower` | 7×16×7 | 16 层坠落塔，掉落/坠落伤害测试（29 处） |
| `mediumglass` | 12×9×11 | 中型玻璃房，生物跳跃/碰撞测试（28 处） |
| `light_box` | 7×7×7 | 石盒黑箱，光照/融化/氧化测试（65 处） |

> 共享结构在各包的副本由 `.gitignore` 精确忽略（仅忽略这 5 个共享结构名），各包独有结构仍是追踪的源文件。


### glass_pit
**尺寸**：7 × 5 × 7（X × Y × Z）
**palette**（3 项）：`minecraft:glass`、`minecraft:cobblestone`、`minecraft:air`
**特征**：7×5×7 的玻璃坑结构。最底层 Y=0 是实心玻璃底板，Y=1～Y=3 各层四周为玻璃墙（G）围合、内部为空气腔，不同层的墙体缺口位置略有不同。Y=4 顶层为玻璃顶封闭。整体形成一个封闭的 7×7 玻璃室，内部空间用于放置方块、spawn 实体或搭建红石电路。
**适用情况**：被 142 处引用，用于 `block_behavior/src/tests/agricultural/BoneMealTests.ts`、`block_behavior/src/tests/agricultural/CocoaTests.ts`、`block_behavior/src/tests/agricultural/CropBoneMealTests.ts`、`block_behavior/src/tests/agricultural/FarmlandTests.ts`、`block_behavior/src/tests/agricultural/HoeTillTests.ts`、`block_behavior/src/tests/agricultural/PumpkinTests.ts`、`block_behavior/src/tests/building/AxeStripTests.ts`、`block_behavior/src/tests/building/ConcretePowderTests.ts`、`block_behavior/src/tests/building/DoorTests.ts`、`block_behavior/src/tests/building/FenceConnectionTests.ts`、`block_behavior/src/tests/building/FenceGateTests.ts`、`block_behavior/src/tests/building/StairsTests.ts`、`block_behavior/src/tests/building/TrapdoorTests.ts`、`block_behavior/src/tests/building/WallConnectionTests.ts`、`block_behavior/src/tests/cave/BigDripleafStemTests.ts`、`block_behavior/src/tests/cave/BigDripleafTests.ts`、`block_behavior/src/tests/cave/CaveVinesTests.ts`、`block_behavior/src/tests/cave/MossTests.ts`、`block_behavior/src/tests/cave/SmallDripleafTests.ts`、`block_behavior/src/tests/cave/SporeBlossomTests.ts`、`block_behavior/src/tests/copper/CopperGolemStatueTests.ts`、`block_behavior/src/tests/copper/CopperWaxTests.ts`、`block_behavior/src/tests/coral/CoralTests.ts`、`block_behavior/src/tests/decorative/BannerTests.ts`、`block_behavior/src/tests/decorative/CampfireTests.ts`、`block_behavior/src/tests/decorative/CandleTests.ts`、`block_behavior/src/tests/decorative/CarpetTests.ts`、`block_behavior/src/tests/decorative/ChainTests.ts`、`block_behavior/src/tests/decorative/FlowerPotTests.ts`、`block_behavior/src/tests/decorative/FrostWalkerTests.ts`、`block_behavior/src/tests/decorative/GlazedTerracottaTests.ts`、`block_behavior/src/tests/decorative/LadderTests.ts`、`block_behavior/src/tests/decorative/LanternTests.ts`、`block_behavior/src/tests/decorative/PaneConnectionTests.ts`、`block_behavior/src/tests/decorative/ScaffoldingTests.ts`、`block_behavior/src/tests/decorative/ShovelTests.ts`、`block_behavior/src/tests/decorative/TorchTests.ts`、`block_behavior/src/tests/dirt/PodzolTests.ts`、`block_behavior/src/tests/end/DragonEggTests.ts`、`block_behavior/src/tests/end/EndRodTests.ts`、`block_behavior/src/tests/falling/FallingBlockTests.ts`、`block_behavior/src/tests/functional/BarrelTests.ts`、`block_behavior/src/tests/functional/BedTests.ts`、`block_behavior/src/tests/functional/BellTests.ts`、`block_behavior/src/tests/functional/BrewingStandTests.ts`、`block_behavior/src/tests/functional/CakeTests.ts`、`block_behavior/src/tests/functional/CandleCakeTests.ts`、`block_behavior/src/tests/functional/CartographyTableTests.ts`、`block_behavior/src/tests/functional/CauldronTests.ts`、`block_behavior/src/tests/functional/ComposterTests.ts`、`block_behavior/src/tests/functional/DecoratedPotTests.ts`、`block_behavior/src/tests/functional/FletchingTableTests.ts`、`block_behavior/src/tests/functional/FurnaceTests.ts`、`block_behavior/src/tests/functional/GrindstoneTests.ts`、`block_behavior/src/tests/functional/JukeboxTests.ts`、`block_behavior/src/tests/functional/LecternTests.ts`、`block_behavior/src/tests/functional/LoomTests.ts`、`block_behavior/src/tests/functional/RespawnAnchorTests.ts`、`block_behavior/src/tests/functional/SmithingTableTests.ts`、`block_behavior/src/tests/functional/StonecutterTests.ts`、`block_behavior/src/tests/garden/CactusFlowerTests.ts`、`block_behavior/src/tests/garden/DryVegetationTests.ts`、`block_behavior/src/tests/garden/FireflyBushTests.ts`、`block_behavior/src/tests/garden/LeafLitterTests.ts`、`block_behavior/src/tests/garden/WildflowersTests.ts`、`block_behavior/src/tests/ice/SnowTests.ts`、`block_behavior/src/tests/liquid/LiquidTests.ts`、`block_behavior/src/tests/liquid/WaterLavaInteractionTests.ts`、`block_behavior/src/tests/mob/BeehiveTests.ts`、`block_behavior/src/tests/mob/TurtleEggTests.ts`、`block_behavior/src/tests/nether/MagmaTests.ts`、`block_behavior/src/tests/nether/NetherRootsTests.ts`、`block_behavior/src/tests/nether/SoulFireTests.ts`、`block_behavior/src/tests/ocean/BubbleColumnTests.ts`、`block_behavior/src/tests/ocean/SeaPickleTests.ts`、`block_behavior/src/tests/pale_garden/CreakingHeartTests.ts`、`block_behavior/src/tests/pale_garden/EyeblossomTests.ts`、`block_behavior/src/tests/pale_garden/MossyCarpetTests.ts`、`block_behavior/src/tests/pale_garden/PaleHangingMossTests.ts`、`block_behavior/src/tests/pale_garden/ResinClumpTests.ts`、`block_behavior/src/tests/redstone/ActivatorRailTests.ts`、`block_behavior/src/tests/redstone/ButtonTests.ts`、`block_behavior/src/tests/redstone/ComparatorTests.ts`、`block_behavior/src/tests/redstone/CopperBulbTests.ts`、`block_behavior/src/tests/redstone/DaylightDetectorTests.ts`、`block_behavior/src/tests/redstone/DispenserTests.ts`、`block_behavior/src/tests/redstone/DropperTests.ts`、`block_behavior/src/tests/redstone/HopperTests.ts`、`block_behavior/src/tests/redstone/LeverTests.ts`、`block_behavior/src/tests/redstone/NoteBlockTests.ts`、`block_behavior/src/tests/redstone/ObserverTests.ts`、`block_behavior/src/tests/redstone/PistonTests.ts`、`block_behavior/src/tests/redstone/PoweredRailTests.ts`、`block_behavior/src/tests/redstone/PressurePlateTests.ts`、`block_behavior/src/tests/redstone/RailTests.ts`、`block_behavior/src/tests/redstone/RedstoneBlockTests.ts`、`block_behavior/src/tests/redstone/RedstoneLampTests.ts`、`block_behavior/src/tests/redstone/RedstoneOreTests.ts`、`block_behavior/src/tests/redstone/RedstoneTorchTests.ts`、`block_behavior/src/tests/redstone/RedstoneWireTests.ts`、`block_behavior/src/tests/redstone/RepeaterTests.ts`、`block_behavior/src/tests/redstone/StickyPistonTests.ts`、`block_behavior/src/tests/redstone/TargetBlockTests.ts`、`block_behavior/src/tests/redstone/TntTests.ts`、`block_behavior/src/tests/redstone/TripWireHookTests.ts`、`block_behavior/src/tests/redstone/WeightedPressurePlateTests.ts`、`block_behavior/src/tests/sculk/SculkSensorTests.ts`、`block_behavior/src/tests/sculk/SculkShriekerTests.ts`、`block_behavior/src/tests/special/SpongeTests.ts`、`block_behavior/src/tests/vegetation/BambooTests.ts`、`block_behavior/src/tests/vegetation/CactusTests.ts`、`block_behavior/src/tests/vegetation/DoublePlantTests.ts`、`block_behavior/src/tests/vegetation/KelpTests.ts`、`block_behavior/src/tests/vegetation/LeavesDistanceTests.ts`、`block_behavior/src/tests/vegetation/LilyPadTests.ts`、`block_behavior/src/tests/vegetation/SugarCaneTests.ts`、`block_behavior/src/tests/vegetation/TallGrassTests.ts`、`block_behavior/src/tests/vegetation/VineTests.ts`、`block_behavior/src/tests/vegetation/WitherRoseTests.ts`、`mob_behavior/src/tests/combat/FireImmunityTests.ts`、`mob_behavior/src/tests/combat/HurtRetaliationTests.ts`、`mob_behavior/src/tests/monster/arthropod/SilverfishTests.ts`、`mob_behavior/src/tests/monster/basic/SlimeTests.ts`、`mob_behavior/src/tests/monster/end/EndermanTests.ts`、`mob_behavior/src/tests/monster/nether/MagmaCubeTests.ts`、`mob_behavior/src/tests/monster/nether/ZombifiedPiglinTests.ts`、`mob_behavior/src/tests/monster/undead/DrownedTests.ts`、`mob_behavior/src/tests/monster/undead/ZombieTests.ts`、`mob_behavior/src/tests/passive/basic/PigTests.ts`、`mob_behavior/src/tests/passive/golem/IronGolemTests.ts`、`mob_behavior/src/tests/passive/golem/SnowGolemTests.ts`、`mob_behavior/src/tests/passive/special/StriderTests.ts`、`mob_behavior/src/tests/spawn/despawn/DespawnTests.ts`、`mob_behavior/src/tests/spawn/loot/MobDeathLootTests.ts`、`mob_behavior/src/tests/spawn/loot/MobDeathXpTests.ts`、`mob_behavior/src/tests/spawn/loot/MobEquipmentDropTests.ts`、`mob_behavior/src/tests/spawn/loot/PlayerDeathLootTests.ts`、`teleport/src/tests/cross_dimension/DimensionIdTests.ts`、`teleport/src/tests/cross_dimension/EndPortalTests.ts`、`teleport/src/tests/cross_dimension/ExecuteInTeleportTests.ts`、`teleport/src/tests/cross_dimension/NetherPortalTests.ts`、`teleport/src/tests/cross_dimension/ScriptTeleportTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  GGGGGGG
Z=1  cGGGGGc
Z=2  cGGGGGc
Z=3  ···c···
Z=4  ···c···
Z=5  GGGGGGG
Z=6  G·····G

── Y=1 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  G·····G
Z=1  ·······
Z=2  ···c···
Z=3  GGGGGGG
Z=4  G·····G
Z=5  G·····G
Z=6  ·······

── Y=2 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ···c···
Z=1  GGGGGGG
Z=2  G·····G
Z=3  G·····G
Z=4  c·····c
Z=5  ccc·ccc
Z=6  GGGGGGG

── Y=3 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  G·····G
Z=1  G·····G
Z=2  ·······
Z=3  ···c···
Z=4  GGGGGGG
Z=5  G·····G
Z=6  G·····G

── Y=4 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ·······
Z=1  ···c···
Z=2  GGGGGGG
Z=3  cGGGGGc
Z=4  cGGGGGc
Z=5  ···c···
Z=6  ···c···

```

---

### grass_pen
**尺寸**：9 × 5 × 9（X × Y × Z）
**palette**（3 项）：`minecraft:grass_block`、`minecraft:glass`、`minecraft:air`
**特征**：9×5×9 的草地围栏结构。底层 Y=0 为草方块（g）地面，四周由玻璃墙（G）围合成 9×9 围栏。Y=1～Y=3 为内部空气腔，Y=4 顶层封闭。适合需要草地地表与围合空间的生物行为测试。
**适用情况**：被 62 处引用，用于 `block_behavior/src/tests/vegetation/SweetBerryBushTests.ts`、`lighting/src/tests/core/BlockChangeRelightTests.ts`、`lighting/src/tests/core/BrightnessTests.ts`、`lighting/src/tests/core/NightSkyDarkeningTests.ts`、`lighting/src/tests/core/ShapeOcclusionSkyLightTests.ts`、`lighting/src/tests/core/SkyLightColumnDepthTests.ts`、`lighting/src/tests/core/SkyLightHorizontalSpreadTests.ts`、`lighting/src/tests/core/SkyLightTests.ts`、`lighting/src/tests/core/WeatherSkyDarkeningTests.ts`、`mob_behavior/src/tests/combat/ResistanceEffectTests.ts`、`mob_behavior/src/tests/combat/WitherEffectTests.ts`、`mob_behavior/src/tests/monster/arthropod/CaveSpiderTests.ts`、`mob_behavior/src/tests/monster/arthropod/SpiderTests.ts`、`mob_behavior/src/tests/monster/basic/CreeperTests.ts`、`mob_behavior/src/tests/monster/basic/PhantomTests.ts`、`mob_behavior/src/tests/monster/end/EndermanTests.ts`、`mob_behavior/src/tests/monster/end/ShulkerTests.ts`、`mob_behavior/src/tests/monster/illager/EvokerTests.ts`、`mob_behavior/src/tests/monster/illager/PillagerTests.ts`、`mob_behavior/src/tests/monster/illager/RavagerTests.ts`、`mob_behavior/src/tests/monster/illager/VexTests.ts`、`mob_behavior/src/tests/monster/illager/VindicatorTests.ts`、`mob_behavior/src/tests/monster/illager/WitchTests.ts`、`mob_behavior/src/tests/monster/nether/BlazeTests.ts`、`mob_behavior/src/tests/monster/nether/ZoglinTests.ts`、`mob_behavior/src/tests/monster/ocean/ElderGuardianTests.ts`、`mob_behavior/src/tests/monster/ocean/GuardianTests.ts`、`mob_behavior/src/tests/monster/undead/BoggedTests.ts`、`mob_behavior/src/tests/monster/undead/DrownedTests.ts`、`mob_behavior/src/tests/monster/undead/HuskTests.ts`、`mob_behavior/src/tests/monster/undead/SkeletonTests.ts`、`mob_behavior/src/tests/monster/undead/StrayTests.ts`、`mob_behavior/src/tests/monster/undead/WitherSkeletonTests.ts`、`mob_behavior/src/tests/monster/undead/ZombieTests.ts`、`mob_behavior/src/tests/monster/undead/ZombieVillagerTests.ts`、`mob_behavior/src/tests/passive/ambient/BatTests.ts`、`mob_behavior/src/tests/passive/basic/BabyCowTests.ts`、`mob_behavior/src/tests/passive/basic/ChickenTests.ts`、`mob_behavior/src/tests/passive/basic/CowBreedTests.ts`、`mob_behavior/src/tests/passive/basic/MooshroomTests.ts`、`mob_behavior/src/tests/passive/basic/PigTests.ts`、`mob_behavior/src/tests/passive/basic/RabbitTests.ts`、`mob_behavior/src/tests/passive/basic/SheepTests.ts`、`mob_behavior/src/tests/passive/golem/SnowGolemTests.ts`、`mob_behavior/src/tests/passive/horse/DonkeyTests.ts`、`mob_behavior/src/tests/passive/horse/HorseTests.ts`、`mob_behavior/src/tests/passive/horse/LlamaChestTests.ts`、`mob_behavior/src/tests/passive/horse/LlamaTests.ts`、`mob_behavior/src/tests/passive/horse/SkeletonHorseTests.ts`、`mob_behavior/src/tests/passive/horse/ZombieHorseTests.ts`、`mob_behavior/src/tests/passive/nautilus/ZombieNautilusTests.ts`、`mob_behavior/src/tests/passive/special/BeeTests.ts`、`mob_behavior/src/tests/passive/special/FoxTests.ts`、`mob_behavior/src/tests/passive/special/PandaTests.ts`、`mob_behavior/src/tests/passive/special/PolarBearTests.ts`、`mob_behavior/src/tests/passive/special/TurtleTests.ts`、`mob_behavior/src/tests/passive/tamable/ParrotTests.ts`、`mob_behavior/src/tests/passive/tamable/WolfTests.ts`、`mob_behavior/src/tests/passive/villager/VillagerBreedTests.ts`、`mob_behavior/src/tests/passive/villager/VillagerTests.ts`、`mob_behavior/src/tests/passive/villager/WanderingTraderTests.ts`、`mob_behavior/src/tests/passive/water/DolphinTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    012345678
Z=0  ggggggggg
Z=1  GGGGGGGGG
Z=2  GGGGGGGGG
Z=3  GGGGGGGGG
Z=4  ·········
Z=5  ggggggggg
Z=6  G·······G
Z=7  G·······G
Z=8  G·······G

── Y=1 ── (俯视: 列=X, 行=Z)
    012345678
Z=0  ·········
Z=1  ggggggggg
Z=2  G·······G
Z=3  G·······G
Z=4  G·······G
Z=5  ·········
Z=6  ggggggggg
Z=7  G·······G
Z=8  G·······G

── Y=2 ── (俯视: 列=X, 行=Z)
    012345678
Z=0  G·······G
Z=1  ·········
Z=2  ggggggggg
Z=3  G·······G
Z=4  G·······G
Z=5  G·······G
Z=6  ·········
Z=7  ggggggggg
Z=8  G·······G

── Y=3 ── (俯视: 列=X, 行=Z)
    012345678
Z=0  G·······G
Z=1  G·······G
Z=2  ·········
Z=3  ggggggggg
Z=4  G·······G
Z=5  G·······G
Z=6  G·······G
Z=7  ·········
Z=8  ggggggggg

── Y=4 ── (俯视: 列=X, 行=Z)
    012345678
Z=0  G·······G
Z=1  G·······G
Z=2  G·······G
Z=3  ·········
Z=4  ggggggggg
Z=5  GGGGGGGGG
Z=6  GGGGGGGGG
Z=7  GGGGGGGGG
Z=8  ·········

```

---

### fall_tower
**尺寸**：7 × 16 × 7（X × Y × Z）
**palette**（3 项）：`minecraft:air`、`minecraft:glass`、`minecraft:cobblestone`
**特征**：7×16×7 的坠落塔结构，共 16 层（Y=0～Y=15）。底层为实心玻璃底，往上各层在不同高度有玻璃平台，形成多高度阶梯结构。专门用于测试坠落伤害、掉落物轨迹、多高度平台行为。
**适用情况**：被 16 处引用，用于 `block_behavior/src/tests/agricultural/FarmlandTests.ts`、`block_behavior/src/tests/cave/PointedDripstoneTests.ts`、`block_behavior/src/tests/cave/PowderSnowFreezeTests.ts`、`block_behavior/src/tests/cave/PowderSnowWalkableTests.ts`、`block_behavior/src/tests/cave/SmallDripleafTests.ts`、`block_behavior/src/tests/decorative/LadderTests.ts`、`block_behavior/src/tests/functional/AnvilTests.ts`、`block_behavior/src/tests/functional/CauldronTests.ts`、`block_behavior/src/tests/nether/FireTests.ts`、`block_behavior/src/tests/special/HayBlockTests.ts`、`block_behavior/src/tests/special/HoneyBlockTests.ts`、`block_behavior/src/tests/special/WebTests.ts`、`block_behavior/src/tests/vegetation/SweetBerryBushTests.ts`、`mob_behavior/src/tests/combat/FallDamageTests.ts`、`mob_behavior/src/tests/combat/FeatherFallingTests.ts`、`mob_behavior/src/tests/combat/WindBurstEnchantmentTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ccccccc
Z=1  ·······
Z=2  ·······
Z=3  ·······
Z=4  ·······
Z=5  ·······
Z=6  ·······

── Y=1 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ·······
Z=1  ·······
Z=2  ·······
Z=3  ·······
Z=4  ·······
Z=5  ·······
Z=6  ·······

── Y=2 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ·······
Z=1  ·······
Z=2  ccccccc
Z=3  ·······
Z=4  ·······
Z=5  ·······
Z=6  ·······

── Y=3 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ·······
Z=1  ·······
Z=2  ·······
Z=3  ·······
Z=4  ·······
Z=5  ·······
Z=6  ·······

── Y=4 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ·······
Z=1  ·······
Z=2  ·······
Z=3  ·······
Z=4  ccccccc
Z=5  ···G···
Z=6  ···G···

── Y=5 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ···G···
Z=1  ···G···
Z=2  ···G···
Z=3  ···G···
Z=4  ···G···
Z=5  ···G···
Z=6  ···G···

── Y=6 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ···G···
Z=1  ···G···
Z=2  ···G···
Z=3  ···G···
Z=4  ···G···
Z=5  ···G···
Z=6  ccccccc

── Y=7 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ··G·G··
Z=1  ··G·G··
Z=2  ··G·G··
Z=3  ··G·G··
Z=4  ··G·G··
Z=5  ··G·G··
Z=6  ··G·G··

── Y=8 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ··G·G··
Z=1  ··G·G··
Z=2  ··G·G··
Z=3  ··G·G··
Z=4  ··G·G··
Z=5  ··G·G··
Z=6  ··G·G··

── Y=9 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ··GGG··
Z=1  ccccccc
Z=2  ···G···
Z=3  ···G···
Z=4  ···G···
Z=5  ···G···
Z=6  ···G···

── Y=10 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ···G···
Z=1  ···G···
Z=2  ···G···
Z=3  ···G···
Z=4  ···G···
Z=5  ···G···
Z=6  ···G···

── Y=11 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ···G···
Z=1  ···G···
Z=2  ···G···
Z=3  ccccccc
Z=4  ·······
Z=5  ·······
Z=6  ·······

── Y=12 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ·······
Z=1  ·······
Z=2  ·······
Z=3  ·······
Z=4  ·······
Z=5  ·······
Z=6  ·······

── Y=13 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ·······
Z=1  ·······
Z=2  ·······
Z=3  ·······
Z=4  ·······
Z=5  ccccccc
Z=6  ·······

── Y=14 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ·······
Z=1  ·······
Z=2  ·······
Z=3  ·······
Z=4  ·······
Z=5  ·······
Z=6  ·······

── Y=15 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ·······
Z=1  ·······
Z=2  ·······
Z=3  ·······
Z=4  ·······
Z=5  ·······
Z=6  ·······

```

---

### mediumglass
**尺寸**：12 × 9 × 11（X × Y × Z）
**palette**（3 项）：`minecraft:air`、`minecraft:glass`、`minecraft:cobblestone`
**特征**：12×9×11 的中型玻璃房结构，共 9 层。底层为实心玻璃底板，往上各层四周玻璃墙围合、内部空气腔。尺寸介于 glass_pit 和大型结构之间，适合中等空间内的生物跳跃、碰撞测试。
**适用情况**：被 20 处引用，用于 `block_behavior/src/tests/special/SlimeBlockTests.ts`、`mob_behavior/src/tests/combat/BreachEnchantmentTests.ts`、`mob_behavior/src/tests/combat/KnockbackTests.ts`、`mob_behavior/src/tests/monster/breeze/BreezeTests.ts`、`mob_behavior/src/tests/monster/illager/EvokerTests.ts`、`mob_behavior/src/tests/monster/illager/IllusionerTests.ts`、`mob_behavior/src/tests/monster/nether/ZoglinTests.ts`、`mob_behavior/src/tests/monster/undead/SkeletonTests.ts`、`mob_behavior/src/tests/passive/basic/ChickenTests.ts`、`mob_behavior/src/tests/passive/basic/CowTests.ts`、`mob_behavior/src/tests/passive/basic/JumpTests.ts`、`mob_behavior/src/tests/passive/golem/IronGolemTests.ts`、`mob_behavior/src/tests/passive/special/BeeTests.ts`、`mob_behavior/src/tests/passive/special/SnifferTests.ts`、`mob_behavior/src/tests/passive/special/StriderTests.ts`、`mob_behavior/src/tests/passive/special/TurtleTests.ts`、`mob_behavior/src/tests/passive/tamable/CatTests.ts`、`mob_behavior/src/tests/passive/tamable/OcelotTests.ts`、`node_modules/@minecraft/server-gametest/index.d.ts`、`node_modules/@minecraft/server-gametest/tests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    012345678901
Z=0  ············
Z=1  ············
Z=2  ············
Z=3  ············
Z=4  ············
Z=5  ············
Z=6  ············
Z=7  ············
Z=8  ···GGGGGGGGG
Z=9  GGGGGGGGGGGG
Z=10 GGGGGGGGGGGG

── Y=1 ── (俯视: 列=X, 行=Z)
    012345678901
Z=0  GGGGGGGGGGGG
Z=1  GGGGGGGGGGGG
Z=2  GGGGGGGGGGGG
Z=3  GGGGGGGGGGGG
Z=4  GGGGGGGGGGGG
Z=5  GGGGGGGccccc
Z=6  ccccGG······
Z=7  ···GG·······
Z=8  ··GG········
Z=9  ·GG·········
Z=10 GG·········G

── Y=2 ── (俯视: 列=X, 行=Z)
    012345678901
Z=0  G·········GG
Z=1  ·········GGc
Z=2  ccccccccGGcc
Z=3  cccccccGG···
Z=4  ······GG····
Z=5  ·····GG·····
Z=6  ····GG······
Z=7  ···GG·······
Z=8  ··GG········
Z=9  ·GG·········
Z=10 GGcccccccccG

── Y=3 ── (俯视: 列=X, 行=Z)
    012345678901
Z=0  GcccccccccGG
Z=1  ·········GG·
Z=2  ········GG··
Z=3  ·······GG···
Z=4  ······GG····
Z=5  ·····GG·····
Z=6  ····GG······
Z=7  ···GGccccccc
Z=8  ccGGcccccccc
Z=9  cGG·········
Z=10 GG·········G

── Y=4 ── (俯视: 列=X, 行=Z)
    012345678901
Z=0  G·········GG
Z=1  ·········GG·
Z=2  ········GG··
Z=3  ·······GG···
Z=4  ······GGcccc
Z=5  cccccGGccccc
Z=6  ccccGG······
Z=7  ···GG·······
Z=8  ··GG········
Z=9  ·GG·········
Z=10 GG·········G

── Y=5 ── (俯视: 列=X, 行=Z)
    012345678901
Z=0  G·········GG
Z=1  ·········GGc
Z=2  ccccccccGGcc
Z=3  cccccccGG···
Z=4  ······GG····
Z=5  ·····GG·····
Z=6  ····GG······
Z=7  ···GG·······
Z=8  ··GG········
Z=9  ·GG·········
Z=10 GGcccccccccG

── Y=6 ── (俯视: 列=X, 行=Z)
    012345678901
Z=0  GcccccccccGG
Z=1  ·········GG·
Z=2  ········GG··
Z=3  ·······GG···
Z=4  ······GG····
Z=5  ·····GG·····
Z=6  ····GG······
Z=7  ···GGccccccc
Z=8  ccGGcccccccc
Z=9  cGG·········
Z=10 GG·········G

── Y=7 ── (俯视: 列=X, 行=Z)
    012345678901
Z=0  G·········GG
Z=1  ·········GG·
Z=2  ········GG··
Z=3  ·······GG···
Z=4  ······GGcccc
Z=5  cccccGGccccc
Z=6  ccccGG······
Z=7  ···GG·······
Z=8  ··GG········
Z=9  ·GG·········
Z=10 GG·········G

── Y=8 ── (俯视: 列=X, 行=Z)
    012345678901
Z=0  G·········GG
Z=1  ·········GGc
Z=2  ccccccccGGGG
Z=3  GGGGGGGGGGGG
Z=4  GGGGGGGGGGGG
Z=5  GGGGGGGGGGGG
Z=6  GGGGGGGGGGGG
Z=7  GGGGGGGGGGGG
Z=8  GGGGGGGGGGGG
Z=9  GGGGGGGGGGGG
Z=10 GGGGGGGGGGGG
```

---

### light_box
**尺寸**：7 × 7 × 7（X × Y × Z）
**palette**（2 项）：`minecraft:stone`、`minecraft:air`
**特征**：7×7×7 的石盒黑箱结构，共 7 层。全部由石头（S）与空气（·）构成，外围为石头墙壁完全封闭光线，内部为空气腔。专门用于光照测试——封闭环境内放置光源方块，测试光照传播、亮度计算等。
**适用情况**：被 22 处引用，用于 `block_behavior/src/tests/agricultural/CropLightThresholdTests.ts`、`block_behavior/src/tests/cave/AmethystBudGrowthTests.ts`、`block_behavior/src/tests/copper/CopperOxidationTests.ts`、`block_behavior/src/tests/ice/IceMeltTests.ts`、`block_behavior/src/tests/ice/SnowMeltTests.ts`、`block_behavior/src/tests/vegetation/GrassSpreadTests.ts`、`block_behavior/src/tests/vegetation/MushroomSpreadTests.ts`、`lighting/src/tests/core/BlockChangeRelightTests.ts`、`lighting/src/tests/core/BlockLightEmissionTests.ts`、`lighting/src/tests/core/BlockLightPropagationSymmetryTests.ts`、`lighting/src/tests/core/BlockLightPropagationTests.ts`、`lighting/src/tests/core/BrightnessTests.ts`、`lighting/src/tests/core/CopperBulbEmissionTests.ts`、`lighting/src/tests/core/CopperLanternEmissionTests.ts`、`lighting/src/tests/core/DynamicEmissionTests.ts`、`lighting/src/tests/core/ExtraEmissionTests.ts`、`lighting/src/tests/core/FireflyBushEmissionTests.ts`、`lighting/src/tests/core/FroglightEmissionTests.ts`、`lighting/src/tests/core/NetherPortalEmissionTests.ts`、`lighting/src/tests/core/OpacityBlockLightTests.ts`、`lighting/src/tests/core/RedstoneLampEmissionTests.ts`、`lighting/src/tests/core/StatefulEmissionTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  SSSSSSS
Z=1  SSSSSSS
Z=2  SSSSSSS
Z=3  SSSSSSS
Z=4  SSSSSSS
Z=5  SSSSSSS
Z=6  SSSSSSS

── Y=1 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  SSSSSSS
Z=1  S·····S
Z=2  S·····S
Z=3  S·····S
Z=4  S·····S
Z=5  S·····S
Z=6  SSSSSSS

── Y=2 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  SSSSSSS
Z=1  S·····S
Z=2  S·····S
Z=3  S·····S
Z=4  S·····S
Z=5  S·····S
Z=6  SSSSSSS

── Y=3 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  SSSSSSS
Z=1  S·····S
Z=2  S·····S
Z=3  S·····S
Z=4  S·····S
Z=5  S·····S
Z=6  SSSSSSS

── Y=4 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  SSSSSSS
Z=1  S·····S
Z=2  S·····S
Z=3  S·····S
Z=4  S·····S
Z=5  S·····S
Z=6  SSSSSSS

── Y=5 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  SSSSSSS
Z=1  S·····S
Z=2  S·····S
Z=3  S·····S
Z=4  S·····S
Z=5  S·····S
Z=6  SSSSSSS

── Y=6 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  SSSSSSS
Z=1  SSSSSSS
Z=2  SSSSSSS
Z=3  SSSSSSS
Z=4  SSSSSSS
Z=5  SSSSSSS
Z=6  SSSSSSS

```

---

## block_behavior 包独有结构

### beacon_pit
**尺寸**：11 × 7 × 11（X × Y × Z）
**palette**（2 项）：`minecraft:glass`、`minecraft:air`
**特征**：11×7×11 的玻璃坑结构，共 7 层。底层 Y=0 为实心玻璃底板，Y=1～Y=5 各层四周玻璃墙围合、内部空气腔，Y=6 顶层封闭。比 glass_pit（7×7）更大的空间，适合需要更大底面积的信标/光源测试。
**适用情况**：被 1 处引用，用于 `block_behavior/src/tests/functional/BeaconTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    01234567890
Z=0  GGGGGGGGGGG
Z=1  GGGGGGGGGGG
Z=2  GGGGGGGGGGG
Z=3  GGGGGGGGGGG
Z=4  GGGGGGGGGGG
Z=5  GGGGGGGGGGG
Z=6  GGGGGGGGGGG
Z=7  GGGGGGGGGGG
Z=8  GGGGGGGGGGG
Z=9  GGGGGGGGGGG
Z=10 GGGGGGGGGGG

── Y=1 ── (俯视: 列=X, 行=Z)
    01234567890
Z=0  GGGGGGGGGGG
Z=1  G·········G
Z=2  G·········G
Z=3  G·········G
Z=4  G·········G
Z=5  G·········G
Z=6  G·········G
Z=7  G·········G
Z=8  G·········G
Z=9  G·········G
Z=10 GGGGGGGGGGG

── Y=2 ── (俯视: 列=X, 行=Z)
    01234567890
Z=0  GGGGGGGGGGG
Z=1  G·········G
Z=2  G·········G
Z=3  G·········G
Z=4  G·········G
Z=5  G·········G
Z=6  G·········G
Z=7  G·········G
Z=8  G·········G
Z=9  G·········G
Z=10 GGGGGGGGGGG

── Y=3 ── (俯视: 列=X, 行=Z)
    01234567890
Z=0  GGGGGGGGGGG
Z=1  G·········G
Z=2  G·········G
Z=3  G·········G
Z=4  G·········G
Z=5  G·········G
Z=6  G·········G
Z=7  G·········G
Z=8  G·········G
Z=9  G·········G
Z=10 GGGGGGGGGGG

── Y=4 ── (俯视: 列=X, 行=Z)
    01234567890
Z=0  GGGGGGGGGGG
Z=1  G·········G
Z=2  G·········G
Z=3  G·········G
Z=4  G·········G
Z=5  G·········G
Z=6  G·········G
Z=7  G·········G
Z=8  G·········G
Z=9  G·········G
Z=10 GGGGGGGGGGG

── Y=5 ── (俯视: 列=X, 行=Z)
    01234567890
Z=0  GGGGGGGGGGG
Z=1  G·········G
Z=2  G·········G
Z=3  G·········G
Z=4  G·········G
Z=5  G·········G
Z=6  G·········G
Z=7  G·········G
Z=8  G·········G
Z=9  G·········G
Z=10 GGGGGGGGGGG

── Y=6 ── (俯视: 列=X, 行=Z)
    01234567890
Z=0  GGGGGGGGGGG
Z=1  GGGGGGGGGGG
Z=2  GGGGGGGGGGG
Z=3  GGGGGGGGGGG
Z=4  GGGGGGGGGGG
Z=5  GGGGGGGGGGG
Z=6  GGGGGGGGGGG
Z=7  GGGGGGGGGGG
Z=8  GGGGGGGGGGG
Z=9  GGGGGGGGGGG
Z=10 GGGGGGGGGGG

```

---

## challenge 包独有结构

### collapsing_space
**尺寸**：13 × 14 × 11（X × Y × Z）
**palette**（9 项）：`minecraft:planks`、`minecraft:fence`、`minecraft:air`、`minecraft:glass`、`minecraft:sand`、`minecraft:gravel`、`minecraft:tnt`、`minecraft:redstone_wire`、`minecraft:acacia_button`
**特征**：13×14×11 的多方块复杂结构，共 14 层。palette 包含 9 种方块：木板、栅栏、玻璃、沙子、沙砾、TNT、红石粉、金合欢按钮等。结构设计模拟崩塌场景——沙子、沙砾等重力方块悬空于 TNT 与红石机械之上，测试链式反应与多方块复杂交互。
**适用情况**：被 1 处引用，用于 `challenge/src/ChallengeTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    0123456789012
Z=0  PPPPPPPPPPPFF
Z=1  FFFFFFFFF····
Z=2  ·············
Z=3  ·············
Z=4  ·············
Z=5  ·············
Z=6  ·············
Z=7  ·············
Z=8  ·············
Z=9  ·············
Z=10 ·············

── Y=1 ── (俯视: 列=X, 行=Z)
    0123456789012
Z=0  ···········PP
Z=1  PPPPPPPPPF···
Z=2  ······F······
Z=3  ·············
Z=4  ·············
Z=5  ·············
Z=6  ·············
Z=7  ·········GGGG
Z=8  GGGGG··nnnnnn
Z=9  nnn··nnnnnnnn
Z=10 n··nnnnnnnnn·

── Y=2 ── (俯视: 列=X, 行=Z)
    0123456789012
Z=0  ·rrrrrrrrr··r
Z=1  rrrrrrrr·PPPP
Z=2  PPPPPPPF·····
Z=3  ····F········
Z=4  ·············
Z=5  ·············
Z=6  ·············
Z=7  ·············
Z=8  ·······GGGGGG
Z=9  GGG··nrnnrnnr
Z=10 n··nrnnrnnrn·

── Y=3 ── (俯视: 列=X, 行=Z)
    0123456789012
Z=0  ·nrnnrnnrn··r
Z=1  rrrrrrrr··rrr
Z=2  rrrrrr·PPPPPP
Z=3  PPPPPF·······
Z=4  ··F··········
Z=5  ·············
Z=6  ·············
Z=7  ·············
Z=8  G··········T·
Z=9  ·····GGGGGGGG
Z=10 G··nnnnnnnnn·

── Y=4 ── (俯视: 列=X, 行=Z)
    0123456789012
Z=0  ·nnnnnnnnn··n
Z=1  nnnnnnnn··rrr
Z=2  rrrrrr··rrrrr
Z=3  rrrr·PPPPPPPP
Z=4  PPPF·········
Z=5  F············
Z=6  ·············
Z=7  ·············
Z=8  ···········G·
Z=9  ·········R···
Z=10 ···GGGGGGGGG·

── Y=5 ── (俯视: 列=X, 行=Z)
    0123456789012
Z=0  ·nnnnnnnnn··n
Z=1  nnnnnnnn··nnn
Z=2  nnnnnn··rrrrr
Z=3  rrrr··rrrrrrr
Z=4  rr·PPPPPPPPPP
Z=5  PF·········F·
Z=6  ·············
Z=7  ·············
Z=8  ·············
Z=9  ·········G···
Z=10 ·······R·····

── Y=6 ── (俯视: 列=X, 行=Z)
    0123456789012
Z=0  ·GGGGGGGGG··n
Z=1  nnnrnnnn··nnn
Z=2  nrnnnn··nnnnr
Z=3  nnnn··rrrrrrr
Z=4  rr··rrrrrrrrr
Z=5  ·PPPPPPPPPPPF
Z=6  ····P····F···
Z=7  ··P··········
Z=8  P··········P·
Z=9  ·········P···
Z=10 ····GGGGGGG··

── Y=7 ── (俯视: 列=X, 行=Z)
    0123456789012
Z=0  ··TRRARRT···G
Z=1  GGGGGGGG··nnn
Z=2  rrrnnn··nnnrr
Z=3  rnnn··nnnrrrn
Z=4  nn··rrrrrrrrr
Z=5  ··rrrrrrrrr·P
Z=6  PPPPPPPPPPF··
Z=7  ·······F·····
Z=8  ·············
Z=9  ·············
Z=10 ·············

── Y=8 ── (俯视: 列=X, 行=Z)
    0123456789012
Z=0  ·····G·······
Z=1  ···R······GGG
Z=2  GGGGGG··nnnnr
Z=3  nnnn··nnnnrnn
Z=4  nn··nnnnrnnnn
Z=5  ··rrrrrrrrr··
Z=6  rrrrrrrrr·PPP
Z=7  PPPPPPPPF····
Z=8  ·····F·······
Z=9  ·············
Z=10 ·············

── Y=9 ── (俯视: 列=X, 行=Z)
    0123456789012
Z=0  ·············
Z=1  ···G·········
Z=2  ·R······GGGGG
Z=3  GGGG··nnnnnnn
Z=4  nn··nnnnnnnnn
Z=5  ··nnnnnnnnn··
Z=6  rrrrrrrrr··rr
Z=7  rrrrrrr·PPPPP
Z=8  PPPPPPF······
Z=9  ···F·········
Z=10 ·············

── Y=10 ── (俯视: 列=X, 行=Z)
    0123456789012
Z=0  ·············
Z=1  ·············
Z=2  ·G··········T
Z=3  ······GGGGGGG
Z=4  GG··nnnnnnnnn
Z=5  ··nnnnnnnnn··
Z=6  nnnnnnnnn··rr
Z=7  rrrrrrr··rrrr
Z=8  rrrrr·PPPPPPP
Z=9  PPPPF········
Z=10 ·F···········

── Y=11 ── (俯视: 列=X, 行=Z)
    0123456789012
Z=0  ·············
Z=1  ·············
Z=2  ·············
Z=3  ·············
Z=4  ····GGGGGGGGG
Z=5  ··nrnnrnnrn··
Z=6  nrnnrnnrn··nr
Z=7  nnrnnrn··rrrr
Z=8  rrrrr··rrrrrr
Z=9  rrr·PPPPPPPPP
Z=10 PPF·········F

── Y=12 ── (俯视: 列=X, 行=Z)
    0123456789012
Z=0  ·············
Z=1  ·············
Z=2  ·············
Z=3  ·············
Z=4  ·············
Z=5  ··GGGGGGGGG··
Z=6  nnnnnnnnn··nn
Z=7  nnnnnnn··nnnn
Z=8  nnnnn··rrrrrr
Z=9  rrr··rrrrrrrr
Z=10 r·PPPPPPPPPPP

── Y=13 ── (俯视: 列=X, 行=Z)
    0123456789012
Z=0  FFFFFFFFFFF··
Z=1  ·············
Z=2  ·············
Z=3  ·············
Z=4  ·············
Z=5  ·············
Z=6  ·············
Z=7  ·············
Z=8  ·············
Z=9  ·············
Z=10 ·············

```

---

### minibiomes
**尺寸**：10 × 10 × 10（X × Y × Z）
**palette**（37 项）：`minecraft:air`、`minecraft:glass`、`minecraft:stone`、`minecraft:sand`、`minecraft:emerald_ore`、`minecraft:cactus`、`minecraft:rail`、`minecraft:gold_ore`、`minecraft:golden_rail`、`minecraft:campfire`、`minecraft:pointed_dripstone`、`minecraft:medium_amethyst_bud`、`minecraft:lava`、`minecraft:obsidian`、`minecraft:large_amethyst_bud`、`minecraft:dirt`、`minecraft:grass_path`、`minecraft:snow`、`minecraft:basalt`、`minecraft:cobblestone`、`minecraft:planks`、`minecraft:redstone_torch`、`minecraft:glow_frame`、`minecraft:grass`、`minecraft:blue_ice`、`minecraft:wooden_door`、`minecraft:log`、`minecraft:leaves`、`minecraft:stained_glass_pane`、`minecraft:vine`、`minecraft:snow_layer`、`minecraft:glass_pane`、`minecraft:double_plant`、`minecraft:spruce_stairs`、`minecraft:lit_pumpkin`、`minecraft:wood`、`minecraft:moss_carpet`
**特征**：10×10×10 的多群系微型结构，共 10 层。palette 高达 57 项（实际使用 37 种），涵盖石头、沙子、仙人掌、绿宝石矿石、金矿石、熔岩、黑曜石、玄武岩、蓝冰、圆石、木板、红石火把、钟乳石、紫晶芽、萤光物品展示框、藤蔓、树叶、原木、苔藓地毯、雪层、发光地衣、覆雪、草径、泥土、双层植物、云杉楼梯、南瓜灯、木等多种群系特征方块。结构内部分隔为多个微型群系区域，用于测试多群系复杂环境下的综合行为。
**适用情况**：被 1 处引用，用于 `challenge/src/ChallengeTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  ··········
Z=1  ··········
Z=2  ··········
Z=3  ··········
Z=4  ··········
Z=5  ··········
Z=6  ··········
Z=7  ··········
Z=8  ··········
Z=9  ··········

── Y=1 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  GGGGGGGGG·
Z=1  SSSSSnnnn·
Z=2  SESSS···C·
Z=3  SSSSS···C·
Z=4  SSSSS···C·
Z=5  ··········
Z=6  ··········
Z=7  ··········
Z=8  ··········
Z=9  ··········

── Y=2 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  GGGGGGGGG·
Z=1  SSSSSnnnn·
Z=2  SRRRSRRR··
Z=3  G···S·····
Z=4  SS·SS·····
Z=5  ·SSS······
Z=6  ··········
Z=7  ··········
Z=8  ··········
Z=9  ··········

── Y=3 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  GGGGGGGGG·
Z=1  SSSSSnnnn·
Z=2  GRSRSG·RC·
Z=3  GPM·S·····
Z=4  SS·SS·····
Z=5  SSS·······
Z=6  ··········
Z=7  ··········
Z=8  ··········
Z=9  ··········

── Y=4 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  GGGGGGGGG·
Z=1  SSSSSnvXn·
Z=2  SRSRSG·R··
Z=3  S·LPS·····
Z=4  SS·SS·····
Z=5  ··SS······
Z=6  ··S·······
Z=7  ··········
Z=8  ··········
Z=9  ··········

── Y=5 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  GGGGGGGGG·
Z=1  dp####vBc·
Z=2  PGRGRG·Bc·
Z=3  PG·····Rc·
Z=4  P·········
Z=5  P·········
Z=6  P·········
Z=7  P·········
Z=8  ··········
Z=9  ··········

── Y=6 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  GGGGGGGGG·
Z=1  ,p##bbvvc·
Z=2  WR·R·R··c·
Z=3  W·······L·
Z=4  P······LL·
Z=5  S·······L·
Z=6  P·········
Z=7  P·········
Z=8  ··········
Z=9  ··········

── Y=7 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  GGGGGGGGG·
Z=1  dp##bbcdc·
Z=2  PR·RRRddc·
Z=3  P·····,,,·
Z=4  P·L···VR··
Z=5  P·····V···
Z=6  P·····V···
Z=7  P·····L···
Z=8  ·····LLL··
Z=9  ··········

── Y=8 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  GGGGGGGGG·
Z=1  dp####cdc·
Z=2  PR·###ddc·
Z=3  g····Vddd·
Z=4  P·L··VL,,·
Z=5  P·L··VLRD·
Z=6  P·L··VL·D·
Z=7  S····LLL··
Z=8  ·····LLLL·
Z=9  ······LL··

── Y=9 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  GGGGGGGGG·
Z=1  dP####ccc·
Z=2  PPL#LVdcc·
Z=3  P·W··Vddd·
Z=4  PLWLLVddd·
Z=5  PLWL·Vd,c·
Z=6  SLWL··MGR·
Z=7  ··L··LL···
Z=8  ·····LLL··
Z=9  ··········

```

---

## command 包独有结构

### clone_command
**尺寸**：10 × 5 × 10（X × Y × Z）
**palette**（12 项）：`minecraft:glass`、`minecraft:air`、`minecraft:acacia_button`、`minecraft:redstone_wire`、`minecraft:command_block`、`minecraft:purple_glazed_terracotta`、`minecraft:log`、`minecraft:chest`、`minecraft:acacia_door`、`minecraft:andesite_stairs`、`minecraft:pink_glazed_terracotta`、`minecraft:cobblestone`
**特征**：10×5×10 的命令方块测试结构，共 5 层。palette 包含 15 种方块（实际使用 12 种）：玻璃、空气、金合欢按钮、红石粉、命令方块、紫色/粉色陶瓦、原木、箱子、金合欢门、安山岩楼梯、圆石等。结构内布置了命令方块与红石机械，专门用于测试 /clone 命令的区块克隆行为。
**适用情况**：被 1 处引用，用于 `command/src/CommandTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  GGGGGGGGGG
Z=1  ··········
Z=2  ··········
Z=3  ··········
Z=4  ··········
Z=5  GGGGGGGGGG
Z=6  AR>AR>AR>·
Z=7  ··········
Z=8  ··········
Z=9  ··········

── Y=1 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  GGGGGGGGGG
Z=1  APLAh·A·AA
Z=2  ······A···
Z=3  ··········
Z=4  ··········
Z=5  GGGGGGGGGG
Z=6  R·PRh··AAA
Z=7  ··········
Z=8  ··········
Z=9  ··········

── Y=2 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  GGGGGGGGGG
Z=1  >··>···R··
Z=2  ··········
Z=3  ··········
Z=4  ··········
Z=5  GGGGGGGGGG
Z=6  ·······>··
Z=7  ··········
Z=8  ··········
Z=9  ··········

── Y=3 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  GGGGGGGGGG
Z=1  ··········
Z=2  ··········
Z=3  ··········
Z=4  ··········
Z=5  GGGGGGGGGG
Z=6  ··········
Z=7  ··········
Z=8  ··········
Z=9  ··········

── Y=4 ── (俯视: 列=X, 行=Z)
    0123456789
Z=0  GGGGGGGGGG
Z=1  ··········
Z=2  ··········
Z=3  ··········
Z=4  ··········
Z=5  GGGGGGGGGG
Z=6  ·c········
Z=7  ··········
Z=8  ··········
Z=9  ··········

```

---

### cmd_arena
**尺寸**：9 × 7 × 9（X × Y × Z）
**palette**（2 项）：`minecraft:stone`、`minecraft:air`
**特征**：9×7×9 的石头竞技场结构，共 7 层。全部由石头（S）与空气（·）构成。底层 Y=0 为实心石头底板，往上各层在不同位置有石头平台，形成多层迷宫结构。封闭的石头竞技场适合命令测试（/gamemode、/give、/effect、/teleport 等玩家命令）。
**适用情况**：被 28 处引用，用于 `command/src/tests/entity/EntityCommandTests.ts`、`command/src/tests/entity/TeleportCommandTests.ts`、`command/src/tests/player/AttributeTests.ts`、`command/src/tests/player/BossBarTests.ts`、`command/src/tests/player/ClearTests.ts`、`command/src/tests/player/EffectTests.ts`、`command/src/tests/player/EnchantTests.ts`、`command/src/tests/player/ExperienceTests.ts`、`command/src/tests/player/GameModeTests.ts`、`command/src/tests/player/GiveTests.ts`、`command/src/tests/player/ReplaceItemTests.ts`、`command/src/tests/player/ScoreboardTests.ts`、`command/src/tests/player/SpawnPointTests.ts`、`command/src/tests/player/SpreadPlayersTests.ts`、`command/src/tests/player/TagTests.ts`、`command/src/tests/player/TeamTests.ts`、`command/src/tests/player/TriggerTests.ts`、`command/src/tests/world/CloneCommandTests.ts`、`command/src/tests/world/DifficultyTests.ts`、`command/src/tests/world/ExecuteTests.ts`、`command/src/tests/world/FillTests.ts`、`command/src/tests/world/GameRuleTests.ts`、`command/src/tests/world/SetBlockTests.ts`、`command/src/tests/world/SetWorldSpawnTests.ts`、`command/src/tests/world/TimeTests.ts`、`command/src/tests/world/WeatherTests.ts`、`command/src/tests/world/WorldBorderTests.ts`、`command/src/tests/world/WorldCommandTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    012345678
Z=0  SSSSSSSSS
Z=1  SSSSSSSSS
Z=2  SSSSSSSSS
Z=3  SSSSSSSSS
Z=4  SSSSSSSSS
Z=5  SSSSSSSSS
Z=6  SSSSSSSSS
Z=7  SSSSSSSSS
Z=8  S·······S

── Y=1 ── (俯视: 列=X, 行=Z)
    012345678
Z=0  S·······S
Z=1  S·······S
Z=2  S·······S
Z=3  S·······S
Z=4  SSSSSSSSS
Z=5  SSSSSSSSS
Z=6  S·······S
Z=7  S·······S
Z=8  S·······S

── Y=2 ── (俯视: 列=X, 行=Z)
    012345678
Z=0  S·······S
Z=1  S·······S
Z=2  SSSSSSSSS
Z=3  SSSSSSSSS
Z=4  S·······S
Z=5  S·······S
Z=6  S·······S
Z=7  S·······S
Z=8  S·······S

── Y=3 ── (俯视: 列=X, 行=Z)
    012345678
Z=0  SSSSSSSSS
Z=1  SSSSSSSSS
Z=2  S·······S
Z=3  S·······S
Z=4  S·······S
Z=5  S·······S
Z=6  S·······S
Z=7  SSSSSSSSS
Z=8  SSSSSSSSS

── Y=4 ── (俯视: 列=X, 行=Z)
    012345678
Z=0  S·······S
Z=1  S·······S
Z=2  S·······S
Z=3  S·······S
Z=4  S·······S
Z=5  SSSSSSSSS
Z=6  SSSSSSSSS
Z=7  S·······S
Z=8  S·······S

── Y=5 ── (俯视: 列=X, 行=Z)
    012345678
Z=0  S·······S
Z=1  S·······S
Z=2  S·······S
Z=3  SSSSSSSSS
Z=4  SSSSSSSSS
Z=5  S·······S
Z=6  S·······S
Z=7  S·······S
Z=8  S·······S

── Y=6 ── (俯视: 列=X, 行=Z)
    012345678
Z=0  S·······S
Z=1  SSSSSSSSS
Z=2  SSSSSSSSS
Z=3  SSSSSSSSS
Z=4  SSSSSSSSS
Z=5  SSSSSSSSS
Z=6  SSSSSSSSS
Z=7  SSSSSSSSS
Z=8  SSSSSSSSS

```

---

## lighting 包独有结构

### cross_chunk_platform
**尺寸**：33 × 7 × 33（X × Y × Z）
**palette**（2 项）：`minecraft:stone`、`minecraft:air`
**特征**：33×7×33 的跨区块平台结构，共 7 层。全部由石头（S）与空气（·）构成。尺寸 33×33 跨越区块边界（区块为 16×16），专门用于测试跨区块的光照计算。各层在不同位置有石头平台，形成多层迷宫结构。
**适用情况**：被 1 处引用，用于 `lighting/src/tests/core/CrossChunkLightingTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    012345678901234567890123456789012
Z=0  SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=1  ·································
Z=2  ·································
Z=3  ·································
Z=4  ·································
Z=5  ·································
Z=6  ·································
Z=7  SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=8  ·································
Z=9  ·································
Z=10 ·································
Z=11 ·································
Z=12 ·································
Z=13 ·································
Z=14 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=15 ·································
Z=16 ·································
Z=17 ·································
Z=18 ·································
Z=19 ·································
Z=20 ·································
Z=21 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=22 ·································
Z=23 ·································
Z=24 ·································
Z=25 ·································
Z=26 ·································
Z=27 ·································
Z=28 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=29 ·································
Z=30 ·································
Z=31 ·································
Z=32 ·································

── Y=1 ── (俯视: 列=X, 行=Z)
    012345678901234567890123456789012
Z=0  ·································
Z=1  ·································
Z=2  SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=3  ·································
Z=4  ·································
Z=5  ·································
Z=6  ·································
Z=7  ·································
Z=8  ·································
Z=9  SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=10 ·································
Z=11 ·································
Z=12 ·································
Z=13 ·································
Z=14 ·································
Z=15 ·································
Z=16 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=17 ·································
Z=18 ·································
Z=19 ·································
Z=20 ·································
Z=21 ·································
Z=22 ·································
Z=23 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=24 ·································
Z=25 ·································
Z=26 ·································
Z=27 ·································
Z=28 ·································
Z=29 ·································
Z=30 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=31 ·································
Z=32 ·································

── Y=2 ── (俯视: 列=X, 行=Z)
    012345678901234567890123456789012
Z=0  ·································
Z=1  ·································
Z=2  ·································
Z=3  ·································
Z=4  SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=5  ·································
Z=6  ·································
Z=7  ·································
Z=8  ·································
Z=9  ·································
Z=10 ·································
Z=11 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=12 ·································
Z=13 ·································
Z=14 ·································
Z=15 ·································
Z=16 ·································
Z=17 ·································
Z=18 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=19 ·································
Z=20 ·································
Z=21 ·································
Z=22 ·································
Z=23 ·································
Z=24 ·································
Z=25 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=26 ·································
Z=27 ·································
Z=28 ·································
Z=29 ·································
Z=30 ·································
Z=31 ·································
Z=32 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS

── Y=3 ── (俯视: 列=X, 行=Z)
    012345678901234567890123456789012
Z=0  ·································
Z=1  ·································
Z=2  ·································
Z=3  ·································
Z=4  ·································
Z=5  ·································
Z=6  SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=7  ·································
Z=8  ·································
Z=9  ·································
Z=10 ·································
Z=11 ·································
Z=12 ·································
Z=13 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=14 ·································
Z=15 ·································
Z=16 ·································
Z=17 ·································
Z=18 ·································
Z=19 ·································
Z=20 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=21 ·································
Z=22 ·································
Z=23 ·································
Z=24 ·································
Z=25 ·································
Z=26 ·································
Z=27 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=28 ·································
Z=29 ·································
Z=30 ·································
Z=31 ·································
Z=32 ·································

── Y=4 ── (俯视: 列=X, 行=Z)
    012345678901234567890123456789012
Z=0  ·································
Z=1  SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=2  ·································
Z=3  ·································
Z=4  ·································
Z=5  ·································
Z=6  ·································
Z=7  ·································
Z=8  SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=9  ·································
Z=10 ·································
Z=11 ·································
Z=12 ·································
Z=13 ·································
Z=14 ·································
Z=15 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=16 ·································
Z=17 ·································
Z=18 ·································
Z=19 ·································
Z=20 ·································
Z=21 ·································
Z=22 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=23 ·································
Z=24 ·································
Z=25 ·································
Z=26 ·································
Z=27 ·································
Z=28 ·································
Z=29 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=30 ·································
Z=31 ·································
Z=32 ·································

── Y=5 ── (俯视: 列=X, 行=Z)
    012345678901234567890123456789012
Z=0  ·································
Z=1  ·································
Z=2  ·································
Z=3  SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=4  ·································
Z=5  ·································
Z=6  ·································
Z=7  ·································
Z=8  ·································
Z=9  ·································
Z=10 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=11 ·································
Z=12 ·································
Z=13 ·································
Z=14 ·································
Z=15 ·································
Z=16 ·································
Z=17 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=18 ·································
Z=19 ·································
Z=20 ·································
Z=21 ·································
Z=22 ·································
Z=23 ·································
Z=24 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=25 ·································
Z=26 ·································
Z=27 ·································
Z=28 ·································
Z=29 ·································
Z=30 ·································
Z=31 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=32 ·································

── Y=6 ── (俯视: 列=X, 行=Z)
    012345678901234567890123456789012
Z=0  ·································
Z=1  ·································
Z=2  ·································
Z=3  ·································
Z=4  ·································
Z=5  SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=6  ·································
Z=7  ·································
Z=8  ·································
Z=9  ·································
Z=10 ·································
Z=11 ·································
Z=12 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=13 ·································
Z=14 ·································
Z=15 ·································
Z=16 ·································
Z=17 ·································
Z=18 ·································
Z=19 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=20 ·································
Z=21 ·································
Z=22 ·································
Z=23 ·································
Z=24 ·································
Z=25 ·································
Z=26 SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=27 ·································
Z=28 ·································
Z=29 ·································
Z=30 ·································
Z=31 ·································
Z=32 ·································

```

---

## mob_behavior 包独有结构

### creeper_pit
**尺寸**：7 × 5 × 7（X × Y × Z）
**palette**（2 项）：`minecraft:grass_block`、`minecraft:air`
**特征**：7×5×7 的苦力怕坑结构，共 5 层。全部由草方块（g）与空气（·）构成。底层 Y=0 为草方块底板，往上各层为空气腔。与 glass_pit 尺寸相同但材质为草地，适合生物战斗测试。
**适用情况**：被 90 处引用，用于 `mob_behavior/src/tests/boss/DragonFireballTests.ts`、`mob_behavior/src/tests/combat/ArmorDamageReductionTests.ts`、`mob_behavior/src/tests/combat/BaneOfArthropodsSlownessTests.ts`、`mob_behavior/src/tests/combat/BlastProtectionKnockbackTests.ts`、`mob_behavior/src/tests/combat/BowArrowDamageTests.ts`、`mob_behavior/src/tests/combat/CooldownScalingTests.ts`、`mob_behavior/src/tests/combat/CrossbowTests.ts`、`mob_behavior/src/tests/combat/EggHatchTests.ts`、`mob_behavior/src/tests/combat/FireAspectEnchantTests.ts`、`mob_behavior/src/tests/combat/FireProtectionBurningTimeTests.ts`、`mob_behavior/src/tests/combat/InstantHealHarmInversionTests.ts`、`mob_behavior/src/tests/combat/LightningDamageTests.ts`、`mob_behavior/src/tests/combat/MeleeEnchantDamageTests.ts`、`mob_behavior/src/tests/combat/PoisonCannotKillTests.ts`、`mob_behavior/src/tests/combat/PotionDowseFireTests.ts`、`mob_behavior/src/tests/combat/PotionExtinguishTests.ts`、`mob_behavior/src/tests/combat/PotionWaterBottleTests.ts`、`mob_behavior/src/tests/combat/ProjectileDeflectTests.ts`、`mob_behavior/src/tests/combat/ProjectileProtectionTests.ts`、`mob_behavior/src/tests/combat/ProjectileRedirectTests.ts`、`mob_behavior/src/tests/combat/ShieldDisableTests.ts`、`mob_behavior/src/tests/combat/ShieldDurabilityTests.ts`、`mob_behavior/src/tests/combat/SnowballDamageTests.ts`、`mob_behavior/src/tests/combat/SpectralArrowTests.ts`、`mob_behavior/src/tests/combat/SweepAttackTests.ts`、`mob_behavior/src/tests/combat/ThornsEnchantTests.ts`、`mob_behavior/src/tests/combat/TridentEnchantTests.ts`、`mob_behavior/src/tests/combat/VillagerExplosionUafTests.ts`、`mob_behavior/src/tests/combat/WindChargeDamageTests.ts`、`mob_behavior/src/tests/monster/arthropod/CaveSpiderTests.ts`、`mob_behavior/src/tests/monster/arthropod/EndermiteTests.ts`、`mob_behavior/src/tests/monster/arthropod/SilverfishTests.ts`、`mob_behavior/src/tests/monster/arthropod/SpiderTests.ts`、`mob_behavior/src/tests/monster/basic/CreeperTests.ts`、`mob_behavior/src/tests/monster/end/EndermanTests.ts`、`mob_behavior/src/tests/monster/end/ShulkerTests.ts`、`mob_behavior/src/tests/monster/illager/PillagerTests.ts`、`mob_behavior/src/tests/monster/illager/RavagerTests.ts`、`mob_behavior/src/tests/monster/illager/VexTests.ts`、`mob_behavior/src/tests/monster/illager/VindicatorTests.ts`、`mob_behavior/src/tests/monster/illager/WitchTests.ts`、`mob_behavior/src/tests/monster/nether/BlazeTests.ts`、`mob_behavior/src/tests/monster/nether/HoglinTests.ts`、`mob_behavior/src/tests/monster/nether/PiglinBruteTests.ts`、`mob_behavior/src/tests/monster/nether/PiglinTests.ts`、`mob_behavior/src/tests/monster/nether/ZombifiedPiglinTests.ts`、`mob_behavior/src/tests/monster/ocean/ElderGuardianTests.ts`、`mob_behavior/src/tests/monster/ocean/GuardianTests.ts`、`mob_behavior/src/tests/monster/undead/BoggedTests.ts`、`mob_behavior/src/tests/monster/undead/DrownedTests.ts`、`mob_behavior/src/tests/monster/undead/EffectImmunityTests.ts`、`mob_behavior/src/tests/monster/undead/GoldenAppleConsumptionTests.ts`、`mob_behavior/src/tests/monster/undead/GoldenAppleSelfConsumptionTests.ts`、`mob_behavior/src/tests/monster/undead/HuskTests.ts`、`mob_behavior/src/tests/monster/undead/SkeletonTests.ts`、`mob_behavior/src/tests/monster/undead/StrayTests.ts`、`mob_behavior/src/tests/monster/undead/WitherSkeletonTests.ts`、`mob_behavior/src/tests/monster/undead/ZombieBreakDoorTests.ts`、`mob_behavior/src/tests/passive/basic/MooshroomTests.ts`、`mob_behavior/src/tests/passive/basic/RabbitTests.ts`、`mob_behavior/src/tests/passive/fish/CodTests.ts`、`mob_behavior/src/tests/passive/fish/FishBucketPickupTests.ts`、`mob_behavior/src/tests/passive/fish/FishBucketReleaseTests.ts`、`mob_behavior/src/tests/passive/fish/PufferfishTests.ts`、`mob_behavior/src/tests/passive/horse/TraderLlamaTests.ts`、`mob_behavior/src/tests/passive/nautilus/NautilusTests.ts`、`mob_behavior/src/tests/passive/special/BowDurabilityTests.ts`、`mob_behavior/src/tests/passive/special/BucketItemTests.ts`、`mob_behavior/src/tests/passive/special/FishingRodTests.ts`、`mob_behavior/src/tests/passive/special/FlintAndSteelDurabilityTests.ts`、`mob_behavior/src/tests/passive/special/FoodContainerConsumptionTests.ts`、`mob_behavior/src/tests/passive/special/FoxTests.ts`、`mob_behavior/src/tests/passive/special/HoeDurabilityTests.ts`、`mob_behavior/src/tests/passive/special/HoneyBottleConsumptionTests.ts`、`mob_behavior/src/tests/passive/special/MilkBucketConsumptionTests.ts`、`mob_behavior/src/tests/passive/special/NameTagConsumptionTests.ts`、`mob_behavior/src/tests/passive/special/PolarBearTests.ts`、`mob_behavior/src/tests/passive/special/PotionConsumptionTests.ts`、`mob_behavior/src/tests/passive/special/PowderSnowBucketReleaseTests.ts`、`mob_behavior/src/tests/passive/special/SaddleConsumptionTests.ts`、`mob_behavior/src/tests/passive/special/ShearsDurabilityTests.ts`、`mob_behavior/src/tests/passive/special/ThrowableItemTests.ts`、`mob_behavior/src/tests/passive/special/TridentConsumptionTests.ts`、`mob_behavior/src/tests/passive/tamable/CatTests.ts`、`mob_behavior/src/tests/passive/tamable/OcelotTests.ts`、`mob_behavior/src/tests/passive/tamable/WolfTests.ts`、`mob_behavior/src/tests/passive/water/AxolotlTests.ts`、`mob_behavior/src/tests/passive/water/DolphinTests.ts`、`mob_behavior/src/tests/passive/water/GlowSquidTests.ts`、`mob_behavior/src/tests/passive/water/SquidTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ggggggg
Z=1  ·······
Z=2  ·······
Z=3  ·······
Z=4  ·······
Z=5  ggggggg
Z=6  ·······

── Y=1 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ·······
Z=1  ·······
Z=2  ·······
Z=3  ggggggg
Z=4  ·······
Z=5  ·······
Z=6  ·······

── Y=2 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ·······
Z=1  ggggggg
Z=2  ·······
Z=3  ·······
Z=4  ·······
Z=5  ·······
Z=6  ggggggg

── Y=3 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ·······
Z=1  ·······
Z=2  ·······
Z=3  ·······
Z=4  ggggggg
Z=5  ·······
Z=6  ·······

── Y=4 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ·······
Z=1  ·······
Z=2  ggggggg
Z=3  ·······
Z=4  ·······
Z=5  ·······
Z=6  ·······

```

---

### dark_cavern
**尺寸**：41 × 7 × 9（X × Y × Z）
**palette**（2 项）：`minecraft:stone`、`minecraft:air`
**特征**：41×7×9 的石头迷宫洞穴结构，共 7 层。全部由石头（S）与空气（·）构成。狭长的 41×9 地道在不同高度有石头平台交错，模拟黑暗洞穴环境。专门用于测试黑暗环境下的自然生成。
**适用情况**：被 1 处引用，用于 `mob_behavior/src/tests/spawn/natural/NaturalSpawnTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    01234567890123456789012345678901234567890
Z=0  SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
Z=1  SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS·······SS
Z=2  ·······SS·······SS·······SS·······SS·····
Z=3  ··SSSSSSSSSSS·······SS·······SS·······SS·
Z=4  ······SS·······SS·······SSSSSSSSSSS······
Z=5  ·SS·······SS·······SS·······SS·······SS··
Z=6  ·····SSSSSSSSSSS·······SS·······SS·······
Z=7  SS·······SS·······SS·······SSSSSSSSSSS···
Z=8  ····SS·······SS·······SS·······SS·······S

── Y=1 ── (俯视: 列=X, 行=Z)
    01234567890123456789012345678901234567890
Z=0  S·······SSSSSSSSSSS·······SS·······SS····
Z=1  ···SS·······SS·······SS·······SSSSSSSSSSS
Z=2  ·······SS·······SS·······SS·······SS·····
Z=3  ··SS·······SSSSSSSSSSS·······SS·······SS·
Z=4  ······SS·······SS·······SS·······SSSSSSSS
Z=5  SSS·······SS·······SS·······SS·······SS··
Z=6  ·····SS·······SSSSSSSSSSS·······SS·······
Z=7  SS·······SS·······SS·······SS·······SSSSS
Z=8  SSSSSS·······SS·······SS·······SS·······S

── Y=2 ── (俯视: 列=X, 行=Z)
    01234567890123456789012345678901234567890
Z=0  S·······SS·······SSSSSSSSSSS·······SS····
Z=1  ···SS·······SS·······SS·······SS·······SS
Z=2  SSSSSSSSS·······SS·······SS·······SS·····
Z=3  ··SS·······SS·······SSSSSSSSSSS·······SS·
Z=4  ······SS·······SS·······SS·······SS······
Z=5  ·SSSSSSSSSSS·······SS·······SS·······SS··
Z=6  ·····SS·······SS·······SSSSSSSSSSS·······
Z=7  SS·······SS·······SS·······SS·······SS···
Z=8  ····SSSSSSSSSSS·······SS·······SS·······S

── Y=3 ── (俯视: 列=X, 行=Z)
    01234567890123456789012345678901234567890
Z=0  S·······SS·······SS·······SSSSSSSSSSS····
Z=1  ···SS·······SS·······SS·······SS·······SS
Z=2  ·······SSSSSSSSSSS·······SS·······SS·····
Z=3  ··SS·······SS·······SS·······SSSSSSSSSSS·
Z=4  ······SS·······SS·······SS·······SS······
Z=5  ·SS·······SSSSSSSSSSS·······SS·······SS··
Z=6  ·····SS·······SS·······SS·······SSSSSSSSS
Z=7  SS·······SS·······SS·······SS·······SS···
Z=8  ····SS·······SSSSSSSSSSS·······SS·······S

── Y=4 ── (俯视: 列=X, 行=Z)
    01234567890123456789012345678901234567890
Z=0  S·······SS·······SS·······SS·······SSSSSS
Z=1  SSSSS·······SS·······SS·······SS·······SS
Z=2  ·······SS·······SSSSSSSSSSS·······SS·····
Z=3  ··SS·······SS·······SS·······SS·······SSS
Z=4  SSSSSSSS·······SS·······SS·······SS······
Z=5  ·SS·······SS·······SSSSSSSSSSS·······SS··
Z=6  ·····SS·······SS·······SS·······SS·······
Z=7  SSSSSSSSSSS·······SS·······SS·······SS···
Z=8  ····SS·······SS·······SSSSSSSSSSS·······S

── Y=5 ── (俯视: 列=X, 行=Z)
    01234567890123456789012345678901234567890
Z=0  S·······SS·······SS·······SS·······SS····
Z=1  ···SSSSSSSSSSS·······SS·······SS·······SS
Z=2  ·······SS·······SS·······SSSSSSSSSSS·····
Z=3  ··SS·······SS·······SS·······SS·······SS·
Z=4  ······SSSSSSSSSSS·······SS·······SS······
Z=5  ·SS·······SS·······SS·······SSSSSSSSSSS··
Z=6  ·····SS·······SS·······SS·······SS·······
Z=7  SS·······SSSSSSSSSSS·······SS·······SS···
Z=8  ····SS·······SS·······SS·······SSSSSSSSSS

── Y=6 ── (俯视: 列=X, 行=Z)
    01234567890123456789012345678901234567890
Z=0  S·······SS·······SS·······SS·······SS····
Z=1  ···SS·······SSSSSSSSSSS·······SS·······SS
Z=2  ·······SS·······SS·······SS·······SSSSSSS
Z=3  SSSS·······SS·······SS·······SS·······SS·
Z=4  ······SS·······SSSSSSSSSSS·······SS······
Z=5  ·SS·······SS·······SS·······SS·······SSSS
Z=6  SSSSSSS·······SS·······SS·······SS·······
Z=7  SS·······SS·······SSSSSSSSSSSSSSSSSSSSSSS
Z=8  SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS

```

---

### ghast_arena
**尺寸**：15 × 30 × 15（X × Y × Z）
**palette**（3 项）：`minecraft:cobblestone`、`minecraft:glass`、`minecraft:air`
**特征**：15×30×15 的高耸玻璃竞技场结构，共 30 层。palette 为 3 种：圆石（c）、玻璃（G）、空气（·）。底层 Y=0 为圆石底板，往上 29 层四周玻璃墙围合、内部空气腔，形成 30 层高的封闭竞技场。专门用于测试高空飞行生物（恶魂、末影龙、凋灵）。
**适用情况**：被 3 处引用，用于 `mob_behavior/src/tests/boss/WardenTests.ts`、`mob_behavior/src/tests/boss/WitherTests.ts`、`mob_behavior/src/tests/monster/nether/GhastTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  GGGGGGGGGGGGGGG
Z=2  GGGGGGGGGGGGGGG
Z=3  GGGGGGGGGGGGGGG
Z=4  GGGGGGGGGGGGGGG
Z=5  GGGGGGGGGGGGGGG
Z=6  GGGGGGGGGGGGGGG
Z=7  GGGGGGGGGGGGGGG
Z=8  GGGGGGGGGGGGGGG
Z=9  GGGGGGGGGGGGGGG
Z=10 GGGGGGGGGGGGGGG
Z=11 GGGGGGGGGGGGGGG
Z=12 GGGGGGGGGGGGGGG
Z=13 GGGGGGGGGGGGGGG
Z=14 GGGGGGGGGGGGGGG

── Y=1 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  GGGGGGGGGGGGGGG
Z=1  GGGGGGGGGGGGGGG
Z=2  GGGGGGGGGGGGGGG
Z=3  GGGGGGGGGGGGGGG
Z=4  GGGGGGGGGGGGGGG
Z=5  GGGGGGGGGGGGGGG
Z=6  GGGGGGGGGGGGGGG
Z=7  GGGGGGGGGGGGGGG
Z=8  GGGGGGGGGGGGGGG
Z=9  GGGGGGGGGGGGGGG
Z=10 GGGGGGGGGGGGGGG
Z=11 GGGGGGGGGGGGGGG
Z=12 GGGGGGGGGGGGGGG
Z=13 GGGGGGGGGGGGGGG
Z=14 ccccccccccccccc

── Y=2 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 G·············G

── Y=3 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  G·············G
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 ccccccccccccccc

── Y=4 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 G·············G

── Y=5 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  G·············G
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 ccccccccccccccc

── Y=6 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 G·············G

── Y=7 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  G·············G
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 ccccccccccccccc

── Y=8 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 G·············G

── Y=9 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  G·············G
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 ccccccccccccccc

── Y=10 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 G·············G

── Y=11 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  G·············G
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 ccccccccccccccc

── Y=12 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 G·············G

── Y=13 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  G·············G
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 ccccccccccccccc

── Y=14 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 G·············G

── Y=15 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  G·············G
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 ccccccccccccccc

── Y=16 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 G·············G

── Y=17 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  G·············G
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 ccccccccccccccc

── Y=18 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 G·············G

── Y=19 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  G·············G
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 ccccccccccccccc

── Y=20 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 G·············G

── Y=21 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  G·············G
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 ccccccccccccccc

── Y=22 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 G·············G

── Y=23 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  G·············G
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 ccccccccccccccc

── Y=24 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 G·············G

── Y=25 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  G·············G
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 ccccccccccccccc

── Y=26 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 G·············G

── Y=27 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  G·············G
Z=1  G·············G
Z=2  G·············G
Z=3  G·············G
Z=4  G·············G
Z=5  G·············G
Z=6  G·············G
Z=7  G·············G
Z=8  G·············G
Z=9  G·············G
Z=10 G·············G
Z=11 G·············G
Z=12 G·············G
Z=13 G·············G
Z=14 ccccccccccccccc

── Y=28 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  ccccccccccccccc
Z=1  GGGGGGGGGGGGGGG
Z=2  GGGGGGGGGGGGGGG
Z=3  GGGGGGGGGGGGGGG
Z=4  GGGGGGGGGGGGGGG
Z=5  GGGGGGGGGGGGGGG
Z=6  GGGGGGGGGGGGGGG
Z=7  GGGGGGGGGGGGGGG
Z=8  GGGGGGGGGGGGGGG
Z=9  GGGGGGGGGGGGGGG
Z=10 GGGGGGGGGGGGGGG
Z=11 GGGGGGGGGGGGGGG
Z=12 GGGGGGGGGGGGGGG
Z=13 GGGGGGGGGGGGGGG
Z=14 GGGGGGGGGGGGGGG

── Y=29 ── (俯视: 列=X, 行=Z)
    012345678901234
Z=0  GGGGGGGGGGGGGGG
Z=1  GGGGGGGGGGGGGGG
Z=2  GGGGGGGGGGGGGGG
Z=3  GGGGGGGGGGGGGGG
Z=4  GGGGGGGGGGGGGGG
Z=5  GGGGGGGGGGGGGGG
Z=6  GGGGGGGGGGGGGGG
Z=7  GGGGGGGGGGGGGGG
Z=8  GGGGGGGGGGGGGGG
Z=9  GGGGGGGGGGGGGGG
Z=10 GGGGGGGGGGGGGGG
Z=11 GGGGGGGGGGGGGGG
Z=12 GGGGGGGGGGGGGGG
Z=13 GGGGGGGGGGGGGGG
Z=14 ccccccccccccccc

```

---

### glass_cells
**尺寸**：7 × 7 × 7（X × Y × Z）
**palette**（3 项）：`minecraft:glass`、`minecraft:air`、`minecraft:wooden_door`
**特征**：7×7×7 的玻璃隔间结构，共 7 层。palette 为 4 种：玻璃、空气、木门（上下半）。内部分隔为多个玻璃小隔间，部分隔间有木门。结构设计用于隔离多组实体进行独立行为测试。
**适用情况**：被 0 处引用（遗留未用结构）
```
── Y=0 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  GGGGGGG
Z=1  GGGGGGG
Z=2  GGGGGGG
Z=3  GGGGGGG
Z=4  GGGGGGG
Z=5  GGGGGGG
Z=6  GGG·GGG

── Y=1 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  GGGGGGG
Z=1  G··G··G
Z=2  G··G··G
Z=3  G··G··G
Z=4  G··G··G
Z=5  G··G··G
Z=6  GGGGGGG

── Y=2 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  GGGGGGG
Z=1  W··G··G
Z=2  W··G··G
Z=3  G··G··G
Z=4  G··G··G
Z=5  G··G··G
Z=6  GGGGGGG

── Y=3 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  GGGGGGG
Z=1  GGGGGGG
Z=2  GGGGGGG
Z=3  GGGGGGG
Z=4  GGGGGGG
Z=5  GGGGGGG
Z=6  GGGGGGG

── Y=4 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  GGGGGGG
Z=1  G·G·G·G
Z=2  G·G·G·G
Z=3  G·G·G·G
Z=4  G·G·G·G
Z=5  G·G·G·G
Z=6  GGGGGGG

── Y=5 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  GGGGGGG
Z=1  GGGGGGG
Z=2  GGGGGGG
Z=3  GGGGGGG
Z=4  GGGGGGG
Z=5  GGGGGGG
Z=6  GGGGGGG

── Y=6 ── (俯视: 列=X, 行=Z)
    0123456
Z=0  ·······
Z=1  ·······
Z=2  ·······
Z=3  ·······
Z=4  ·······
Z=5  ·······
Z=6  ·······

```

---

### open_grass_hall
**尺寸**：41 × 7 × 9（X × Y × Z）
**palette**（3 项）：`minecraft:grass_block`、`minecraft:glass`、`minecraft:air`
**特征**：41×7×9 的开放草地走廊结构，共 7 层。palette 为 3 种：草方块（g）、玻璃（G）、空气（·）。底层 Y=0 为大面积草方块与玻璃交错，形成长长的走廊结构。适合需要长距离移动的生物行为测试（村民远距离寻路、动物迁徙等）。
**适用情况**：被 6 处引用，用于 `mob_behavior/src/tests/monster/nether/PiglinTests.ts`、`mob_behavior/src/tests/monster/ocean/GuardianTests.ts`、`mob_behavior/src/tests/passive/basic/RabbitTests.ts`、`mob_behavior/src/tests/passive/special/FoxTests.ts`、`mob_behavior/src/tests/passive/water/DolphinTests.ts`、`mob_behavior/src/tests/spawn/natural/NaturalSpawnTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    01234567890123456789012345678901234567890
Z=0  gggggggggGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
Z=1  GGGGGGGGGGGGGGGGGGGGGGgggggggggG·······GG
Z=2  ·······GG·······GG·······GG·······GG·····
Z=3  ··GgggggggggG·······GG·······GG·······GG·
Z=4  ······GG·······GG·······GgggggggggG······
Z=5  ·GG·······GG·······GG·······GG·······GG··
Z=6  ·····GgggggggggG·······GG·······GG·······
Z=7  GG·······GG·······GG·······GgggggggggG···
Z=8  ····GG·······GG·······GG·······GG·······G

── Y=1 ── (俯视: 列=X, 行=Z)
    01234567890123456789012345678901234567890
Z=0  G·······GgggggggggG·······GG·······GG····
Z=1  ···GG·······GG·······GG·······GgggggggggG
Z=2  ·······GG·······GG·······GG·······GG·····
Z=3  ··GG·······GgggggggggG·······GG·······GG·
Z=4  ······GG·······GG·······GG·······Gggggggg
Z=5  ggG·······GG·······GG·······GG·······GG··
Z=6  ·····GG·······GgggggggggG·······GG·······
Z=7  GG·······GG·······GG·······GG·······Ggggg
Z=8  gggggG·······GG·······GG·······GG·······G

── Y=2 ── (俯视: 列=X, 行=Z)
    01234567890123456789012345678901234567890
Z=0  G·······GG·······GgggggggggG·······GG····
Z=1  ···GG·······GG·······GG·······GG·······Gg
Z=2  ggggggggG·······GG·······GG·······GG·····
Z=3  ··GG·······GG·······GgggggggggG·······GG·
Z=4  ······GG·······GG·······GG·······GG······
Z=5  ·GgggggggggG·······GG·······GG·······GG··
Z=6  ·····GG·······GG·······GgggggggggG·······
Z=7  GG·······GG·······GG·······GG·······GG···
Z=8  ····GgggggggggG·······GG·······GG·······G

── Y=3 ── (俯视: 列=X, 行=Z)
    01234567890123456789012345678901234567890
Z=0  G·······GG·······GG·······GgggggggggG····
Z=1  ···GG·······GG·······GG·······GG·······GG
Z=2  ·······GgggggggggG·······GG·······GG·····
Z=3  ··GG·······GG·······GG·······GgggggggggG·
Z=4  ······GG·······GG·······GG·······GG······
Z=5  ·GG·······GgggggggggG·······GG·······GG··
Z=6  ·····GG·······GG·······GG·······Ggggggggg
Z=7  gG·······GG·······GG·······GG·······GG···
Z=8  ····GG·······GgggggggggG·······GG·······G

── Y=4 ── (俯视: 列=X, 行=Z)
    01234567890123456789012345678901234567890
Z=0  G·······GG·······GG·······GG·······Gggggg
Z=1  ggggG·······GG·······GG·······GG·······GG
Z=2  ·······GG·······GgggggggggG·······GG·····
Z=3  ··GG·······GG·······GG·······GG·······Ggg
Z=4  gggggggG·······GG·······GG·······GG······
Z=5  ·GG·······GG·······GgggggggggG·······GG··
Z=6  ·····GG·······GG·······GG·······GG·······
Z=7  GgggggggggG·······GG·······GG·······GG···
Z=8  ····GG·······GG·······GgggggggggG·······G

── Y=5 ── (俯视: 列=X, 行=Z)
    01234567890123456789012345678901234567890
Z=0  G·······GG·······GG·······GG·······GG····
Z=1  ···GgggggggggG·······GG·······GG·······GG
Z=2  ·······GG·······GG·······GgggggggggG·····
Z=3  ··GG·······GG·······GG·······GG·······GG·
Z=4  ······GgggggggggG·······GG·······GG······
Z=5  ·GG·······GG·······GG·······GgggggggggG··
Z=6  ·····GG·······GG·······GG·······GG·······
Z=7  GG·······GgggggggggG·······GG·······GG···
Z=8  ····GG·······GG·······GG·······Gggggggggg

── Y=6 ── (俯视: 列=X, 行=Z)
    01234567890123456789012345678901234567890
Z=0  G·······GG·······GG·······GG·······GG····
Z=1  ···GG·······GgggggggggG·······GG·······GG
Z=2  ·······GG·······GG·······GG·······Ggggggg
Z=3  gggG·······GG·······GG·······GG·······GG·
Z=4  ······GG·······GgggggggggG·······GG······
Z=5  ·GG·······GG·······GG·······GG·······Gggg
Z=6  ggggggG·······GG·······GG·······GG·······
Z=7  GG·······GG·······GgggggggggGGGGGGGGGGGGG
Z=8  GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG

```

---

### reinforce_arena
**尺寸**：81 × 7 × 81（X × Y × Z）
**palette**（3 项）：`minecraft:grass_block`、`minecraft:glass`、`minecraft:air`
**特征**：81×7×81 的超大草地竞技场结构，共 7 层。palette 为 3 种：草方块（g）、玻璃（G）、空气（·）。底层 Y=0 为大面积草方块与玻璃交错围栏，形成 81×81 的开阔竞技场。尺寸远超普通结构，专门用于测试僵尸增援机制（僵尸在受击时呼叫附近僵尸增援的 AI 逻辑，需要足够空间容纳多个增援僵尸）。
**适用情况**：被 1 处引用，用于 `mob_behavior/src/tests/spawn/reinforcement/ZombieReinforcementTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    012345678901234567890123456789012345678901234567890123456789012345678901234567890
Z=0  ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=1  GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
Z=2  GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
Z=3  GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
Z=4  GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
Z=5  GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
Z=6  GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
Z=7  ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=8  G···············································································G
Z=9  G···············································································G
Z=10 G···············································································G
Z=11 G···············································································G
Z=12 G···············································································G
Z=13 G···············································································G
Z=14 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=15 G···············································································G
Z=16 G···············································································G
Z=17 G···············································································G
Z=18 G···············································································G
Z=19 G···············································································G
Z=20 G···············································································G
Z=21 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=22 G···············································································G
Z=23 G···············································································G
Z=24 G···············································································G
Z=25 G···············································································G
Z=26 G···············································································G
Z=27 G···············································································G
Z=28 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=29 G···············································································G
Z=30 G···············································································G
Z=31 G···············································································G
Z=32 G···············································································G
Z=33 G···············································································G
Z=34 G···············································································G
Z=35 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=36 G···············································································G
Z=37 G···············································································G
Z=38 G···············································································G
Z=39 G···············································································G
Z=40 G···············································································G
Z=41 G···············································································G
Z=42 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=43 G···············································································G
Z=44 G···············································································G
Z=45 G···············································································G
Z=46 G···············································································G
Z=47 G···············································································G
Z=48 G···············································································G
Z=49 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=50 G···············································································G
Z=51 G···············································································G
Z=52 G···············································································G
Z=53 G···············································································G
Z=54 G···············································································G
Z=55 G···············································································G
Z=56 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=57 G···············································································G
Z=58 G···············································································G
Z=59 G···············································································G
Z=60 G···············································································G
Z=61 G···············································································G
Z=62 G···············································································G
Z=63 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=64 G···············································································G
Z=65 G···············································································G
Z=66 G···············································································G
Z=67 G···············································································G
Z=68 G···············································································G
Z=69 G···············································································G
Z=70 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=71 G···············································································G
Z=72 G···············································································G
Z=73 G···············································································G
Z=74 G···············································································G
Z=75 G···············································································G
Z=76 G···············································································G
Z=77 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=78 G···············································································G
Z=79 G···············································································G
Z=80 G···············································································G

── Y=1 ── (俯视: 列=X, 行=Z)
    012345678901234567890123456789012345678901234567890123456789012345678901234567890
Z=0  G···············································································G
Z=1  G···············································································G
Z=2  G···············································································G
Z=3  ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=4  G···············································································G
Z=5  G···············································································G
Z=6  G···············································································G
Z=7  G···············································································G
Z=8  G···············································································G
Z=9  G···············································································G
Z=10 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=11 G···············································································G
Z=12 G···············································································G
Z=13 G···············································································G
Z=14 G···············································································G
Z=15 G···············································································G
Z=16 G···············································································G
Z=17 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=18 G···············································································G
Z=19 G···············································································G
Z=20 G···············································································G
Z=21 G···············································································G
Z=22 G···············································································G
Z=23 G···············································································G
Z=24 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=25 G···············································································G
Z=26 G···············································································G
Z=27 G···············································································G
Z=28 G···············································································G
Z=29 G···············································································G
Z=30 G···············································································G
Z=31 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=32 G···············································································G
Z=33 G···············································································G
Z=34 G···············································································G
Z=35 G···············································································G
Z=36 G···············································································G
Z=37 G···············································································G
Z=38 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=39 G···············································································G
Z=40 G···············································································G
Z=41 G···············································································G
Z=42 G···············································································G
Z=43 G···············································································G
Z=44 G···············································································G
Z=45 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=46 G···············································································G
Z=47 G···············································································G
Z=48 G···············································································G
Z=49 G···············································································G
Z=50 G···············································································G
Z=51 G···············································································G
Z=52 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=53 G···············································································G
Z=54 G···············································································G
Z=55 G···············································································G
Z=56 G···············································································G
Z=57 G···············································································G
Z=58 G···············································································G
Z=59 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=60 G···············································································G
Z=61 G···············································································G
Z=62 G···············································································G
Z=63 G···············································································G
Z=64 G···············································································G
Z=65 G···············································································G
Z=66 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=67 G···············································································G
Z=68 G···············································································G
Z=69 G···············································································G
Z=70 G···············································································G
Z=71 G···············································································G
Z=72 G···············································································G
Z=73 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=74 G···············································································G
Z=75 G···············································································G
Z=76 G···············································································G
Z=77 G···············································································G
Z=78 G···············································································G
Z=79 G···············································································G
Z=80 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg

── Y=2 ── (俯视: 列=X, 行=Z)
    012345678901234567890123456789012345678901234567890123456789012345678901234567890
Z=0  G···············································································G
Z=1  G···············································································G
Z=2  G···············································································G
Z=3  G···············································································G
Z=4  G···············································································G
Z=5  G···············································································G
Z=6  ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=7  G···············································································G
Z=8  G···············································································G
Z=9  G···············································································G
Z=10 G···············································································G
Z=11 G···············································································G
Z=12 G···············································································G
Z=13 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=14 G···············································································G
Z=15 G···············································································G
Z=16 G···············································································G
Z=17 G···············································································G
Z=18 G···············································································G
Z=19 G···············································································G
Z=20 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=21 G···············································································G
Z=22 G···············································································G
Z=23 G···············································································G
Z=24 G···············································································G
Z=25 G···············································································G
Z=26 G···············································································G
Z=27 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=28 G···············································································G
Z=29 G···············································································G
Z=30 G···············································································G
Z=31 G···············································································G
Z=32 G···············································································G
Z=33 G···············································································G
Z=34 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=35 G···············································································G
Z=36 G···············································································G
Z=37 G···············································································G
Z=38 G···············································································G
Z=39 G···············································································G
Z=40 G···············································································G
Z=41 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=42 G···············································································G
Z=43 G···············································································G
Z=44 G···············································································G
Z=45 G···············································································G
Z=46 G···············································································G
Z=47 G···············································································G
Z=48 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=49 G···············································································G
Z=50 G···············································································G
Z=51 G···············································································G
Z=52 G···············································································G
Z=53 G···············································································G
Z=54 G···············································································G
Z=55 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=56 G···············································································G
Z=57 G···············································································G
Z=58 G···············································································G
Z=59 G···············································································G
Z=60 G···············································································G
Z=61 G···············································································G
Z=62 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=63 G···············································································G
Z=64 G···············································································G
Z=65 G···············································································G
Z=66 G···············································································G
Z=67 G···············································································G
Z=68 G···············································································G
Z=69 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=70 G···············································································G
Z=71 G···············································································G
Z=72 G···············································································G
Z=73 G···············································································G
Z=74 G···············································································G
Z=75 G···············································································G
Z=76 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=77 G···············································································G
Z=78 G···············································································G
Z=79 G···············································································G
Z=80 G···············································································G

── Y=3 ── (俯视: 列=X, 行=Z)
    012345678901234567890123456789012345678901234567890123456789012345678901234567890
Z=0  G···············································································G
Z=1  G···············································································G
Z=2  ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=3  G···············································································G
Z=4  G···············································································G
Z=5  G···············································································G
Z=6  G···············································································G
Z=7  G···············································································G
Z=8  G···············································································G
Z=9  ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=10 G···············································································G
Z=11 G···············································································G
Z=12 G···············································································G
Z=13 G···············································································G
Z=14 G···············································································G
Z=15 G···············································································G
Z=16 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=17 G···············································································G
Z=18 G···············································································G
Z=19 G···············································································G
Z=20 G···············································································G
Z=21 G···············································································G
Z=22 G···············································································G
Z=23 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=24 G···············································································G
Z=25 G···············································································G
Z=26 G···············································································G
Z=27 G···············································································G
Z=28 G···············································································G
Z=29 G···············································································G
Z=30 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=31 G···············································································G
Z=32 G···············································································G
Z=33 G···············································································G
Z=34 G···············································································G
Z=35 G···············································································G
Z=36 G···············································································G
Z=37 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=38 G···············································································G
Z=39 G···············································································G
Z=40 G···············································································G
Z=41 G···············································································G
Z=42 G···············································································G
Z=43 G···············································································G
Z=44 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=45 G···············································································G
Z=46 G···············································································G
Z=47 G···············································································G
Z=48 G···············································································G
Z=49 G···············································································G
Z=50 G···············································································G
Z=51 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=52 G···············································································G
Z=53 G···············································································G
Z=54 G···············································································G
Z=55 G···············································································G
Z=56 G···············································································G
Z=57 G···············································································G
Z=58 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=59 G···············································································G
Z=60 G···············································································G
Z=61 G···············································································G
Z=62 G···············································································G
Z=63 G···············································································G
Z=64 G···············································································G
Z=65 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=66 G···············································································G
Z=67 G···············································································G
Z=68 G···············································································G
Z=69 G···············································································G
Z=70 G···············································································G
Z=71 G···············································································G
Z=72 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=73 G···············································································G
Z=74 G···············································································G
Z=75 G···············································································G
Z=76 G···············································································G
Z=77 G···············································································G
Z=78 G···············································································G
Z=79 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=80 G···············································································G

── Y=4 ── (俯视: 列=X, 行=Z)
    012345678901234567890123456789012345678901234567890123456789012345678901234567890
Z=0  G···············································································G
Z=1  G···············································································G
Z=2  G···············································································G
Z=3  G···············································································G
Z=4  G···············································································G
Z=5  ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=6  G···············································································G
Z=7  G···············································································G
Z=8  G···············································································G
Z=9  G···············································································G
Z=10 G···············································································G
Z=11 G···············································································G
Z=12 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=13 G···············································································G
Z=14 G···············································································G
Z=15 G···············································································G
Z=16 G···············································································G
Z=17 G···············································································G
Z=18 G···············································································G
Z=19 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=20 G···············································································G
Z=21 G···············································································G
Z=22 G···············································································G
Z=23 G···············································································G
Z=24 G···············································································G
Z=25 G···············································································G
Z=26 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=27 G···············································································G
Z=28 G···············································································G
Z=29 G···············································································G
Z=30 G···············································································G
Z=31 G···············································································G
Z=32 G···············································································G
Z=33 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=34 G···············································································G
Z=35 G···············································································G
Z=36 G···············································································G
Z=37 G···············································································G
Z=38 G···············································································G
Z=39 G···············································································G
Z=40 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=41 G···············································································G
Z=42 G···············································································G
Z=43 G···············································································G
Z=44 G···············································································G
Z=45 G···············································································G
Z=46 G···············································································G
Z=47 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=48 G···············································································G
Z=49 G···············································································G
Z=50 G···············································································G
Z=51 G···············································································G
Z=52 G···············································································G
Z=53 G···············································································G
Z=54 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=55 G···············································································G
Z=56 G···············································································G
Z=57 G···············································································G
Z=58 G···············································································G
Z=59 G···············································································G
Z=60 G···············································································G
Z=61 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=62 G···············································································G
Z=63 G···············································································G
Z=64 G···············································································G
Z=65 G···············································································G
Z=66 G···············································································G
Z=67 G···············································································G
Z=68 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=69 G···············································································G
Z=70 G···············································································G
Z=71 G···············································································G
Z=72 G···············································································G
Z=73 G···············································································G
Z=74 G···············································································G
Z=75 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=76 G···············································································G
Z=77 G···············································································G
Z=78 G···············································································G
Z=79 G···············································································G
Z=80 G···············································································G

── Y=5 ── (俯视: 列=X, 行=Z)
    012345678901234567890123456789012345678901234567890123456789012345678901234567890
Z=0  G···············································································G
Z=1  ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=2  G···············································································G
Z=3  G···············································································G
Z=4  G···············································································G
Z=5  G···············································································G
Z=6  G···············································································G
Z=7  G···············································································G
Z=8  ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=9  G···············································································G
Z=10 G···············································································G
Z=11 G···············································································G
Z=12 G···············································································G
Z=13 G···············································································G
Z=14 G···············································································G
Z=15 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=16 G···············································································G
Z=17 G···············································································G
Z=18 G···············································································G
Z=19 G···············································································G
Z=20 G···············································································G
Z=21 G···············································································G
Z=22 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=23 G···············································································G
Z=24 G···············································································G
Z=25 G···············································································G
Z=26 G···············································································G
Z=27 G···············································································G
Z=28 G···············································································G
Z=29 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=30 G···············································································G
Z=31 G···············································································G
Z=32 G···············································································G
Z=33 G···············································································G
Z=34 G···············································································G
Z=35 G···············································································G
Z=36 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=37 G···············································································G
Z=38 G···············································································G
Z=39 G···············································································G
Z=40 G···············································································G
Z=41 G···············································································G
Z=42 G···············································································G
Z=43 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=44 G···············································································G
Z=45 G···············································································G
Z=46 G···············································································G
Z=47 G···············································································G
Z=48 G···············································································G
Z=49 G···············································································G
Z=50 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=51 G···············································································G
Z=52 G···············································································G
Z=53 G···············································································G
Z=54 G···············································································G
Z=55 G···············································································G
Z=56 G···············································································G
Z=57 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=58 G···············································································G
Z=59 G···············································································G
Z=60 G···············································································G
Z=61 G···············································································G
Z=62 G···············································································G
Z=63 G···············································································G
Z=64 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=65 G···············································································G
Z=66 G···············································································G
Z=67 G···············································································G
Z=68 G···············································································G
Z=69 G···············································································G
Z=70 G···············································································G
Z=71 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=72 G···············································································G
Z=73 G···············································································G
Z=74 G···············································································G
Z=75 G···············································································G
Z=76 G···············································································G
Z=77 G···············································································G
Z=78 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=79 G···············································································G
Z=80 G···············································································G

── Y=6 ── (俯视: 列=X, 行=Z)
    012345678901234567890123456789012345678901234567890123456789012345678901234567890
Z=0  G···············································································G
Z=1  G···············································································G
Z=2  G···············································································G
Z=3  G···············································································G
Z=4  ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=5  G···············································································G
Z=6  G···············································································G
Z=7  G···············································································G
Z=8  G···············································································G
Z=9  G···············································································G
Z=10 G···············································································G
Z=11 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=12 G···············································································G
Z=13 G···············································································G
Z=14 G···············································································G
Z=15 G···············································································G
Z=16 G···············································································G
Z=17 G···············································································G
Z=18 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=19 G···············································································G
Z=20 G···············································································G
Z=21 G···············································································G
Z=22 G···············································································G
Z=23 G···············································································G
Z=24 G···············································································G
Z=25 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=26 G···············································································G
Z=27 G···············································································G
Z=28 G···············································································G
Z=29 G···············································································G
Z=30 G···············································································G
Z=31 G···············································································G
Z=32 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=33 G···············································································G
Z=34 G···············································································G
Z=35 G···············································································G
Z=36 G···············································································G
Z=37 G···············································································G
Z=38 G···············································································G
Z=39 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=40 G···············································································G
Z=41 G···············································································G
Z=42 G···············································································G
Z=43 G···············································································G
Z=44 G···············································································G
Z=45 G···············································································G
Z=46 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=47 G···············································································G
Z=48 G···············································································G
Z=49 G···············································································G
Z=50 G···············································································G
Z=51 G···············································································G
Z=52 G···············································································G
Z=53 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=54 G···············································································G
Z=55 G···············································································G
Z=56 G···············································································G
Z=57 G···············································································G
Z=58 G···············································································G
Z=59 G···············································································G
Z=60 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=61 G···············································································G
Z=62 G···············································································G
Z=63 G···············································································G
Z=64 G···············································································G
Z=65 G···············································································G
Z=66 G···············································································G
Z=67 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=68 G···············································································G
Z=69 G···············································································G
Z=70 G···············································································G
Z=71 G···············································································G
Z=72 G···············································································G
Z=73 G···············································································G
Z=74 ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
Z=75 GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
Z=76 GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
Z=77 GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
Z=78 GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
Z=79 GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
Z=80 GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG

```

---

### spawner_chamber
**尺寸**：11 × 7 × 11（X × Y × Z）
**palette**（2 项）：`minecraft:stone`、`minecraft:air`
**特征**：11×7×11 的石头刷怪室结构，共 7 层。全部由石头（S）与空气（·）构成。内部为复杂的石头迷宫结构，多层交错。用于测试刷怪笼的 spawn 机制或自然生成在受限空间内的行为。
**适用情况**：被 1 处引用，用于 `mob_behavior/src/tests/spawn/spawner/MobSpawnerTests.ts`
```
── Y=0 ── (俯视: 列=X, 行=Z)
    01234567890
Z=0  SSSSSSSSSSS
Z=1  SSSSSSSSSSS
Z=2  SSSSSSSSSSS
Z=3  SSSSSSSSSSS
Z=4  SSSSSSSSSSS
Z=5  SSSSSSSSSSS
Z=6  SSSSSSSSSSS
Z=7  SSSSSSSSSSS
Z=8  S·········S
Z=9  S·········S
Z=10 S·········S

── Y=1 ── (俯视: 列=X, 行=Z)
    01234567890
Z=0  S·········S
Z=1  S·········S
Z=2  SSSSSSSSSSS
Z=3  SSSSSSSSSSS
Z=4  S·········S
Z=5  S·········S
Z=6  S·········S
Z=7  S·········S
Z=8  S·········S
Z=9  SSSSSSSSSSS
Z=10 SSSSSSSSSSS

── Y=2 ── (俯视: 列=X, 行=Z)
    01234567890
Z=0  S·········S
Z=1  S·········S
Z=2  S·········S
Z=3  S·········S
Z=4  S·········S
Z=5  SSSSSSSSSSS
Z=6  SSSSSSSSSSS
Z=7  S·········S
Z=8  S·········S
Z=9  S·········S
Z=10 S·········S

── Y=3 ── (俯视: 列=X, 行=Z)
    01234567890
Z=0  S·········S
Z=1  SSSSSSSSSSS
Z=2  SSSSSSSSSSS
Z=3  S·········S
Z=4  S·········S
Z=5  S·········S
Z=6  S·········S
Z=7  S·········S
Z=8  SSSSSSSSSSS
Z=9  SSSSSSSSSSS
Z=10 S·········S

── Y=4 ── (俯视: 列=X, 行=Z)
    01234567890
Z=0  S·········S
Z=1  S·········S
Z=2  S·········S
Z=3  S·········S
Z=4  SSSSSSSSSSS
Z=5  SSSSSSSSSSS
Z=6  S·········S
Z=7  S·········S
Z=8  S·········S
Z=9  S·········S
Z=10 S·········S

── Y=5 ── (俯视: 列=X, 行=Z)
    01234567890
Z=0  SSSSSSSSSSS
Z=1  SSSSSSSSSSS
Z=2  S·········S
Z=3  S·········S
Z=4  S·········S
Z=5  S·········S
Z=6  S·········S
Z=7  SSSSSSSSSSS
Z=8  SSSSSSSSSSS
Z=9  S·········S
Z=10 S·········S

── Y=6 ── (俯视: 列=X, 行=Z)
    01234567890
Z=0  S·········S
Z=1  S·········S
Z=2  S·········S
Z=3  SSSSSSSSSSS
Z=4  SSSSSSSSSSS
Z=5  SSSSSSSSSSS
Z=6  SSSSSSSSSSS
Z=7  SSSSSSSSSSS
Z=8  SSSSSSSSSSS
Z=9  SSSSSSSSSSS
Z=10 SSSSSSSSSSS

```

---


## 结构选择指南

为新增 GameTest 用例选择结构时，参考下表：

| 场景需求 | 推荐结构 | 理由 |
|---|---|---|
| 通用方块交互（放置/破坏/红石） | `glass_pit` | 最通用，7×7 玻璃坑，397 处引用 |
| 生物 AI（追逐/繁殖/寻路） | `grass_pen` / `creeper_pit` | 草地平台，适合生物移动 |
| 坠落伤害/掉落测试 | `fall_tower` | 16 层坠落塔，多高度平台 |
| 生物跳跃/碰撞测试 | `mediumglass` | 中型玻璃房，开阔空间 |
| 光照/融化/氧化测试 | `light_box` | 石盒黑箱，封闭无光 |
| 命令测试 | `cmd_arena` | 石头竞技场，封闭空间 |
| 信标/光源测试 | `beacon_pit` | 11×11 玻璃坑，更大空间 |
| 跨区块光照计算 | `cross_chunk_platform` | 33×33 跨区块平台 |
| 高空飞行生物测试 | `ghast_arena` | 15×30×15 高耸玻璃竞技场 |
| 长距离移动测试 | `open_grass_hall` | 41×9 草地走廊 |
| 僵尸增援机制 | `reinforce_arena` | 81×81 超大竞技场 |
| 黑暗环境自然生成 | `dark_cavern` | 41×9 石头迷宫洞穴 |
| 刷怪笼 spawn 机制 | `spawner_chamber` | 11×11 石头刷怪室 |
| 隔离多组实体 | `glass_cells` | 7×7 玻璃隔间（遗留未用） |
| 综合挑战（崩塌/多群系） | `collapsing_space` / `minibiomes` | 多方块复杂结构 |

---

## 附录：解析工具

本文档的每层二维图由 `tests/integrated/parse-structures.mjs` 生成。该脚本解析 Bedrock `.mcstructure`（little-endian NBT）并输出每层俯视图。

重新生成某结构的二维图：

```bash
cd tests/integrated
node parse-structures.mjs structures/gametests/glass_pit.mcstructure
```

重新生成全部 18 个结构的二维图：

```bash
cd tests/integrated
node parse-structures.mjs \
  block_behavior/structures/gametests/beacon_pit.mcstructure \
  challenge/structures/gametests/collapsing_space.mcstructure \
  challenge/structures/gametests/minibiomes.mcstructure \
  command/structures/gametests/clone_command.mcstructure \
  command/structures/gametests/cmd_arena.mcstructure \
  lighting/structures/gametests/cross_chunk_platform.mcstructure \
  mob_behavior/structures/gametests/creeper_pit.mcstructure \
  mob_behavior/structures/gametests/dark_cavern.mcstructure \
  mob_behavior/structures/gametests/ghast_arena.mcstructure \
  mob_behavior/structures/gametests/glass_cells.mcstructure \
  mob_behavior/structures/gametests/open_grass_hall.mcstructure \
  mob_behavior/structures/gametests/reinforce_arena.mcstructure \
  mob_behavior/structures/gametests/spawner_chamber.mcstructure \
  structures/gametests/fall_tower.mcstructure \
  structures/gametests/glass_pit.mcstructure \
  structures/gametests/grass_pen.mcstructure \
  structures/gametests/light_box.mcstructure \
  structures/gametests/mediumglass.mcstructure
```
