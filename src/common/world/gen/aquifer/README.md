# 含水层系统 (Aquifer System)

在噪声地形生成过程中，确定每个方块位置是否应该被流体（水/熔岩）替代。当 finalDensity < 0（空腔）时，含水层系统决定空腔内填充水、熔岩还是空气。

## 目录结构

```
aquifer/
├── Aquifer.hpp/cpp               # 含水层抽象基类（含工厂方法 createNoiseBased/createDisabled）
├── Aquifers.hpp                  # 便捷包含头文件（引入所有含水层头文件）
├── DisabledAquifer.hpp/cpp       # 禁用含水层的空实现（下界/末地使用）
├── FluidPickerFactory.hpp/cpp    # 流体选择器工厂函数（createOverworld/Nether/EndFluidPicker）
├── FluidStatus.hpp/cpp           # 流体状态记录 + FluidPicker 类型别名
└── NoiseBasedAquifer.hpp/cpp     # 基于噪声的含水层实现（主世界使用）
```

## 内部模块关系

```
FluidStatus.hpp（FluidStatus 结构体 + FluidPicker 类型别名）
    │
    ├── Aquifer.hpp（含水层抽象基类，工厂方法返回 NoiseBasedAquifer/DisabledAquifer）
    │       │
    │       ├── NoiseBasedAquifer.hpp（主世界含水层实现，依赖密度函数和噪声系统）
    │       └── DisabledAquifer.hpp（空实现，直接返回全局流体）
    │
    └── FluidPickerFactory.hpp（维度特定的流体选择器工厂函数）
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `block/registry/VanillaBlocks` | 空气、水、熔岩方块状态（FluidStatus::at、流体类型判断） |
| `density/NoiseChunk` | 噪声区块数据（NoiseBasedAquifer 采样地表高度） |
| `density/NoiseRouter` | 噪声路由器（提供 barrier/fluidLevel/lava 等密度函数） |
| `noise/NormalNoise` | 噪声生成 |
| `util/math/` | 数学工具（floorDiv、clamp、map） |
| `util/math/random/` | 位置随机工厂（含水层中心位置随机化） |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `density/NoiseChunk` | 持有 Aquifer 实例，通过 AquiferFiller 填充流体 |
| `chunk/NoiseChunkGenerator` | 创建含水层实例（根据维度选择 NoiseBased/Disabled），配置流体选择器 |
| `carver/WorldCarver` | 雕刻时查询含水层确定空腔填充 |

## 容易踩的坑

### 1. 含水层网格坐标与区块坐标不同

含水层使用 16×12×16 的网格间距（X_SPACING=16, Y_SPACING=12, Z_SPACING=16），且有 SAMPLE_OFFSET 偏移。不要混淆网格坐标和区块坐标，转换必须使用 gridX/gridY/gridZ 和 fromGridX/fromGridY/fromGridZ。

### 2. AquiferStatus 缓存是扁平数组

缓存索引计算为 `(gridY - minGridY) * gridSizeZ * gridSizeX + (gridZ - minGridZ) * gridSizeX + (gridX - minGridX)`，越界时会返回默认值而非崩溃。调试缓存命中率时需要注意这一点。

### 3. FluidPicker 返回 FluidStatus，不是 BlockState

FluidPicker 返回 FluidStatus，其中 fluidLevel 和 fluidType 组合表示"此处的流体液面高度和类型"。需要调用 `FluidStatus::at(y)` 才能得到具体的 BlockState。直接使用 `fluidType` 而不检查 `fluidLevel` 会导致所有 Y 高度都返回流体。
