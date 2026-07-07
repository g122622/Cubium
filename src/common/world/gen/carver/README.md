# 雕刻器模块 (Carver)

本模块实现了地形雕刻系统，用于生成洞穴、峡谷等地下结构。

## 目录结构

```
carver/
├── WorldCarver.hpp             # 雕刻器模板基类 + ConfiguredCarver 类型擦除
├── WorldCarver.cpp             # 基类实现
├── CaveCarver.hpp              # 洞穴雕刻器
├── CaveCarver.cpp              # 洞穴雕刻器实现
├── CanyonCarver.hpp            # 峡谷雕刻器
├── CanyonCarver.cpp            # 峡谷雕刻器实现
├── NetherWorldCarver.hpp       # 下界雕刻器
├── NetherWorldCarver.cpp       # 下界雕刻器实现
├── CarverConfiguration.hpp     # 配置结构体 + 工厂函数
├── CarverConfiguration.cpp     # 配置工厂实现
├── CarvingContext.hpp          # 雕刻上下文（含水层引用、topMaterial）
├── CarvingContext.cpp          # 雕刻上下文实现（topMaterial 生物群系地表查询）
├── CarvingMask.hpp             # 雕刻掩码声明
├── CarvingMask.cpp             # 雕刻掩码实现
├── ConfiguredCarverRegistry.hpp # 配置化雕刻器注册表（数据驱动，按 ResourceLocation 索引）
├── ConfiguredCarverRegistry.cpp # 注册表实现
├── ConfiguredCarverLoader.hpp  # 从数据包加载 configured_carver JSON
├── ConfiguredCarverLoader.cpp  # Loader 实现（解析 4 个 carver config）
└── Carvers.hpp                 # 便捷包含头文件
```

---

## 内部模块关系

```
WorldCarver<Config>          雕刻器模板基类
├── CaveCarver               洞穴雕刻器
│   └── NetherWorldCarver    下界雕刻器
└── CanyonCarver             峡谷雕刻器

ConfiguredCarverBase         类型擦除基类
ConfiguredCarver<Carver, Config>  配置化雕刻器

CarverConfiguration          基础配置
├── CaveCarverConfiguration  洞穴配置
└── CanyonCarverConfiguration 峡谷配置
    └── CanyonShapeConfiguration 峡谷形状配置

ConfiguredCarvers            默认配置工厂（createOverworldCaveConfig 等）
```

**继承关系**：
- `WorldCarver<Config>` 是模板基类，定义 `carve`/`shouldCarve`/`carveEllipsoid` 等接口
- `CaveCarver` 和 `CanyonCarver` 是两个主要的具体实现
- `NetherWorldCarver` 继承 `CaveCarver`，为下界维度定制（不使用含水层、熔岩阈值 Y+31）
- 水下雕刻通过配置区分（使用包含水下方块的 replaceable tag + `shouldCheckForFluid()=false`），不需要单独子类

---

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 依赖 | 用途 |
|------|------|
| `ChunkPrimer` | 区块数据存储和修改 |
| `BiomeSource` | 获取生物群系信息 |
| `BlockRegistry` / `VanillaBlocks` | 方块注册表和原版方块定义 |
| `Random` | 随机数生成 |
| `MathUtils` | 数学常量 (PI, TWO_PI 等) |
| `WorldConstants` | 世界常量（MIN_BUILD_HEIGHT, MAX_BUILD_HEIGHT 等） |
| `Aquifer` | 含水层系统 |
| `WorldGenerationContext` | 世界生成上下文基类 |

### 下游依赖（谁依赖本模块）

| 依赖方 | 用途 |
|--------|------|
| `NoiseChunkGenerator` | 区块生成器，在 CARVERS 阶段调用雕刻器 |
| `BiomeGenerationSettings` | 存储配置化雕刻器列表 |
| `ChunkPrimer` | 包含 `CarvingMask` 头文件 |

---

## 容易踩的坑

### 1. 雕刻掩码坐标必须与区块坐标一致
`CarvingMask` 构造时传入的 chunkX/chunkZ 必须与雕刻时使用的坐标一致，否则掩码索引会错误。

### 2. 雕刻器可能影响相邻区块
`getRange()` 返回影响范围（默认 4），实际影响范围: `(range * 2 - 1) * 16` 方块。需要确保相邻区块已加载。

### 3. 随机种子一致性
使用确定性随机种子以确保相同输入产生相同输出：
```cpp
math::Random rng(static_cast<u64>(chunkX) * 341873128712ULL +
                 static_cast<u64>(chunkZ) * 132897987541ULL);
```

### 4. 液体检测与下界雕刻器
`shouldCheckForFluid()` 默认检测区域内的水/熔岩，存在液体时会阻止雕刻。`NetherWorldCarver` 重写返回 false，可在熔岩中雕刻。

### 5. Y 坐标与熔岩填充
主世界：`config.lavaLevel`（默认 `aboveBottom(8)`）以下的雕刻区域填充熔岩。下界：`NetherWorldCarver::getCarveState()` 中 Y <= minY + 31 填充熔岩。

### 6. 高度常量不要硬编码
使用 `mc::world::MIN_BUILD_HEIGHT` 和 `mc::world::MAX_BUILD_HEIGHT`，不要硬编码 0、256 等数字。

### 7. 草地表面替换逻辑
`handlesSurfaceReplacement()` 控制是否在雕刻后将草方块/菌丝替换为泥土。主世界雕刻器返回 true（默认），下界雕刻器返回 false。

### 8. CarvingContext 与含水层系统
通过 `CarvingContext` 访问含水层（Aquifer），决定雕刻后填充空气、水还是熔岩。含水层返回 nullptr 表示不雕刻该位置。`NetherWorldCarver` 不使用含水层系统。

### 9. CarvingContext.topMaterial() 生物群系地表查询
`CarvingContext::topMaterial()` 根据指定位置查询生物群系的地表方块，用于雕刻后表面替换：
- 当草地/菌丝被雕刻后，下方泥土替换为生物群系对应的地表方块（沙漠→沙子、恶地→红砂岩等）
- `hasFluid=true` 时优先返回 `biome.underWaterBlock()`（水下地表方块）
- 当前通过 `Biome::surfaceBlock()/underWaterBlock()` 实现，与 MC 原版 SurfaceRules 评估等效

### 9. CarveSkipChecker 回调机制
椭球跳过逻辑通过 `CarveSkipChecker` 回调实现，而非虚方法。洞穴使用基于 `floorLevel` 的 dy 检查，峡谷使用高度相关的宽度因子。回调参数使用 `CarverEllipsePos` 结构体封装。
