# Color 模块

本目录包含客户端颜色解析系统，负责从生物群系获取各种颜色值（草、树叶、水体等）。

## 目录结构

```
color/
├── ColorResolver.hpp    # 颜色解析器接口
├── BiomeColors.hpp      # 生物群系颜色解析器和常量
├── BiomeColors.cpp      # 实现文件
└── README.md            # 本文档
```

## 文件详细说明

### ColorResolver.hpp

**职责**：定义颜色解析器的抽象接口。

**主要类型**：
```cpp
class ColorResolver {
public:
    virtual ~ColorResolver() = default;
    virtual u32 getColor(const Biome& biome, f64 x, f64 z) const = 0;
};
```

**设计理念**：
- 函数式接口，支持基于位置的颜色计算
- 位置参数用于沼泽等需要噪声的颜色混合
- 返回 RGB 格式颜色 (0xRRGGBB)

### BiomeColors.hpp / BiomeColors.cpp

**职责**：实现具体的颜色解析器，提供颜色常量。

**主要类型**：

| 类型 | 描述 |
|------|------|
| `GrassColorResolver` | 草颜色解析器 |
| `FoliageColorResolver` | 树叶颜色解析器 |
| `WaterColorResolver` | 水颜色解析器 |
| `BiomeColors` | 颜色常量和工具函数 |

**颜色解析流程**：

```
BiomeColors.grassColorResolver().getColor(biome, x, z)
    │
    ├─ 检查 biome.effects().grassColor() 是否有覆盖
    │       ↓ 有：返回覆盖颜色
    │
    ├─ 检查 grassColorModifier()
    │       ├─ Swamp: 返回双色噪声混合
    │       ├─ DarkForest: 返回深绿色
    │       ├─ Badlands: 返回黄褐色
    │       └─ None: 返回 0xFFFFFFFF (需查 colormap)
    │
    └─ 调用方检测 0xFFFFFFFF，从 grass colormap 计算
```

## 颜色常量

### 特殊树叶颜色

| 方块 | 颜色值 | 说明 |
|------|--------|------|
| 云杉树叶 | `0x619961` | 固定颜色，不从 colormap 获取 |
| 桦树树叶 | `0x80A755` | 固定颜色，不从 colormap 获取 |

### 特殊生物群系颜色

| 生物群系 | 草颜色 | 树叶颜色 | 说明 |
|---------|--------|----------|------|
| 沼泽 | `0x6A7039` / `0x4C6139` | `0x6A7039` / `0x4C6139` | 双色噪声混合 |
| 黑森林 | `0x507A50` | 从 colormap | 草颜色变暗 |
| 恶地 | `0x90814D` | `0x9E814D` | 特殊黄褐色 |

### 水体颜色

| 生物群系 | 水体颜色 | 水下雾颜色 |
|---------|---------|-----------|
| 默认 | `0x3F76E4` | `0x050533` |
| 沼泽 | `0x617B64` | `0x232817` |
| 冻洋 | `0x3938C9` | `0x050533` |
| 暖水海洋 | `0x43D5EE` | `0x041F33` |
| 温水海洋 | `0x45ADF2` | `0x0E4673` |
| 冷水海洋 | `0x3D57E6` | `0x1A3AA3` |

## 模块关系图

```
┌────────────────────────────────────────────────────────────────┐
│                     ChunkMesher                                 │
│                          │                                      │
│                          ▼                                      │
│                   resolveTintColor()                            │
│                          │                                      │
│                          ▼                                      │
│                   BiomeColors.waterColorResolver()              │
│                          │                                      │
│                          ▼                                      │
│                   WaterColorResolver.getColor()                 │
│                          │                                      │
│                          ▼                                      │
│                      Biome.waterColor()                         │
│                          │                                      │
│                          ▼                                      │
│                    BiomeEffects                                 │
│                   (存储实际颜色值)                               │
└────────────────────────────────────────────────────────────────┘
```

## 与其他模块的交互

### 上游依赖

```
common/world/biome/Biome.hpp          # 生物群系定义
common/world/biome/BiomeEffects.hpp   # 视觉效果
common/util/math/random/Random.hpp    # 随机数（噪声计算）
```

### 下游依赖

```
client/renderer/trident/chunk/ChunkMesher.cpp  # 区块网格生成
client/world/ClientWorld.cpp                    # 世界颜色查询
```

## 使用方法

### 获取水体颜色

```cpp
#include "client/world/color/BiomeColors.hpp"

// 方式1：通过解析器
const Biome& biome = ...;
u32 waterColor = BiomeColors::waterColorResolver().getColor(biome, x, z);

// 方式2：直接从 BiomeEffects 获取
u32 waterColor = biome.waterColor();
```

### 获取草颜色（需要 colormap）

```cpp
#include "client/world/color/BiomeColors.hpp"

const Biome& biome = ...;
f64 x = blockPos.x;
f64 z = blockPos.z;

// 获取颜色
u32 color = BiomeColors::grassColorResolver().getColor(biome, x, z);

if (color == 0xFFFFFFFF) {
    // 需要从 grass colormap 计算
    f32 temperature = biome.temperature();
    f32 humidity = biome.humidity();
    color = getColorFromGrassColormap(temperature, humidity);
}

// 应用颜色到顶点
vertex.color = packColor(color);
```

### 沼泽颜色混合

```cpp
// 沼泽草/树叶颜色是双色调混合
u32 swampColor = BiomeColors::calculateSwampColor(
    blockPos.x, blockPos.z,
    0x6A7039,  // 浅色
    0x4C6139   // 深色
);
```

## 容易踩的坑

### 1. 颜色格式

**问题**：颜色格式混乱，有些用 ARGB，有些用 RGB。

**解决方案**：
- `BiomeEffects` 中存储的颜色是 RGB 格式（无 alpha）
- 渲染时添加 alpha 通道：`color | 0xFF000000`
- 顶点颜色打包使用 ARGB 格式

### 2. 沼泽颜色噪声

**问题**：沼泽颜色使用噪声，实现需要与 MC 一致。

**解决方案**：
- MC 使用 `PerlinNoiseGenerator.getValue(x * 0.0225, z * 0.0225)`
- 阈值是 `-0.1`
- 简化实现使用确定性哈希

### 3. Colormap 返回值

**问题**：`GrassColorResolver` 和 `FoliageColorResolver` 可能返回 `0xFFFFFFFF`。

**解决方案**：
- 调用方检测此值
- 如果返回 `0xFFFFFFFF`，从 colormap 计算
- colormap 基于温度和湿度索引

### 4. 位置参数

**问题**：为什么 `getColor` 需要 x 和 z 参数？

**解决方案**：
- 大多数生物群系不需要位置参数
- 沼泽使用位置进行双色混合
- 保持接口统一，非沼泽实现可忽略位置参数

## 测试用例

测试文件位于 `tests/client/world/color/`：

| 测试名称 | 描述 |
|---------|------|
| WaterColorResolver_ReturnsBiomeWaterColor | 验证水颜色解析器返回正确值 |
| GrassColorResolver_SwampUsesDualColor | 验证沼泽草颜色使用双色混合 |
| GrassColorResolver_DarkForestReturnsFixed | 验证黑森林返回固定颜色 |
| FoliageColorResolver_ReturnsOverride | 验证覆盖颜色优先级 |
| BiomeColorsConstants_Correct | 验证颜色常量值正确 |

## 参考资料

- Minecraft Wiki - Biome colors: https://minecraft.wiki/w/Color
- MC 1.16.5 BiomeColors.java
- MC 1.16.5 ColorResolver.java
- MC 1.16.5 BiomeAmbience.java
