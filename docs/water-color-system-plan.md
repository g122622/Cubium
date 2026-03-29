# 水体颜色系统实现计划

## 1. 概述

实现MC 1.16.5风格的水体颜色系统，使水体颜色随生物群系变化，同时建立可扩展的颜色系统架构。

## 2. 架构设计

### 2.1 目录结构

```
src/
├── common/
│   └── world/
│       └── biome/
│           ├── BiomeEffects.hpp/cpp     # 新增：生物群系视觉效果
│           └── Biome.hpp/cpp            # 修改：添加颜色属性
├── client/
│   ├── world/
│   │   └── color/                       # 新增：颜色系统
│   │       ├── ColorResolver.hpp        # 颜色解析器接口
│   │       ├── BiomeColors.hpp/cpp      # 生物群系颜色常量和解析器
│   │       └── BlockColors.hpp/cpp      # 方块颜色注册中心
│   └── renderer/
│       └── trident/
│           ├── chunk/
│           │   └── ChunkMesher.cpp      # 修改：水体颜色解析
│           └── fog/
│               └── FogManager.cpp       # 修改：水下雾颜色支持
```

### 2.2 类设计

```
┌─────────────────────────────────────────────────────────────────┐
│                        BiomeEffects                              │
│  - waterColor: u32          // 水体颜色 (ARGB)                   │
│  - waterFogColor: u32       // 水下雾颜色 (ARGB)                 │
│  - fogColor: u32            // 雾颜色 (ARGB)                     │
│  - skyColor: u32            // 天空颜色 (ARGB, 可选)             │
│  - foliageColor: Optional<u32>  // 树叶颜色覆盖                  │
│  - grassColor: Optional<u32>    // 草颜色覆盖                    │
│  - grassColorModifier: Enum     // 草颜色修改器                  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                          Biome                                   │
│  + effects(): const BiomeEffects&                               │
│  + waterColor(): u32                                            │
│  + waterFogColor(): u32                                         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      ColorResolver                               │
│  + getColor(biome: const Biome&, x: f64, z: f64): u32           │
└─────────────────────────────────────────────────────────────────┘
                              │
            ┌─────────────────┼─────────────────┐
            ▼                 ▼                 ▼
    ┌───────────────┐ ┌───────────────┐ ┌───────────────┐
    │ GrassColorRes │ │FoliageColorRes│ │ WaterColorRes │
    └───────────────┘ └───────────────┘ └───────────────┘
```

## 3. 实现步骤

### Phase 1: BiomeEffects 基础设施 (common模块)

#### 3.1.1 创建 BiomeEffects 类

**文件**: `src/common/world/biome/BiomeEffects.hpp`

```cpp
namespace mc {
namespace world {
namespace biome {

/**
 * @brief 草颜色修改器
 *
 * 某些生物群系会对草颜色应用特殊修改
 */
enum class GrassColorModifier : u8 {
    None,       // 无修改
    Swamp,      // 沼泽：基于噪声的双色混合
    DarkForest, // 黑森林：变暗
    Badlands    // 恶地：特殊颜色
};

/**
 * @brief 生物群系视觉效果配置
 *
 * 存储生物群系的视觉相关属性，包括水体颜色、雾颜色等。
 * 参考 MC 1.16.5 BiomeAmbience
 */
class BiomeEffects {
public:
    using OptionalColor = Optional<u32>;

    BiomeEffects() = default;

    // Builder模式
    class Builder {
    public:
        Builder& waterColor(u32 color);
        Builder& waterFogColor(u32 color);
        Builder& fogColor(u32 color);
        Builder& skyColor(u32 color);
        Builder& foliageColor(u32 color);
        Builder& grassColor(u32 color);
        Builder& grassColorModifier(GrassColorModifier modifier);
        BiomeEffects build();
    };

    // Getters
    u32 waterColor() const noexcept;
    u32 waterFogColor() const noexcept;
    u32 fogColor() const noexcept;
    u32 skyColor() const noexcept;
    OptionalColor foliageColor() const noexcept;
    OptionalColor grassColor() const noexcept;
    GrassColorModifier grassColorModifier() const noexcept;

    // 默认值
    static constexpr u32 DEFAULT_WATER_COLOR = 0x3F76E4;      // 默认水体颜色
    static constexpr u32 DEFAULT_WATER_FOG_COLOR = 0x050533;  // 默认水下雾颜色
    static constexpr u32 DEFAULT_FOG_COLOR = 0xC0D8FF;        // 默认雾颜色
    static constexpr u32 DEFAULT_SKY_COLOR = 0x78A7FF;        // 默认天空颜色

private:
    u32 m_waterColor = DEFAULT_WATER_COLOR;
    u32 m_waterFogColor = DEFAULT_WATER_FOG_COLOR;
    u32 m_fogColor = DEFAULT_FOG_COLOR;
    u32 m_skyColor = DEFAULT_SKY_COLOR;
    OptionalColor m_foliageColor;
    OptionalColor m_grassColor;
    GrassColorModifier m_grassColorModifier = GrassColorModifier::None;
};

} // namespace biome
} // namespace world
} // namespace mc
```

#### 3.1.2 修改 Biome 类

添加 BiomeEffects 成员和相关getter方法。

### Phase 2: 颜色解析系统 (client模块)

#### 3.2.1 ColorResolver 接口

**文件**: `src/client/world/color/ColorResolver.hpp`

```cpp
namespace mc {
namespace client {

/**
 * @brief 颜色解析器接口
 *
 * 函数式接口，根据生物群系和位置返回颜色值。
 * 参考 MC 1.16.5 ColorResolver
 */
class ColorResolver {
public:
    virtual ~ColorResolver() = default;

    /**
     * @brief 获取指定位置的颜色
     * @param biome 生物群系引用
     * @param x X坐标（用于噪声计算）
     * @param z Z坐标（用于噪声计算）
     * @return ARGB颜色值
     */
    virtual u32 getColor(const world::biome::Biome& biome, f64 x, f64 z) const = 0;
};

} // namespace client
} // namespace mc
```

#### 3.2.2 BiomeColors 颜色常量

**文件**: `src/client/world/color/BiomeColors.hpp`

```cpp
namespace mc {
namespace client {

/**
 * @brief 生物群系颜色解析器集合
 *
 * 提供草、树叶、水的颜色解析器实例。
 */
class BiomeColors {
public:
    // 草颜色解析器
    static const ColorResolver& grassColor();

    // 树叶颜色解析器
    static const ColorResolver& foliageColor();

    // 水颜色解析器
    static const ColorResolver& waterColor();

    // 特殊颜色常量
    static constexpr u32 SWAMP_GRASS_COLOR = 0x6A7039;
    static constexpr u32 SWAMP_FOLIAGE_COLOR = 0x6A7039;
    static constexpr u32 DARK_FOREST_GRASS_COLOR = 0x507A50;
    static constexpr u32 BADLANDS_GRASS_COLOR = 0x90814D;
    static constexpr u32 BADLANDS_FOLIAGE_COLOR = 0x9E814D;
};

} // namespace client
} // namespace mc
```

#### 3.2.3 BlockColors 注册中心

**文件**: `src/client/world/color/BlockColors.hpp`

```cpp
namespace mc {
namespace client {

/**
 * @brief 方块颜色注册中心
 *
 * 管理各类方块的颜色解析器，支持动态颜色（如水、草、树叶）。
 */
class BlockColors {
public:
    static BlockColors& instance();

    /**
     * @brief 初始化默认方块颜色
     *
     * 注册水、草、树叶等方块的颜色解析器
     */
    void initialize();

    /**
     * @brief 获取方块颜色
     * @param block 方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @param tintIndex 着色索引
     * @return ARGB颜色值，-1表示无颜色
     */
    i32 getColor(const world::block::BlockState* block,
                 const world::ClientWorld* world,
                 const glm::ivec3& pos,
                 i32 tintIndex) const;

private:
    // 使用函数指针映射，性能优于std::function
    using ColorGetter = i32(*)(const world::block::BlockState*,
                               const world::ClientWorld*,
                               const glm::ivec3&);

    std::unordered_map<world::block::BlockId, ColorGetter> m_colorGetters;
};

} // namespace client
} // namespace mc
```

### Phase 3: 渲染集成

#### 3.3.1 修改 ChunkMesher

在 `resolveTintColor` 中添加水体颜色处理：

```cpp
u32 ChunkMesher::resolveTintColor(...) {
    // ...

    // 水体颜色 (tintIndex == 2 或特定水方块)
    if (block->isLiquid() && block->is(VanillaBlocks::WATER)) {
        return biome.waterColor();
    }

    // ...
}
```

#### 3.3.2 修改 FogManager

支持水下雾颜色：

```cpp
void FogManager::updateUnderwaterFog(const Biome& biome) {
    m_waterFogColor = biome.waterFogColor();
    // ... 水下雾密度计算
}
```

### Phase 4: 生物群系颜色数据

#### 3.4.1 更新 BiomeRegistry

为每个生物群系设置正确的颜色：

| 生物群系 | 水体颜色 | 水下雾颜色 |
|---------|---------|-----------|
| 默认 | 0x3F76E4 | 0x050533 |
| 沼泽 | 0x617B64 | 0x232817 |
| 冻洋 | 0x3938C9 | 0x050533 |
| 暖水海洋 | 0x43D5EE | 0x041F33 |
| 温水海洋 | 0x45ADF2 | 0x0E4673 |
| 冷水海洋 | 0x3D57E6 | 0x1A3AA3 |
| 河流 | 0x3F76E4 | 0x050533 |

## 4. 测试计划

### 4.1 单元测试

- [ ] BiomeEffects 构建和默认值测试
- [ ] ColorResolver 接口实现测试
- [ ] BlockColors 注册和查询测试
- [ ] 颜色值正确性测试（对比MC Java值）

### 4.2 集成测试

- [ ] 不同生物群系水体颜色正确渲染
- [ ] 水下雾颜色正确应用
- [ ] 生物群系边界处颜色混合

## 5. 文件清单

| 文件 | 操作 | 描述 |
|------|------|------|
| `src/common/world/biome/BiomeEffects.hpp` | 新增 | 生物群系视觉效果定义 |
| `src/common/world/biome/BiomeEffects.cpp` | 新增 | 实现 |
| `src/common/world/biome/Biome.hpp` | 修改 | 添加BiomeEffects成员 |
| `src/common/world/biome/Biome.cpp` | 修改 | 实现getter |
| `src/common/world/biome/BiomeRegistry.cpp` | 修改 | 设置颜色属性 |
| `src/client/world/color/ColorResolver.hpp` | 新增 | 颜色解析器接口 |
| `src/client/world/color/BiomeColors.hpp` | 新增 | 生物群系颜色常量 |
| `src/client/world/color/BiomeColors.cpp` | 新增 | 实现 |
| `src/client/world/color/BlockColors.hpp` | 新增 | 方块颜色注册中心 |
| `src/client/world/color/BlockColors.cpp` | 新增 | 实现 |
| `src/client/world/color/README.md` | 新增 | 模块文档 |
| `src/client/renderer/trident/chunk/ChunkMesher.cpp` | 修改 | 水体颜色处理 |
| `src/client/renderer/trident/fog/FogManager.cpp` | 修改 | 水下雾颜色 |
| `tests/client/world/color/` | 新增 | 测试目录 |

## 6. 依赖关系

```
BiomeEffects (common)
    ↓
Biome (common)
    ↓
ColorResolver (client)
    ↓
BiomeColors, BlockColors (client)
    ↓
ChunkMesher, FogManager (client renderer)
```

## 7. 注意事项

1. **颜色格式**: 使用ARGB格式 (0xAARRGGBB)，与MC Java一致
2. **性能**: 颜色计算需要缓存，避免每帧重复计算
3. **生物群系混合**: 在生物群系边界处需要平滑过渡
4. **沼泽特殊处理**: 沼泽草色需要基于噪声的混合
5. **扩展性**: 设计应支持未来添加更多颜色类型（如红石、作物等）
