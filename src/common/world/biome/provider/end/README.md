# End Biome Provider

末地生物群系提供者，使用简单的 2D 噪声采样区分主岛和外岛区域。

## 目录结构

```
provider/end/
├── EndBiomeProvider.hpp  # 末地生物群系提供者声明
├── EndBiomeProvider.cpp  # 实现
└── README.md             # 本文档
```

## 设计原理

### 与主世界和下界的区别

| 特性 | 主世界 | 下界 | 末地 |
|------|--------|------|------|
| 采样方式 | 2D Layer 系统 | 3D 噪声采样 | 2D 噪声采样 |
| 垂直变化 | 无 | 有 | 无 |
| 主岛/外岛 | 无概念 | 无概念 | 有明显区分 |

### 主岛和外岛

末地生物群系分为两个主要区域：

1. **主岛（The End 主岛）**
   - 固定位置：中心在 (0, 0)
   - 半径：约 96 方块
   - 生物群系：The End
   - 特征：末地龙战斗区域、黑曜石柱、返回传送门

2. **外岛**
   - 距离主岛 1000+ 方块
   - 由噪声决定生物群系分布
   - 生物群系：Small End Islands, End Midlands, End Highlands, End Barrens
   - 特征：末地城、紫颂树、末地船

## 末地生物群系

| ID | 名称 | 英文名 | 特征 |
|----|------|--------|------|
| 9 | 末地 | The End | 主岛，末地龙战斗区域 |
| 40 | 小型末地岛屿 | Small End Islands | 外岛的小型岛屿群 |
| 41 | 末地中部 | End Midlands | 外岛过渡区域 |
| 42 | 末地高地 | End Highlands | 末地城、紫颂树 |
| 43 | 末地荒地 | End Barrens | 空旷区域，无特征 |

## 噪声参数

```cpp
// 岛屿噪声采样缩放
static constexpr f32 ISLAND_SCALE = 0.0078125f;  // 1/128

// 主岛半径（方块单位）
static constexpr i32 MAIN_ISLAND_RADIUS = 96;
```

## 生物群系选择算法

```
输入: x, z (世界坐标), noise (岛屿噪声 [-1, 1])

1. 如果在主岛范围内 (distance(x, z) <= 96):
      返回 The End

2. 外岛区域:
   - 如果 noise < -0.5:
       返回 Small End Islands
   - 如果 noise < 0:
       返回 End Barrens
   - 如果 noise < 0.5:
       返回 End Midlands
   - 否则:
       返回 End Highlands
```

## 使用示例

```cpp
#include "world/biome/provider/end/EndBiomeProvider.hpp"

// 创建末地生物群系提供者
mc::biome::end::EndBiomeProvider provider(seed);

// 获取单个位置的生物群系
mc::BiomeId biome = provider.getBiome(x, y, z);

// 检查是否在主岛
bool mainIsland = provider.isInMainIsland(x, z);

// 填充区块生物群系容器
mc::BiomeContainer container;
provider.fillBiomeContainer(container, chunkX, chunkZ);
```

## 依赖关系

```
EndBiomeProvider
├── BiomeProvider (基类)
├── SimplexNoiseGenerator (岛屿噪声)
├── BiomeRegistry (生物群系定义)
└── BiomeContainer (生物群系存储)
```

## 参考

- MC 1.16.5 `EndBiomeProvider` 类
- MC 1.16.5 `SimplexNoiseGenerator` 类
