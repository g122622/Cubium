# 方块状态模式匹配系统

3D 方块模式匹配系统，用于在世界中查找特定方块结构布局。对应 MC Java 的
`net.minecraft.world.level.block.state.pattern` 包。

## 文件

| 文件 | 说明 |
|------|------|
| `BlockInWorld.hpp/cpp` | 世界中方块的轻量级缓存包装器，延迟加载方块状态和方块实体 |
| `BlockPattern.hpp/cpp` | 3D 方块模式，支持 `find()` 搜索匹配和 `matches()` 定向匹配 |
| `BlockPatternBuilder.hpp/cpp` | 流式构建器，通过 `aisle()` 和 `where()` 构建模式 |

## 使用示例

### 构建模式

```cpp
using namespace mc::blockpattern;

auto pattern = BlockPatternBuilder::start()
    .aisle({"       ", "       ", "       ", "   #   ", "       ", "       ", "       "})
    .aisle({"       ", "       ", "       ", "   #   ", "       ", "       ", "       "})
    .aisle({"       ", "       ", "       ", "   #   ", "       ", "       ", "       "})
    .aisle({"  ###  ", " #   # ", "#     #", "#  #  #", "#     #", " #   # ", "  ###  "})
    .aisle({"       ", "  ###  ", " ##### ", " ##### ", " ##### ", "  ###  ", "       "})
    .where('#', BlockInWorld::hasState([](const BlockState& s) {
        return s.is(VanillaBlocks::BEDROCK);
    }))
    .build();
```

### 搜索模式

```cpp
auto match = pattern->find(world, BlockPos(0, 64, 0));
if (match.has_value()) {
    // 获取模式内指定位置的方块
    blockpattern::BlockInWorld block = match->getBlock(widthIdx, heightIdx, depthIdx);
    BlockPos pos = block.pos();
    const BlockState* state = block.getState();
}
```

## 架构设计

### BlockInWorld

- **延迟加载**：`getState()` 和 `getEntity()` 首次调用时查询世界，结果缓存
- **区块感知**：`loadChunks=false` 时，未加载区块返回 `nullptr`（避免强制加载）
- **谓词工厂**：`hasState(predicate)` 返回接受 `BlockInWorld` 的谓词，先获取状态再判断

### BlockPattern

- **三维谓词数组**：`pattern[depth][height][width]`，索引顺序与 MC Java 一致
- **方向旋转**：`find()` 遍历所有正交 `(forwards, up)` 方向组合（24 种），支持任意朝向匹配
- **坐标转换**：`_translateAndRotate(origin, forwards, up, i, j, k)` 将模式坐标映射到世界坐标
  - 公式：`pos = origin + up * (-j) + (forwards × up) * i + forwards * k`

### BlockPatternBuilder

- **链式 API**：`start().aisle(...).where(...).build()`
- **校验**：`build()` 时检查所有字符均已绑定谓词，高度/宽度一致性
- **默认字符**：`' '`（空格）默认匹配任何方块

## 与 MC Java 的差异

| MC Java | Cubium | 原因 |
|---------|--------|------|
| `Predicate<BlockInWorld>` | `std::function<bool(const BlockInWorld&)>` | C++ 无函数式接口，使用 std::function |
| `Predicate<BlockState>` | `std::function<bool(const BlockState&)>` | 同上 |
| `LoadingCache<BlockPos, BlockInWorld>` | `BlockInWorld` 内部缓存 | C++ 无 Guava Cache，改为 BlockInWorld 自身缓存 |
| `@Nullable BlockPatternMatch` | `std::optional<BlockPatternMatch>` | C++ 语义化空值 |
| `BlockPredicate.forBlock(block)` | `[](const BlockState& s){ return s.is(block); }` | Cubium 无 BlockPredicate 层级 |

## 主要使用方

- `EndDragonFight::_findExitPortal()` - 通过 `exitPortalPattern` 精确定位末地出口讲台
- `EndDragonFight::_respawnDragon()` - 遍历匹配的讲台方块，替换基岩/末地传送门为末地石

## 性能考虑

- `find()` 的搜索范围为 `[pos, pos + (max-1)^3]`，其中 `max = max(width, height, depth)`
- 每个候选位置尝试 24 种方向组合，最坏情况下复杂度为 `O(max^3 * 24 * pattern_size)`
- `BlockInWorld` 缓存避免重复查询世界，但每次 `find()` 仍会创建大量临时对象
- 对于已知大致位置的场景（如 `EndDragonFight`），优先在 END_PORTAL 方块位置附近搜索
