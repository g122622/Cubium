# FeatureSize 最小尺寸约束

本目录实现树木生成的最小空间占用规则。对应 MC 1.21.11 `net.minecraft.world.level.levelgen.feature.featuresize` 包。

## 目录结构

```
featuresize/
├── FeatureSize.hpp    # 基类 + TwoLayersFeatureSize + ThreeLayersFeatureSize（纯头文件）
└── README.md
```

## 核心概念

`TreeFeature::place` → `_calculateAvailableHeight`（对应 MC `getMaxFreeTreeHeight`）阶段会调用 `FeatureSize::getSizeAtHeight(trunkHeight, y)` 获取每一层 y 的水平检查半径，用于判断树干周围是否有足够空间放置。

### 两层特征尺寸（TwoLayersFeatureSize）

以 `limit` 为界：
- `y < limit`：返回 `lowerSize`
- `y >= limit`：返回 `upperSize`

数据包示例（acacia.json）：
```json
"minimum_size": {
  "type": "minecraft:two_layers_feature_size",
  "limit": 1,
  "lower_size": 0,
  "upper_size": 2
}
```

### 三层特征尺寸（ThreeLayersFeatureSize）

以 `limit` 与 `trunkHeight - upperLimit` 为界分三层：
- `y < limit`：返回 `lowerSize`
- `limit <= y < trunkHeight - upperLimit`：返回 `middleSize`
- `y >= trunkHeight - upperLimit`：返回 `upperSize`

数据包示例（dark_oak.json）：
```json
"minimum_size": {
  "type": "minecraft:three_layers_feature_size",
  "limit": 1,
  "lower_size": 0,
  "middle_size": 1,
  "upper_limit": 1,
  "upper_size": 2
}
```

### 最小裁剪高度（minClippedHeight）

`minClippedHeight` 为可选字段。当 `getMaxFreeTreeHeight` 返回的实际可用高度不足 `trunkHeight` 时，若 `minClippedHeight` 有值且实际高度 >= 该值，仍然允许树木生成（用裁剪后的高度）。用于 `fancy_oak` 等需要容忍较矮空间的配置。

## 上下游依赖

### 上游
- `mc::core::Types` - 基础类型（`i32`, `u8`）
- `std::optional<i32>` - 可选裁剪高度
- `std::unique_ptr` - `clone()` 深拷贝接口

### 下游
- `mc::TreeFeatureConfig` - 持有 `std::unique_ptr<FeatureSize> minimumSize`
- `mc::world::gen::feature::parser::FeatureSizeParser` - 从 JSON 解析 `minimum_size` 字段

## 容易踩的坑

### 1. ThreeLayersFeatureSize 的 upperLimit 是相对偏移

`upperLimit` 不是绝对 y 值，而是相对于 `trunkHeight` 的偏移：
```cpp
// 正确：中层到上层的阈值是 trunkHeight - upperLimit
return y >= trunkHeight - m_upperLimit ? m_upperSize : m_middleSize;
```
直接将 `upperLimit` 当作绝对 y 阈值会导致顶部层判断错误。

### 2. clone() 必须保留 minClippedHeight

`TreeFeatureConfig` 的拷贝构造/赋值依赖 `FeatureSize::clone()` 深拷贝。子类的 `clone()` 实现必须将 `m_minClippedHeight` 传递给新对象，否则拷贝后的配置会丢失裁剪能力。

### 3. minClippedHeight 为空表示不允许裁剪

当 JSON 中未指定 `min_clipped_height` 时，`minClippedHeight()` 返回 `std::nullopt`，表示实际可用高度不足时直接放弃生成（不进行裁剪）。不要将 `nullopt` 误当作"允许任意裁剪"。
