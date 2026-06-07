# Template 模板系统

结构模板系统，用于加载、存储和放置 NBT 格式的结构模板文件。主要用于 Jigsaw 结构生成。

## 目录结构

```
template/
├── Template.hpp                      # 模板类（BlockInfo、PlacementSettings、StructureProcessor、Palette等）
├── Template.cpp                      # 模板类实现
├── TemplateLoader.hpp                # NBT 模板加载器，从资源包加载 .nbt 文件
├── TemplateLoader.cpp                # 模板加载实现
├── TemplateManager.hpp               # 模板管理器（缓存、资源包集成）
├── TemplateManager.cpp               # 模板管理实现
├── RuleTest.hpp                      # 规则测试类（用于 RuleStructureProcessor）
├── RuleTest.cpp                      # 规则测试实现
├── CopperBulbDegradationProcessor.hpp # 铜灯降级处理器（试炼密室用）
└── CopperBulbDegradationProcessor.cpp # 铜灯降级处理器实现
```

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
                                      ├── BlockAgeProcessor
                                      ├── BlackstoneReplacementProcessor
                                      └── CopperBulbDegradationProcessor
```

**关键依赖链**：
- `TemplateLoader` 解析 NBT 文件创建 `Template` 对象
- `TemplateManager` 提供模板缓存和资源包集成
- `Template::place()` 遍历方块并调用 `StructureProcessor` 链处理
- `RuleStructureProcessor` 使用 `RuleTest` 进行条件匹配

## 上下游外部依赖关系

### 上游依赖（本模块依赖的外部模块）

- `mc::core` - 基础类型（u32, i32, f32 等）
- `mc::util::math::Random` - 确定性随机数生成
- `mc::world::block` - BlockState、Block、BlockRegistry、VanillaBlocks
- `mc::world::gen::structure` - StructureBoundingBox
- `mc::resource` - ResourceLocation、IResourcePack、DataPackList
- `mc::nbt` - NBT 解析（CompoundTag、ListTag）
- `mc::util::Direction` - Rotation、Mirror 枚举

### 下游依赖（依赖本模块的外部模块）

- `mc::world::gen::jigsaw` - JigsawManager、JigsawPiece 使用模板放置拼图块
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
