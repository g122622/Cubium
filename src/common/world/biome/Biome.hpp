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
#include "common/world/spawn/MobSpawnInfo.hpp"
#include <string>
#include <string_view>
#include <utility>

// 前向声明（必须在 mc::world::biome 命名空间之外，避免命名空间污染）
namespace mc {
class BlockState;
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
     * @brief 获取指定位置的高度调整温度
     *
     * 算法：
     * 1. 先应用 TemperatureModifier（None 或 Frozen）
     * 2. 如果 Y > seaLevel + 17，应用高度降温
     *    降温公式: temperature - (noiseValue * 8.0 + y - seaLevel - 17) * 0.00125
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
        return getHeightAdjustedTemperature(x, y, z, seaLevel) < SNOW_TEMPERATURE_THRESHOLD;
    }

    /**
     * @brief 判断水是否应该结冰
     *
     * @param x 方块 X 坐标
     * @param y 方块 Y 坐标
     * @param z 方块 Z 坐标
     * @param seaLevel 海平面高度
     * @return 是否应该结冰
     */
    [[nodiscard]] bool doesWaterFreeze(i32 x, i32 y, i32 z, i32 seaLevel) const
    {
        return getHeightAdjustedTemperature(x, y, z, seaLevel) < FREEZE_TEMPERATURE_THRESHOLD;
    }

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
     * 返回每次尝试生成动物的基础概率。
     * 默认值为 10.0f / 128.0f ≈ 0.078
     *
     * @return 生成概率 (0.0 - 1.0)
     */
    [[nodiscard]] f32 creatureSpawnProbability() const { return m_creatureSpawnProbability; }
    void setCreatureSpawnProbability(f32 prob) { m_creatureSpawnProbability = prob; }

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

private:
    BiomeId m_id = 0;
    std::string m_name;

    // 地形参数
    f32 m_depth = 0.0f; ///< 深度/基础高度
    f32 m_scale = 0.0f; ///< 高度变化比例

    // 气候参数
    BiomeClimate m_climate;

    // 方块设置 - 使用BlockState指针，运行时从VanillaBlocks获取
    const BlockState* m_surfaceBlock = nullptr;
    const BlockState* m_subSurfaceBlock = nullptr;
    const BlockState* m_underWaterBlock = nullptr;
    const BlockState* m_bedrockBlock = nullptr;

    // 生成设置
    BiomeGenerationSettings m_generationSettings;

    // 生物生成设置
    MobSpawnInfo m_spawnInfo;
    f32 m_creatureSpawnProbability = 10.0f / 128.0f; ///< 动物生成概率，默认 ~7.8%

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
