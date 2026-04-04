# Biome Provider Directory

生物群系提供者目录，按维度隔离组织。

## 目录结构

```
provider/
├── README.md                    # 本文档
├── overworld/                   # 主世界生物群系提供者
│   ├── LayerBiomeProvider.hpp   # Layer 系统生物群系提供者
│   ├── LayerBiomeProvider.cpp
│   └── README.md
├── nether/                      # 下界生物群系提供者
│   ├── NetherBiomeProvider.hpp  # 3D 噪声生物群系提供者
│   ├── NetherBiomeProvider.cpp
│   └── README.md
└── end/                         # 末地生物群系提供者
    ├── EndBiomeProvider.hpp     # 原版岛屿高度函数生物群系提供者
    ├── EndBiomeProvider.cpp
    └── README.md
```

## 命名空间

每个维度的生物群系提供者位于独立命名空间：

| 目录 | 命名空间 |
|------|----------|
| `overworld/` | `mc::biome::overworld` |
| `nether/` | `mc::biome::nether` |
| `end/` | `mc::biome::end` |

## 维度对比

| 特性 | 主世界 | 下界 | 末地 |
|------|--------|------|------|
| 提供者 | LayerBiomeProvider | NetherBiomeProvider | EndBiomeProvider |
| 采样方式 | 2D Layer 系统 | 3D 噪声采样 | 2D 噪声采样 |
| 生物群系数 | 170+ | 5 | 5 |
| 垂直变化 | 无 | 有 | 无 |
| 主要噪声 | 温度/湿度/大陆度 Layer | 温度/湿度/生物群系噪声 | 岛屿高度函数 + Simplex 噪声 |

## 使用示例

```cpp
#include "world/biome/provider/overworld/LayerBiomeProvider.hpp"
#include "world/biome/provider/nether/NetherBiomeProvider.hpp"
#include "world/biome/provider/end/EndBiomeProvider.hpp"

// 主世界
mc::biome::overworld::LayerBiomeProvider overworldProvider(seed);

// 下界
mc::biome::nether::NetherBiomeProvider netherProvider(seed);

// 末地
mc::biome::end::EndBiomeProvider endProvider(seed);

// 获取生物群系
mc::BiomeId biome = provider.getBiome(x, y, z);
```

## 依赖关系

```
BiomeProvider (基类)
├── overworld/LayerBiomeProvider
│   ├── Layer 系统
│   └── BiomeRegistry
├── nether/NetherBiomeProvider
│   ├── SimplexNoiseGenerator
│   ├── PerlinNoiseGenerator
│   └── BiomeRegistry
└── end/EndBiomeProvider
    ├── SimplexNoiseGenerator
    └── BiomeRegistry
```
