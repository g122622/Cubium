# Template 模板系统

结构模板系统，用于加载、存储和放置 NBT 格式的结构模板文件。主要用于 Jigsaw 结构生成。

## 目录结构

```
template/
├── Template.hpp                      # 模板类（PaletteBlock、BlockInfo、PlacementSettings、StructureProcessor、Palette 与全部处理器声明）
├── Template.cpp                      # 模板类与全部处理器的唯一实现
├── TemplateLoader.hpp                # NBT 模板加载器，从资源包加载 .nbt / .mcstructure 文件
├── TemplateLoader.cpp                # 模板加载实现
├── TemplateManager.hpp               # 模板管理器（模板缓存 + 来源注入，来源注入接口私有）
├── TemplateManager.cpp               # 模板管理实现
├── TemplateManagerHostBinding.hpp    # 模板管理器宿主资源绑定令牌（RAII 绑/解绑，防悬垂指针）
├── TemplateManagerHostBinding.cpp    # 绑定令牌实现
├── IStructurePackSource.hpp          # 结构包资源源接口
├── RuleTest.hpp                      # 规则测试类（用于 RuleStructureProcessor）
├── RuleTest.cpp                      # 规则测试实现
├── CappedStructureProcessor.hpp      # 限制次数处理器（limit 支持 IntProvider 随机范围）
├── CappedStructureProcessor.cpp      # 限制次数处理器实现
├── ProtectedBlocksProcessor.hpp      # 受保护方块处理器（数据包标签驱动）
├── ProtectedBlocksProcessor.cpp      # 受保护方块处理器实现
└── README.md
```

其余处理器（BlockAge / BlackstoneReplacement / BlockIgnore / Gravity / Integrity /
JigsawReplacement / LavaSubmerging / Nop / RuleStructure）与盒模型（BlockInfo、
ProcessedBlockInfo、PlacementSettings、StructureProcessor、TemplateEntityInfo）的声明和实现
全部集中在 `Template.hpp` / `Template.cpp` 中，不再保留同名独立文件。

## 内部模块关系

```
TemplateLoader ──加载──> Template ──缓存──> TemplateManager
                               │
                               ├── Palette（多调色板支持）
                               │      ├── PaletteBlock（持久存储，8 B，模板内坐标）
                               │      └── 稀疏 NBT 表（仅带 NBT 的方块，按块下标二分查找）
                               ├── BlockInfo / ProcessedBlockInfo（放置管线，24 B，世界坐标）
                               ├── PlacementSettings（旋转/镜像/边界框/处理器链）
                               └── StructureProcessor（处理器链）
                                      ├── GravityStructureProcessor
                                      ├── BlockIgnoreStructureProcessor
                                      ├── JigsawReplacementStructureProcessor
                                      ├── IntegrityProcessor
                                      ├── RuleStructureProcessor ──依赖──> RuleTest
                                      ├── NopStructureProcessor
                                      ├── LavaSubmergingProcessor
                                      ├── BlockAgeProcessor ──依赖──> BlockTags（STAIRS/SLABS/WALLS）
                                      ├── BlackstoneReplacementProcessor
                                      ├── ProtectedBlocksProcessor ──依赖──> BlockTags（FEATURES_CANNOT_REPLACE 等保护标签）
                                      └── CappedStructureProcessor ──包装──> StructureProcessor（delegate）
                                                            └──使用──> IntProvider（limit 采样）
```

**关键依赖链**：
- `TemplateLoader` 解析 NBT 文件创建 `Template` 对象
- `TemplateManager` 提供模板缓存；模板来源（数据包仓库 / 结构包资源源）经 `TemplateManagerHostBinding` 注入
- `Template::placeInWorld()` 遍历方块并调用 `StructureProcessor` 链处理（唯一的放置入口）
- `RuleStructureProcessor` 使用 `RuleTest` 进行条件匹配
- `BlockAgeProcessor` 使用 `BlockTags::STAIRS/SLABS/WALLS` 进行标签化方块匹配，使用 `withPropertiesOf()` 保留原方块属性

## 上下游外部依赖关系

### 上游依赖（本模块依赖的外部模块）

- `mc::core` - 基础类型（u32, i32, f32 等）
- `mc::util::math::Random` - 确定性随机数生成
- `mc::world::block` - BlockState、Block、BlockRegistry、VanillaBlocks、BlockTags
- `mc::world::gen::structure` - StructureBoundingBox
- `mc::resource` - ResourceLocation、IResourcePack、DataPackList
- `mc::nbt` - NBT 解析（CompoundTag、ListTag）
- `mc::util::Direction` - Rotation、Mirror 枚举
- `mc::util::property::Properties` - BlockStateProperties（HORIZONTAL_FACING、HALF 等属性）

### 下游依赖（依赖本模块的外部模块）

- `mc::world::gen::jigsaw` - JigsawAssembler/JigsawPlacer、JigsawPiece 使用模板放置拼图块
- `mc::world::gen::structure::structures` - 各种结构（村庄、堡垒遗迹、沉船等）使用模板

## 容易踩的坑

### 1. 方块状态 ID 未注册

使用未注册的方块状态 ID 会导致崩溃或放置失败。确保 `BlockRegistry` 已初始化。

基岩结构中的旧名 `minecraft:grass` 指草方块，必须映射为 `minecraft:grass_block`；按 Java 旧名映射为矮草会使轨道失去支撑。

### 2. 处理器忘记复制 NBT 数据

处理器返回 `ProcessedBlockInfo` 时必须检查并复制 NBT：

```cpp
if (blockInfo.nbt) {
    result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
}
```

### 3. 边界框检查缺失

模板方块可能超出区块边界。必须设置边界框：

```cpp
auto chunkBounds = StructureBoundingBox::fromChunk(chunkX, chunkZ);
settings.setBoundingBox(&chunkBounds);
```

### 4. 放置入口、flags 与 knownShape 必须配套

`Template::placeInWorld()` 是唯一放置入口（旧的基础版 `place()` 已删除，它不做液体、形状与方块实体处理）。

- **拼图池构件**（`SingleJigsawPiece`）：构件在装配阶段已保证形状正确，必须 `settings.setKnownShape(true)`，并传 `UPDATE_CLIENTS | UPDATE_KNOWN_SHAPE`（18）。
- **手工实现的构件**（海洋废墟、沉船、雪屋、下界化石、废弃传送门、林地府邸、末地城，对应原版 `TemplateStructurePiece`）：必须保持 `knownShape=false`，并传 `UPDATE_CLIENTS`（2）。放置后要按邻居重算连接状态。
- 两者写反的症状都是「方块朝向/连接看似合理但对不上原版」：拼图误开形状重算，会在空世界把模板记录好的栅栏连接重算掉；手工构件误设 `knownShape=true`，则后放置的相邻构件无法刷新先放构件的连接状态。

### 4.1 液体处理分三段，缺一不可

1. 放置每个方块前读取原位流体；方块自身是流体源头（水方块或 `waterlogged=true`）时记入 `sourceFluidPositions`。
2. 否则若方块实现 `ILiquidContainer`，用原位流体尝试灌入，并把「原位不是源头」的位置记入 `fluidUpdatePositions`。
3. 全部方块落位后，对 `fluidUpdatePositions` 反复扫描上/北/东/南/西，从相邻水源（且不在 `sourceFluidPositions` 中）灌注，直到不再变化。

只实现第 2 段（只看放置前原位有没有非源流体）会漏掉「干燥容器被相邻水源补成水浸」：海洋废墟叠放构件时，前一个构件的数据标记写下水源，后一个构件的楼梯即使放在空气上也应变成 `waterlogged=true`。

### 5. 实体位置精度

实体有两个位置字段：
- `posx/posy/posz`（Double）：精确位置，用于实体放置
- `blockPos`（Int 列表）：方块坐标，用于方块对齐

不要只读取整数坐标。

**位置与朝向变换**：`placeInWorld` 中实体放置时，精确位置（f64）通过 `Template::transformEntityPos` 变换，对应 MC 1.21.11 `StructureTemplate#transform(Vec3, Mirror, Rotation, BlockPos)`。注意实体位置变换与方块位置变换（`transformBlockPos`）公式不同：

| 变换           | `transformBlockPos`（整数） | `transformEntityPos`（f64）     |
|----------------|-----------------------------|----------------------------------|
| 镜像（LeftRight/FrontBack） | `-coord`                    | `1.0 - coord`（block-corner 坐标系） |
| 旋转（90/180/270）          | 无 +1 偏移                  | 含 +1 偏移（保持子方块对齐）     |

朝向变换公式为 `f = rotate(yaw) + (mirror(yaw) - yaw)`，对应 MC 1.21.11 `Entity#rotate` + `Entity#mirror`。

**身体/头部朝向同步**：`placeInWorld` 中实体放置时，在 `setRotation(finalYaw, pitch)` 后会调用 `setYBodyRot(finalYaw)` / `setYHeadRot(finalYaw)` 同步身体与头部朝向到结构旋转后的 yaw，对应 MC 1.21.11 `StructureTemplate#placeEntities` 中的 `setYBodyRot(f)` / `setYHeadRot(f)`。`Entity` 基类的 `setYBodyRot` / `setYHeadRot` 为空实现，`LivingEntity` 重写后写入 `m_renderYawOffset` / `m_rotationYawHead` 字段，因此对任意实体类型调用都安全（非 `LivingEntity` 时为 no-op）。详见 `src/common/entity/core/README.md` 中 "setYBodyRot / setYHeadRot 虚方法" 章节。

### 6. 调色板格式兼容性

MC 1.16.5 支持单调色板（`palette`）和多调色板（`palettes`）两种格式。加载器同时支持，默认使用第一个调色板。

### 6.1 PaletteBlock 只装得下模板内坐标（u8）

`Palette` 存的是 `PaletteBlock`——8 字节紧凑表示，三轴坐标各占 **1 字节**，上限 255。它只表达**模板内相对坐标**。

- **不要把世界坐标塞进 `PaletteBlock`**：世界坐标可达 ±3000 万，会被静默截断到错误位置。需要世界坐标的是放置管线，用 `BlockInfo`（24 B，`BlockPos` + NBT 指针），二者不可混用。
- **加载期越界即拒载**：`TemplateLoader` 在两条加载路径（Java `.nbt` / 基岩版 `.mcstructure`）上都会调 `PaletteBlock::canRepresent()`，越界时打 error 日志并**拒绝整个模板**（返回空模板），而不是产出坐标错乱的建筑。结构模板体积上限为 48×48×48，实测 1202 个模板坐标最大 47，正常数据不会触发。
- **NBT 不在 `PaletteBlock` 内**：模板中仅约 0.5% 的方块带 NBT，为其内联指针不划算。NBT 存于 `Palette` 的稀疏表（键为方块下标，**必须递增**），用 `Palette::nbtAt(index)` 取，无 NBT 时返回 `nullptr`。`PaletteBlock::hasNbt()` 的 flag 由 `Palette` 依据稀疏表回填，不是独立状态。
- **多调色板模板会复制整份方块列表**：每个 palette 各持一份 `PaletteBlock` 数组（含各自 clone 的 NBT），20 个模板有 8 个 palette。改动加载逻辑时不要假设方块列表是共享的。

### 6.2 基岩 `.mcstructure` 的实体表与方块表是两条独立路径

`structure.entities` 是 **Compound**（键为任意字符串），值是 `{ unique_id: Int, entity: { identifier, pos: List<Double>[3], ... } }`；`pos` 是结构内相对坐标。加载器在解析方块表**之前**调用 `_parseBedrockEntities`，因为方块表在 `block_indices` 缺失/为空时会提前返回——把实体解析放在后面会让「只有实体、没有方块」的结构永远解析不出实体。缺 `identifier` 的条目会被跳过并记 warn，不会留下类型为空的 `TemplateEntityInfo`。

### 7. 位置哈希种子

IntegrityProcessor 等使用位置哈希进行确定性随机。必须使用 `MathUtils::hashBlockPos()` 而非简单取模，保证正确概率分布。

### 8. 处理器返回 nullopt 跳过方块

处理器链中任一处理器返回 `std::nullopt` 会跳过该方块。这是过滤方块的预期行为，不是错误。

### 9. 高度常量使用

【重要】必须使用 `mc::world::MIN_BUILD_HEIGHT`、`MAX_BUILD_HEIGHT` 等常量，禁止硬编码 0、256 等数字。

### 10. 区块尺寸常量使用

【重要】必须使用 `mc::world::CHUNK_WIDTH`、`CHUNK_HEIGHT`、`CHUNK_SECTION_HEIGHT` 等常量，禁止硬编码 16 等数字。

### 11. BlockAgeProcessor 标签匹配与属性保留

`BlockAgeProcessor` 使用 `BlockTags` 标签匹配替代硬编码方块指针比较：
- **楼梯方块**：通过 `BlockTags::STAIRS()` 标签匹配所有楼梯
- **台阶方块**：通过 `BlockTags::SLABS()` 标签匹配所有台阶
- **墙壁方块**：通过 `BlockTags::WALLS()` 标签匹配所有墙壁

替换时使用 `withPropertiesOf()` 保留原方块的共享属性（如朝向、含水状态等），确保楼梯/台阶/墙壁的朝向和形状不会丢失。

**注意**：`withPropertiesOf()` 只复制两个方块状态共有的属性，不会添加目标方块不支持的属性，也不会删除目标方块独有的属性。

### 12. 形状重算必须包含放置体积的外露面

`placeInWorld` 在 `knownShape=false` 时执行两步，缺一不可：

1. 用本次放置位置构造 `DiscreteVoxelShape`，对所有**外露面**调用两侧方块的 `updatePostPlacement`（`_updateShapeAtEdge`）；
2. 对本次放置的每个方块调用 `Block::updateFromNeighbourShapes`。

只做第 2 步会漏掉「后放置的构件紧贴先放置构件」的情况：先放构件的栅栏/墙/玻璃板连接状态仍停在模板记录值，新邻居不会触发它刷新（沉船跨构件栅栏连接的差异来源）。第 1 步的坐标必须来自 `DiscreteVoxelShape::forAllFaces`，其 FORWARD/BACKWARD 通道要用**逆循环**把体素坐标还原回来，否则面会落到错误的格子上（详见 `src/common/physics/shape/README.md`）。

### 13. 战利品容器 LootTableSeed 注入

`placeInWorld` 在加载方块实体 NBT 前，会对 `LootableContainerBlockEntity`（对应 MC Java `RandomizableContainer`，含 `ChestEntity`/`BarrelEntity`/`ShulkerBoxEntity`/`DispenserBlockEntity` 等）注入随机 `LootTableSeed`：

```cpp
if (dynamic_cast<LootableContainerBlockEntity*>(tileEntity) != nullptr) {
    processedBlock.nbt->put("LootTableSeed", rng.nextLong());
}
tileEntity->loadFromNBT(*processedBlock.nbt);
```

- 仅注入 `LootTableSeed`，`LootTable` 键已存在于模板 NBT 中（由数据包模板定义）。
- `BrushableBlockEntity`（可疑沙）不是 `LootableContainerBlockEntity`，其 `LootTableSeed` 由结构生成器（如 `DesertPyramidStructure`）通过 `setLootTable()` 直接设置，此处不干预。
- `LootableContainerBlockEntity::loadFromNBT` 负责读取 `LootTable`/`LootTableSeed` 并触发延迟填充机制。

**容器物品序列化**：`LootableContainerBlockEntity` 子类（`ChestEntity`/`BarrelEntity`/`ShulkerBoxEntity`/`DispenserBlockEntity`）已重写 `loadFromNBT`/`saveToNBT`，通过基类 protected 辅助方法 `saveItemsToNBT`/`loadItemsFromNBT` 序列化容器物品列表（`Items` NBT 键）。`LootTable`/`LootTableSeed` 与 `Items` 互斥：模板 NBT 中含 `LootTable` 时仅处理战利品表引用，否则加载/保存实际物品。结构模板放置预填充物品的容器（无战利品表）时物品得以正确保留。

### 14. 调色板选择用位置种子，不用 placement 传入的随机源

`Template::selectPalette(settings, pos)` 的随机源取 `settings.getRandom(&pos)`——未显式设置随机源时即 `LegacyRandomSource(Mth.getSeed(pos))`，**位置派生**、与 placement 传入的随机源无关，且**不消耗**它。

- 同一构件被多次放置（同一结构由多个区块各放一遍）时，调色板必须稳定；用结构随机源会让选择随处理次序漂移。
- 即使模板只有 1 个调色板，`nextInt(1)` 也会推进随机源——每个构件悄悄消耗一次随机数，会让后续构件/区块的随机序列整体错位。这类错位**不改变当前构件外观**，是"前若干个构件完全一致、之后突然发散"的隐蔽成因。

### 15. terrain_matching 投影贴地：公式两处都不能少

`GravityStructureProcessor`（`terrain_matching` 投影专用）的落点公式：

```
newY = 高度图取值(x, z) + offset + 模板内局部 Y
```

- **高度图取值是"首个可放置高度"（最高方块 Y + 1）**：`IWorld::getHeight` 返回的是"最高方块 Y"，故此处必须 `+1`（详见 `docs/PROJECT_CONVENTIONS.md` 的"高度图 +1 语义"）。
- **高度图类型来自处理器配置**：构造参数是 `HeightmapType`（不是裸整数），由处理器 JSON 的 `heightmap` 字段（如 `WORLD_SURFACE_WG`、`OCEAN_FLOOR_WG`）解析而来，落点查询走 `getHeight(x, z, type)`。原版在运行时世界（`ServerLevel`）会把 WG 变体换成非 WG 变体，项目尚无运行时结构放置路径，故不做该替换（`Template.cpp` 中留有 TODO）。
- **局部 Y 不可省**：整块模板沿 Y 平移，使局部 y=0 落在高度图处（村庄街道的路面在局部 y=0、jigsaw 在局部 y=1）。
- 漏掉任一项的症状不同：漏 `+1` 是整片构件**下移一格**；漏局部 Y 是构件被**压扁到同一层**（上层方块覆盖下层）。两者叠加时村庄路面 dirt_path 会被草方块盖住——玩家侧表现为"村庄里看不见路"，而构件包围盒、模板、旋转全都正确，只看结构起点的构件列表完全看不出来。

### 16. TemplateManager 是进程级单例，模板来源必须经绑定令牌注入

`JigsawAssembler::getTemplateManager()` 的实例跨服务端常驻，而它持有的数据包仓库、结构包资源源都是**宿主对象的非拥有指针**。

- **必须**经 `TemplateManagerHostBinding` 绑定（`TemplateManager` 的注入接口是 private + friend，绕过令牌无法编译）；宿主销毁时令牌自动解绑。
- **必须**在加载模板**之前**完成绑定：装配期的模板池加载会立即构造 `SingleJigsawPiece` 并读模板（`JigsawPiece::loadJointsFromTemplate`），绑定过晚会得到空连接点——结构只放起始块、不再扩展，且无任何报错。
- 结构源先于宿主析构时（如 `GameTestServer` 的派生类成员），**必须**显式解绑。
- 漏解绑的症状：下次读到模板时 SIGSEGV（`TemplateManager::_loadTemplate`），或 `std::system_error("mutex lock failed: Invalid argument")`——对象已析构但其内存尚未被复用，虚函数调用仍能派发进去，随后死在成员的已析构锁上。

### 17. 标签类处理器：数据包标签表为空时整段分支静默失效

`BlockAgeProcessor` / `ProtectedBlocksProcessor` 依赖 `BlockTags::STAIRS/SLABS/WALLS`、`FEATURES_CANNOT_REPLACE`。这些标签**只来自数据包**（`BlockTags` 内置表已清空，启动期由 `RegistryBootstrap::loadBlockDataPackTags` 重建）。

- 标签表未加载时 `contains()` 恒为 false：楼梯/台阶/墙全部不替换、受保护方块全部不保护，**没有任何报错**。
- 单元测试的全局环境（`tests/unit/main.cpp`）必须加载标签，否则用例的失败信息会指向处理器逻辑，而真正原因是环境没装数据。
- `ProtectedBlocksProcessor` 在标签未注册时返回 nullptr 并**透传**方块（"空标签"语义）——数据包缺失不该让结构生成崩掉，这是刻意取舍。

### 18. `BlockAgeProcessor` 的随机源只来自 `settings.getRandom(&pos)`，不能掺入结构原点

原版是 `p_74021_.getRandom(p_74020_.pos())`，即**只**用方块自身世界坐标派生（`LegacyRandomSource(Mth.getSeed(pos))`）。

- 把结构原点（`seedPos`）混进种子，会让同一模板在不同偏移处得到不同的苔藓化图案，与原版不符且不可复现。
- 与第 7 条并列看：`IntegrityProcessor` / `RuleStructureProcessor` 直接用 `RandomSource.create(Mth.getSeed(pos))`，与 placement 随机源无关；`BlockAgeProcessor` 走 `settings.getRandom(...)`，会**借用**预设随机源。两者的随机源口径不同，不要互相套用。

### 19. `settings.getRandom()` 返回 (借用指针, 自有指针)，借用指针可能为 nullptr

```cpp
auto [borrowed, owned] = settings.getRandom(&pos);
math::IRandom& rng = borrowed != nullptr ? *borrowed : *owned;
```

未设预设随机源时借用指针就是 **nullptr**，直接解引用即空指针解引用（表现为 ACCESS_VIOLATION at 0x0，栈落在调用点而非 `getRandom` 内部）。此时由 `owned` 持有对象，必须活到本次处理结束。


### 20. `CappedStructureProcessor` 的随机源是「世界种子 + 模板原点」的位置派生

原版 `CappedProcessor.finalizeProcessing`：

```java
RandomSource.create(level.getSeed()).forkPositional().at(templatePos)
```

它决定「哪几个命中方块被转换」，与结构随机源、`Mth.getSeed(pos)` 都不同。三条必须同时满足：

- **世界种子**：`PlacementSettings::getWorld()->seed()`；调用方没有 `setWorld` 时退化到 0，只适用于单元测试。
- **forkPositional 的 flavor**：`RandomSource.create(long)` 是 Legacy 版，`forkPositional()` 会先消耗一次 `nextLong()`；不要用 `math::Random` 直接代替。
- **洗牌顺序**：`Util.toShuffledList` 是**反向** Fisher-Yates（`j` 从 size 递减到 2，`nextInt(j)` 与 `j-1` 交换），不是正向、也不是 `std::shuffle`。

任何一项写错，转换数量可能仍然正确，但落点会整体漂移（海洋废墟的可疑沙砾就是这样暴露的：方块种类和数量都对，坐标对不上）。

### 21. `final_state` 解析必须从默认状态出发，只覆盖显式给出的属性

原版 `BlockStateParser.parseForBlock` 先取 `block.defaultBlockState()`，再逐属性 `setValue`；
未写出的属性保持默认值。不能实现成「枚举 `validStates()` 取第一个匹配给定子集的候选」——
那样未指定的属性会随状态枚举顺序取值。

- 症状：`final_state = minecraft:waxed_copper_bulb[lit=true]` 被解析成
  `lit=true,powered=true`（试炼密室的铜灯差异），而原版是 `lit=true,powered=false`。
- 修法是 `state = &state->withValueIndex(*prop, *parsedValue)`，逐个属性叠加。

### 22. `RuleStructureProcessor` 的输入谓词作用于「当前已处理状态」

原版 `RuleProcessor` 用 `blockInfo.state()`（链上处理器处理后的状态）做输入匹配，
`rawBlockInfo` 只提供模板内坐标。若误用原始状态，`JigsawReplacement` 产出的
`final_state` 永远不会参与规则匹配。

- 症状：堡垒遗迹里由拼图连接点 final_state 生成的 `polished_blackstone_bricks` 不会按
  `bastion_generic_degradation` 的 30% 概率变成裂纹砖；试炼密室的铜灯也不会走降级规则。
- 位置谓词的两个坐标不要混：`originalPos` 取 `rawBlockInfo.pos`（模板内），
  `worldPos` 取 `blockInfo.pos`（世界坐标）。

### 23. `placeInWorld` 的旋转/镜像必须绕 `PlacementSettings::getRotationPivot()` 展开

模板内的旋转枢轴不是世界原点。沉船、废弃传送门的枢轴在模板一侧，若用 `BlockPos(0, 0, 0)`
做枢轴，整批方块会平移到相邻区块甚至可写区之外。

- 症状：沉船的世界坐标整体偏移一格以上，探针记录到的方块数远少于原版（部分写入被边界过滤）。
- 实体放置同理，必须用同一枢轴换算。

### 24. 拼图方块的 `final_state` 替换不是自动的

`Template::placeInWorld()` 只按调色板原样放置方块，拼图连接点仍是 `minecraft:jigsaw`。
构造用的 `TemplateStructurePiece` 在放置后还要把每个 jigsaw 方块按 NBT 的 `final_state`
覆盖一次（缺省 `minecraft:air`），且必须排在数据标记之前/之后按各结构自身顺序执行。

- 症状：沉船、废弃传送门、海底废墟残留 `minecraft:jigsaw` 方块，原版该处是空气或指定方块。
- 用 `Template::getJigsawReplacements()` 取替换表（已按枢轴换算 + 边界框过滤），
  再 `applyJigsawReplacements()` 落盘；不要自己重写坐标变换。
