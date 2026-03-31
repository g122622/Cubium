# Nether Biome Provider

下界生物群系提供者，使用 3D 噪声采样确定生物群系分布。

## 目录结构

```
provider/nether/
├── NetherBiomeProvider.hpp  # 下界生物群系提供者声明
├── NetherBiomeProvider.cpp  # 实现
└── README.md                # 本文档
```

## 设计原理

### 与主世界的区别

| 特性 | 主世界 (LayerBiomeProvider) | 下界 (NetherBiomeProvider) |
|------|----------------------------|---------------------------|
| 采样方式 | 2D Layer 系统 | 3D 噪声采样 |
| 生物群系分布 | 基于温度/湿度/大陆度 Layer | 基于温度/湿度/生物群系噪声 |
| 垂直变化 | 无（仅水平变化） | 有（不同高度不同生物群系）|
| 采样密度 | 1:4 (每个采样点 4x4 方块) | 1:4x4x4 (每个采样点 4x4x4 方块)|

### 3D 生物群系采样

下界生物群系使用 3D 噪声采样，允许生物群系在垂直方向变化：

```cpp
// 噪声坐标 = 方块坐标 >> 2
i32 noiseX = x >> 2;
i32 noiseY = y >> 2;
i32 noiseZ = z >> 2;

BiomeId biome = provider.getNoiseBiome(noiseX, noiseY, noiseZ);
```

## 下界生物群系

| ID | 名称 | 英文名 | 特征 |
|----|------|--------|------|
| 8 | 下界荒地 | Nether Wastes | 下界岩为主，猪灵、恶魂 |
| 170 | 灵魂沙谷 | Soul Sand Valley | 灵魂沙和灵魂土，蓝色迷雾，骷髅 |
| 171 | 绯红森林 | Crimson Forest | 绯红菌和疣猪兽，红色主题 |
| 172 | 诡异森林 | Warped Forest | 诡异菌和末影人，青色主题 |
| 173 | 玄武岩三角洲 | Basalt Deltas | 玄武岩和岩浆块，黑色颗粒 |

## 噪声参数

```cpp
// 温度噪声采样缩放
static constexpr f32 TEMPERATURE_SCALE = 0.015625f;  // 1/64

// 湿度噪声采样缩放
static constexpr f32 HUMIDITY_SCALE = 0.015625f;     // 1/64

// 生物群系选择噪声采样缩放
static constexpr f32 BIOME_SCALE = 0.0078125f;       // 1/128
```

## 生物群系选择算法

```
输入: temperature, humidity, biomeNoise (均为 [-1, 1] 范围)

1. 如果 biomeNoise > 0.5:
      返回玄武岩三角洲

2. 如果 temperature < -0.5 且 humidity < 0:
      返回灵魂沙谷

3. 如果 temperature < 0 且 humidity > 0:
      返回绯红森林

4. 如果 temperature > 0.5 且 humidity > 0:
      返回诡异森林

5. 否则:
      返回下界荒地
```

## 使用示例

```cpp
#include "world/biome/provider/nether/NetherBiomeProvider.hpp"

// 创建下界生物群系提供者
mc::biome::nether::NetherBiomeProvider provider(seed);

// 获取单个位置的生物群系
mc::BiomeId biome = provider.getBiome(x, y, z);

// 填充区块生物群系容器
mc::BiomeContainer container;
provider.fillBiomeContainer(container, chunkX, chunkZ);
```

## 依赖关系

```
NetherBiomeProvider
├── BiomeProvider (基类)
├── SimplexNoiseGenerator (温度/湿度噪声)
├── PerlinNoiseGenerator (生物群系噪声)
├── BiomeRegistry (生物群系定义)
└── BiomeContainer (生物群系存储)
```

## 参考

- MC 1.16.5 `NetherBiomeProvider` 类
- MC 1.16.5 `SimplexNoiseGenerator` 类
- MC 1.16.5 `PerlinNoiseGenerator` 类
