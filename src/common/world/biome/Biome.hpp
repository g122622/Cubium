/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "BiomeAmbientSounds.hpp"
#include "BiomeClimate.hpp"
#include "BiomeEffects.hpp"
#include "BiomeGenerationSettings.hpp"
#include "common/core/Types.hpp"
#include "common/util/cache/Long2FloatLRUCache.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"
#include <string>
#include <string_view>
#include <utility>

// 前向声明（必须在 mc::world::biome 命名空间之外，避免命名空间污染）
namespace mc {
class BlockState;
class IWorld;
} // namespace mc

namespace mc {
namespace world {
namespace biome {

// 跨命名空间引用简写
using MobSpawnInfo = ::mc::world::spawn::MobSpawnInfo;
using BlockState = ::mc::BlockState;

/**
 * @brief 生物群系定义
 *
 * 存储单个生物群系的生成参数。
 */
class Biome {
public:
    /**
     * @brief 默认构造函数（用于容器）
     */
    Biome() = default;

    /**
     * @brief 生物群系构造函数
     * @param id 生物群系ID
     * @param name 生物群系名称
     */
    Biome(BiomeId id, std::string_view name) noexcept;

    // === 基本信息 ===
    [[nodiscard]] BiomeId id() const { return m_id; }
    [[nodiscard]] const std::string& name() const { return m_name; }

    // === 地形参数 ===
    [[nodiscard]] f32 depth() const { return m_depth; }
    [[nodiscard]] f32 scale() const { return m_scale; }

    // === 气候参数 ===
    [[nodiscard]] const BiomeClimate& climate() const { return m_climate; }

    /**
     * @brief 获取指定位置的高度调整温度（带缓存）
     *
     * 使用 ThreadLocal LRU 缓存避免重复计算噪声。
     * 算法：
     * 1. 先应用 TemperatureModifier（None 或 Frozen）
     * 2. 如果 Y > seaLevel + 17，应用高度降温
     *    降温公式: temperature - (noiseValue * 8.0 + y - seaLevel - 17) * 0.00125
     *    其中 noiseValue = TEMPERATURE_NOISE.getValue(x / 8.0, z / 8.0, false)
     *
     * MC 1.21.11: Biome.getTemperature(BlockPos, int seaLevel)
     * 使用 ThreadLocal<Long2FloatLinkedOpenHashMap> 缓存（容量 1024，不 rehash）
     *
     * @param x 方块 X 坐标（世界坐标）
     * @param y 方块 Y 坐标
     * @param z 方块 Z 坐标（世界坐标）
     * @param seaLevel 海平面高度
     * @return 高度调整后的温度
     */
    [[nodiscard]] f32 getTemperature(i32 x, i32 y, i32 z, i32 seaLevel) const;

    /**
     * @brief 获取指定位置的高度调整温度（无缓存）
     *
     * 直接计算，不使用缓存。用于测试和特殊场景。
     *
     * 算法：
     * 1. 先应用 TemperatureModifier（None 或 Frozen）
     * 2. 如果 Y > seaLevel + 17，应用高度降温
     *    降温公式: temperature - (noiseValue * 8.0 + y - seaLevel - 17) * 0.05 / 40.0
     *    其中 noiseValue = TEMPERATURE_NOISE.getValue(x / 8.0, z / 8.0, false)
     *
     * @param x 方块 X 坐标（世界坐标）
     * @param y 方块 Y 坐标
     * @param z 方块 Z 坐标（世界坐标）
     * @param seaLevel 海平面高度
     * @return 高度调整后的温度
     */
    [[nodiscard]] f32 getHeightAdjustedTemperature(i32 x, i32 y, i32 z, i32 seaLevel) const;

    /**
     * @brief 获取基础温度（不含高度调整）
     *
     * 返回 BiomeClimate 中的原始温度值，已经过 TemperatureModifier 处理。
     */
    [[nodiscard]] f32 getBaseTemperature() const;

    /**
     * @brief 判断是否应该降雪
     *
     * @param x 方块 X 坐标
     * @param y 方块 Y 坐标
     * @param z 方块 Z 坐标
     * @param seaLevel 海平面高度
     * @return 是否应该降雪
     */
    [[nodiscard]] bool doesSnowGenerate(i32 x, i32 y, i32 z, i32 seaLevel) const
    {
        return getTemperature(x, y, z, seaLevel) < SNOW_TEMPERATURE_THRESHOLD;
    }

    /**
     * @brief 判断冻洋冰山是否应该轻微融化
     *
     * MC 1.21.11: Biome.shouldMeltFrozenOceanIcebergSlightly()
     * 当位置温度 > 0.1 时返回 true，冰山高度会降低 2.0
     * 与 doesSnowGenerate（阈值 0.15）不同，此方法使用更低的温度阈值
     *
     * @param x 方块 X 坐标
     * @param y 方块 Y 坐标（MC 使用 seaLevel）
     * @param z 方块 Z 坐标
     * @param seaLevel 海平面高度
     * @return 温度是否 > 0.1（冰山应轻微融化）
     */
    [[nodiscard]] bool shouldMeltFrozenOceanIcebergSlightly(i32 x, i32 y, i32 z, i32 seaLevel) const
    {
        return getTemperature(x, y, z, seaLevel) > ICEBERG_MELT_TEMPERATURE_THRESHOLD;
    }

    /**
     * @brief 判断水是否应该结冰（仅温度判断）
     *
     * @param x 方块 X 坐标
     * @param y 方块 Y 坐标
     * @param z 方块 Z 坐标
     * @param seaLevel 海平面高度
     * @return 是否应该结冰
     */
    [[nodiscard]] bool doesWaterFreeze(i32 x, i32 y, i32 z, i32 seaLevel) const
    {
        return getTemperature(x, y, z, seaLevel) < FREEZE_TEMPERATURE_THRESHOLD;
    }

    /**
     * @brief 判断该生物群系在指定位置是否足够温暖可以下雨
     *
     * 当高度调整后的温度 >= 0.15 时返回 true。
     *
     * @param x 方块 X 坐标
     * @param y 方块 Y 坐标
     * @param z 方块 Z 坐标
     * @param seaLevel 海平面高度
     * @return 是否足够温暖可以下雨
     */
    [[nodiscard]] bool warmEnoughToRain(i32 x, i32 y, i32 z, i32 seaLevel) const
    {
        return getHeightAdjustedTemperature(x, y, z, seaLevel) >= SNOW_TEMPERATURE_THRESHOLD;
    }

    /**
     * @brief 判断该生物群系在指定位置是否足够冷以降雪
     *
     * 当高度调整后的温度 < 0.15 时返回 true（即不够温暖到无法下雨）。
     *
     * @param x 方块 X 坐标
     * @param y 方块 Y 坐标
     * @param z 方块 Z 坐标
     * @param seaLevel 海平面高度
     * @return 是否足够冷以降雪
     */
    [[nodiscard]] bool coldEnoughToSnow(i32 x, i32 y, i32 z, i32 seaLevel) const
    {
        return !warmEnoughToRain(x, y, z, seaLevel);
    }

    /**
     * @brief 判断该生物群系是否有降水
     *
     * MC 1.21.11: Biome.hasPrecipitation()
     * 返回 ClimateSettings 中的 hasPrecipitation 布尔值。
     */
    [[nodiscard]] bool hasPrecipitation() const { return m_climate.hasPrecipitation; }

    /**
     * @brief 获取指定位置的降水类型
     *
     * 根据生物群系的降水设置和高度调整后的温度确定降水类型。
     * 如果生物群系没有降水（hasPrecipitation == false），返回 None。
     * 如果高度调整后的温度 < 0.15，返回 Snow；否则返回 Rain。
     *
     * MC 1.21.11: Biome.getPrecipitationAt(BlockPos, int seaLevel)
     *
     * @param x 方块 X 坐标
     * @param y 方块 Y 坐标
     * @param z 方块 Z 坐标
     * @param seaLevel 海平面高度
     * @return 降水类型（None / Rain / Snow）
     */
    [[nodiscard]] BiomeClimate::Precipitation getPrecipitationAt(i32 x, i32 y, i32 z, i32 seaLevel) const
    {
        if (!m_climate.hasPrecipitation) {
            return BiomeClimate::Precipitation::None;
        }
        return coldEnoughToSnow(x, y, z, seaLevel) ? BiomeClimate::Precipitation::Snow
                                                   : BiomeClimate::Precipitation::Rain;
    }

    /**
     * @brief 判断指定位置的水是否应该结冰
     *
     * 完整实现 MC 的 Biome.shouldFreeze 逻辑：
     * 1. 温度检查：如果足够温暖可以下雨，则不冻结
     * 2. 高度检查：位置必须在建造高度范围内
     * 3. 光照检查：方块光照必须 < 10
     * 4. 流体检查：该位置的流体必须是水，且方块必须是 LiquidBlock
     * 5. 邻居检查（可选）：如果 checkNeighbors 为 true，
     *    则四个水平邻居不全是水时才冻结（防止深海中心大面积结冰）
     *
     * LakeFeature 已集成（checkNeighbors=false）。
     * SnowAndFreezeFeature 已集成（TopLayerModification 生成阶段，checkNeighbors=false）。
     * ServerWorld::tickPrecipitation() 已集成（运行时逐 tick 冻结水面，checkNeighbors=true）。
     *
     * @param world 世界接口（用于查询方块状态、流体状态、光照等）
     * @param x 方块 X 坐标
     * @param y 方块 Y 坐标
     * @param z 方块 Z 坐标
     * @param seaLevel 海平面高度
     * @param checkNeighbors 是否检查邻居水域暴露（默认 true）
     * @return 水是否应该结冰
     */
    [[nodiscard]] bool shouldFreeze(
        const IWorld& world, i32 x, i32 y, i32 z, i32 seaLevel, bool checkNeighbors = true) const;

    /**
     * @brief 判断指定位置是否应该降雪
     *
     * 完整实现 MC 的 Biome.shouldSnow 逻辑：
     * 1. 降水类型检查：生物群系在此位置的降水类型必须是雪
     * 2. 高度检查：位置必须在建造高度范围内
     * 3. 光照检查：方块光照必须 < 10
     * 4. 方块检查：该位置的方块必须是空气或已有雪层，且雪层方块能在此处存活
     *
     * SnowAndFreezeFeature 已集成（TopLayerModification 生成阶段）。
     * ServerWorld::tickPrecipitation() 已集成（运行时逐 tick 放置雪层）。
     *
     * @param world 世界接口（用于查询方块状态、光照等）
     * @param x 方块 X 坐标
     * @param y 方块 Y 坐标
     * @param z 方块 Z 坐标
     * @param seaLevel 海平面高度
     * @return 是否应该降雪
     */
    [[nodiscard]] bool shouldSnow(const IWorld& world, i32 x, i32 y, i32 z, i32 seaLevel) const;

    // === 方块设置 ===
    [[nodiscard]] const BlockState* surfaceBlock() const { return m_surfaceBlock; }
    [[nodiscard]] const BlockState* subSurfaceBlock() const { return m_subSurfaceBlock; }
    [[nodiscard]] const BlockState* underWaterBlock() const { return m_underWaterBlock; }
    [[nodiscard]] const BlockState* bedrockBlock() const { return m_bedrockBlock; }

    // === 设置器 ===
    void setDepth(f32 value) { m_depth = value; }
    void setScale(f32 value) { m_scale = value; }
    void setClimate(const BiomeClimate& climate) { m_climate = climate; }
    void setTemperature(f32 value) { m_climate.temperature = value; }
    void setHumidity(f32 value) { m_climate.humidity = value; }
    void setContinentalness(f32 value) { m_climate.continentalness = value; }
    void setErosion(f32 value) { m_climate.erosion = value; }
    void setDownfall(f32 value) { m_climate.downfall = value; }
    void setHasPrecipitation(bool value) { m_climate.hasPrecipitation = value; }
    void setTemperatureModifier(BiomeClimate::TemperatureModifier value) { m_climate.temperatureModifier = value; }

    /**
     * @brief 是否为"火焰加速熄灭"群系（vanilla EnvironmentAttributes.INCREASED_FIRE_BURNOUT）
     *
     * 对齐 vanilla 1.21.11 FireBlock.checkBurnOut（FireBlock.java:178-179）：在潮湿/特殊群系
     * 中火焰蔓延时基础概率减 50、远距离点燃几率折半。由 BiomeLoader 解析群系 JSON 的
     * attributes["minecraft:gameplay/increased_fire_burnout"] 注入（数据驱动，vanilla 共 8 个
     * 群系置 true：swamp/mangrove_swamp/jungle/bamboo_jungle/mushroom_fields/frozen_peaks/
     * jagged_peaks/snowy_slopes）。
     *
     * TODO: 完整 EnvironmentAttributes 系统（EnvironmentAttribute/Map/System/Timeline/Codec/
     * 网络同步）实现后，本标志应迁移到 world.environmentAttributes().getValue(
     * EnvironmentAttributes::INCREASED_FIRE_BURNOUT, pos)，支持维度级 override 与空间插值。
     * 群系层标志届时作为该环境属性的数据源之一。
     *
     * @return 是否为火焰加速熄灭群系
     */
    [[nodiscard]] bool isIncreasedFireBurnout() const noexcept { return m_increasedFireBurnout; }
    void setIncreasedFireBurnout(bool value) noexcept { m_increasedFireBurnout = value; }
    void setSurfaceBlock(const BlockState* block) { m_surfaceBlock = block; }
    void setSubSurfaceBlock(const BlockState* block) { m_subSurfaceBlock = block; }
    void setUnderWaterBlock(const BlockState* block) { m_underWaterBlock = block; }
    void setBedrockBlock(const BlockState* block) { m_bedrockBlock = block; }

    // === 生成设置 ===
    [[nodiscard]] const BiomeGenerationSettings& generationSettings() const { return m_generationSettings; }
    [[nodiscard]] BiomeGenerationSettings& generationSettings() { return m_generationSettings; }
    void setGenerationSettings(BiomeGenerationSettings settings) { m_generationSettings = std::move(settings); }

    // === 生物生成设置 ===
    [[nodiscard]] const MobSpawnInfo& spawnInfo() const { return m_spawnInfo; }
    [[nodiscard]] MobSpawnInfo& spawnInfo() { return m_spawnInfo; }
    void setSpawnInfo(const MobSpawnInfo& info) { m_spawnInfo = info; }
    void setSpawnInfo(MobSpawnInfo&& info) { m_spawnInfo = std::move(info); }

    /**
     * @brief 获取动物生成概率
     *
     * 返回每次尝试生成动物的基础概率。直接代理到 MobSpawnInfo 的
     * creatureSpawnProbability（唯一数据来源），与 WorldGenSpawner / NaturalSpawner
     * 读取同一字段。
     *
     * @return 生成概率 (0.0 - 1.0)
     */
    [[nodiscard]] f32 creatureSpawnProbability() const { return m_spawnInfo.getCreatureSpawnProbability(); }

    // === 视觉效果 ===

    /**
     * @brief 获取生物群系视觉效果
     * @return BiomeEffects 引用
     */
    [[nodiscard]] const BiomeEffects& effects() const { return m_effects; }
    [[nodiscard]] BiomeEffects& effects() { return m_effects; }

    /**
     * @brief 设置生物群系视觉效果
     * @param effects 视觉效果配置
     */
    void setEffects(const BiomeEffects& effects) { m_effects = effects; }

    // === 环境音效 ===

    /**
     * @brief 获取环境音效配置
     * @return BiomeAmbientSounds 常量引用
     */
    [[nodiscard]] const BiomeAmbientSounds& ambientSounds() const { return m_ambientSounds; }
    [[nodiscard]] BiomeAmbientSounds& ambientSounds() { return m_ambientSounds; }

    /**
     * @brief 设置环境音效配置
     * @param sounds 环境音效配置
     */
    void setAmbientSounds(const BiomeAmbientSounds& sounds) { m_ambientSounds = sounds; }

    // === 温度缓存 ===

    /// MC 1.21.11: Biome.TEMPERATURE_CACHE_SIZE = 1024
    static constexpr i32 TEMPERATURE_CACHE_SIZE = 1024;

    /**
     * @brief 获取当前线程的温度缓存
     *
     * MC 1.21.11 使用 ThreadLocal<Long2FloatLinkedOpenHashMap>，
     * 每个线程有独立的缓存实例，无需加锁。
     */
    [[nodiscard]] static Long2FloatLRUCache& getTemperatureCache();

    /**
     * @brief 清除温度缓存
     *
     * 用于世界卸载或种子变更时重置缓存。
     */
    static void clearTemperatureCache();

private:
    BiomeId m_id = 0;
    std::string m_name;

    // 地形参数
    f32 m_depth = 0.0f; ///< 深度/基础高度
    f32 m_scale = 0.0f; ///< 高度变化比例

    // 气候参数
    BiomeClimate m_climate;

    // 环境属性：火焰加速熄灭标志（vanilla EnvironmentAttributes.INCREASED_FIRE_BURNOUT）
    bool m_increasedFireBurnout = false;

    // 方块设置 - 使用BlockState指针，运行时从VanillaBlocks获取
    const BlockState* m_surfaceBlock = nullptr;
    const BlockState* m_subSurfaceBlock = nullptr;
    const BlockState* m_underWaterBlock = nullptr;
    const BlockState* m_bedrockBlock = nullptr;

    // 生成设置
    BiomeGenerationSettings m_generationSettings;

    // 生物生成设置
    MobSpawnInfo m_spawnInfo;

    // 视觉效果
    BiomeEffects m_effects; ///< 生物群系视觉效果（水体颜色、雾颜色等）

    // 环境音效
    BiomeAmbientSounds m_ambientSounds; ///< 生物群系环境音效配置
};

} // namespace biome
} // namespace world
} // namespace mc

// 旧命名空间兼容别名
namespace mc {
using Biome = ::mc::world::biome::Biome;
} // namespace mc
