# Template 模板系统

结构模板系统，用于加载、存储和放置 NBT 格式的结构模板文件。主要用于 Jigsaw 结构生成。

## 目录结构

```
template/
├── Template.hpp                      # 模板类（BlockInfo、PlacementSettings、StructureProcessor、Palette等）
├── Template.cpp                      # 模板类实现（含所有处理器实现）
├── TemplateLoader.hpp                # NBT 模板加载器，从资源包加载 .nbt 文件
├── TemplateLoader.cpp                # 模板加载实现
├── TemplateManager.hpp               # 模板管理器（缓存、资源包集成）
├── TemplateManager.cpp               # 模板管理实现
├── RuleTest.hpp                      # 规则测试类（用于 RuleStructureProcessor）
├── RuleTest.cpp                      # 规则测试实现
├── CappedStructureProcessor.hpp  # 限制次数处理器（limit 支持 IntProvider 随机范围）
├── CappedStructureProcessor.cpp  # 限制次数处理器实现
├── BlockAgeProcessor.hpp             # [未编译] 方块老化处理器独立声明（实现在 Template.cpp）
├── BlockAgeProcessor.cpp             # [未编译] 方块老化处理器独立实现（实现在 Template.cpp）
├── BlackstoneReplacementProcessor.hpp # [未编译] 黑石替换处理器
├── BlackstoneReplacementProcessor.cpp # [未编译] 黑石替换处理器
├── BlockIgnoreStructureProcessor.hpp # [未编译] 方块忽略处理器
├── BlockIgnoreStructureProcessor.cpp # [未编译] 方块忽略处理器
├── GravityStructureProcessor.hpp     # [未编译] 重力处理器
├── GravityStructureProcessor.cpp     # [未编译] 重力处理器
├── IntegrityProcessor.hpp            # [未编译] 完整度处理器
├── IntegrityProcessor.cpp            # [未编译] 完整度处理器
├── JigsawReplacementStructureProcessor.hpp # [未编译] 拼图替换处理器
├── JigsawReplacementStructureProcessor.cpp # [未编译] 拼图替换处理器
├── LavaSubmergingProcessor.hpp       # [未编译] 岩浆淹没处理器
├── LavaSubmergingProcessor.cpp       # [未编译] 岩浆淹没处理器
├── NopStructureProcessor.hpp         # [未编译] 空处理器
├── NopStructureProcessor.cpp         # [未编译] 空处理器
├── BlockInfo.hpp                     # [未编译] 方块信息
├── BlockInfo.cpp                     # [未编译] 方块信息
├── PlacementSettings.hpp             # [未编译] 放置设置
├── PlacementSettings.cpp             # [未编译] 放置设置
├── StructureProcessor.hpp            # [未编译] 处理器基类
├── StructureProcessor.cpp            # [未编译] 处理器基类
├── TemplateEntityInfo.hpp            # [未编译] 模板实体信息
└── TemplateEntityInfo.cpp            # [未编译] 模板实体信息
```

**注意**：标记为 `[未编译]` 的文件不在 CMakeLists.txt 编译列表中，其功能已在 `Template.hpp/cpp` 中统一实现。这些文件保留作为参考，未来可能重构为独立编译单元。

## 内部模块关系

```
TemplateLoader ──加载──> Template ──缓存──> TemplateManager
                               │
                               ├── Palette（多调色板支持）
                               ├── BlockInfo / ProcessedBlockInfo
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
- `TemplateManager` 提供模板缓存和资源包集成
- `Template::place()` 遍历方块并调用 `StructureProcessor` 链处理
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

### 4. 方块更新标志不当

放置方块后可能产生过多更新。使用适当的标志（默认 18：通知邻居和观察者）。

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

### 12. 未编译的独立处理器文件

目录中存在多个标记为 `[未编译]` 的独立处理器文件（如 `BlockAgeProcessor.hpp/cpp`、`BlackstoneReplacementProcessor.hpp/cpp` 等），这些文件不在 `CMakeLists.txt` 编译列表中。它们的功能已在 `Template.hpp/cpp` 中统一实现。修改处理器逻辑时，务必修改 `Template.hpp/cpp` 中的实现，而非仅修改独立文件。

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
