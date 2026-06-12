# Settings 模块 - 世界生成设置

本模块包含世界生成的配置设置，参考 Minecraft 1.16.5 的 DimensionSettings 和 NoiseSettings 实现。

## 目录结构

```
settings/
├── DimensionSettings.cpp    # 维度设置预设实现（主世界/下界/末地/平坦）
├── DimensionSettings.hpp    # 维度设置定义（默认方块、流体、海平面等）
├── NoiseSettings.hpp        # 噪声设置定义（地形噪声参数、尺寸、密度、滑动）
├── ScalingSettings.hpp      # 缩放设置定义（噪声在 XZ/Y 轴的缩放比例）
├── Settings.hpp             # 总头文件（聚合所有设置类型）
└── SlideSettings.hpp        # 滑动设置定义（地形边界平滑过渡参数）
```

## 内部模块关系

```
Settings.hpp（总入口）
    └── DimensionSettings（维度设置）
            └── NoiseSettings（噪声设置）
                    ├── ScalingSettings（缩放设置）
                    └── SlideSettings（滑动设置）
```

**依赖链**：`SlideSettings` + `ScalingSettings` → `NoiseSettings` → `DimensionSettings`

## 上下游外部依赖关系

### 本模块依赖

| 依赖项 | 用途 |
|--------|------|
| `common/core/Types.hpp` | 基础类型 i32, f32 等 |
| `common/world/WorldConstants.hpp` | world::MAX_BUILD_HEIGHT, world::SEA_LEVEL |
| `common/world/block/Block.hpp` | BlockState 前向声明 |
| `common/world/block/BlockRegistry.hpp` | 方块注册表访问 |
| `common/world/block/registry/VanillaBlocks.hpp` | 原版方块定义（石头、水、熔岩等） |

### 被依赖

| 使用者 | 用途 |
|--------|------|
| `world/gen/NoiseChunkGenerator` | 噪声区块生成器，核心使用者 |
| `world/gen/IChunkGenerator` | 区块生成器接口 |
| `world/gen/BaseChunkGenerator` | 区块生成器基类 |
| `server/ServerChunkManager` | 服务端区块管理器初始化 |

## 容易踩的坑

### 1. BlockState 指针有效性

`DimensionSettings::overworld()` 等预设方法在调用时从 `VanillaBlocks::getState()` 获取 `BlockState*`。**必须在方块注册完成后调用**，否则返回 `nullptr`。

```cpp
// ❌ 错误：方块注册前获取
DimensionSettings settings = DimensionSettings::overworld();  // defaultBlock 为 nullptr

// ✅ 正确：方块注册后获取
BlockRegistry::instance().initialize();
VanillaBlocks::registerAll();
DimensionSettings settings = DimensionSettings::overworld();  // defaultBlock 有效
```

### 2. NoiseSettings 默认值不适合所有维度

`NoiseSettings` 的默认构造值是主世界参数，其他维度需使用对应预设：

```cpp
// ❌ 错误：默认值用于下界
NoiseSettings noise;  // 主世界参数

// ✅ 正确：使用预设
NoiseSettings noise = NoiseSettings::nether();
```

### 3. 海平面高度与噪声高度的配合

`seaLevel` 必须在噪声高度范围内，否则水体生成异常。

### 4. 下界的特殊基岩设置

下界需要设置 `bedrockRoof = 127` 和 `bedrockFloor = 0`，其他维度默认即可。

### 5. 密度参数的影响

- `densityFactor > 0`：密度随深度增加
- `densityOffset < 0`：增加空气空间
- 主世界：`densityFactor = 1.0`, `densityOffset = -0.46875`
- 下界：`densityFactor = 0.0`, `densityOffset = 0.019921875`

### 6. NoiseSettings.height 使用常量

`NoiseSettings::overworld()` 的 `height` 使用 `world::MAX_BUILD_HEIGHT` 常量，而非硬编码 256。未来如果高度限制变化，无需修改此模块。
