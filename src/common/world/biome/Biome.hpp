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
#include "BiomeEffects.hpp"
#include "BiomeGenerationSettings.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {

// 前向声明
class BlockState;
class BlockPos;

// ============================================================================
// 生物群系气候常量
// ============================================================================

/// 温度变化起始高度（海拔超过此高度时温度开始降低）
inline constexpr i32 TEMPERATURE_HEIGHT_BASE = 64;

/// 降雪温度阈值（温度低于此值时生成雪）
inline constexpr f32 SNOW_TEMPERATURE_THRESHOLD = 0.15f;

/// 结冰温度阈值（温度低于此值时水结冰）
inline constexpr f32 FREEZE_TEMPERATURE_THRESHOLD = 0.15f;

/// 温度高度因子系数（温度随高度降低的系数）
inline constexpr f32 TEMPERATURE_HEIGHT_FACTOR = 0.05f;

/// 温度高度因子除数（温度随高度降低的除数）
inline constexpr f32 TEMPERATURE_HEIGHT_DIVISOR = 30.0f;

// ============================================================================
// 生物群系气候设置
// ============================================================================

/**
 * @brief 生物群系气候设置
 */
struct BiomeClimate {
    enum class Precipitation { None, Rain, Snow };

    /**
     * @brief 温度修改器（MC 1.18+）
     *
     * FROZEN 用于冰冻生物群系的特殊温度处理，
     * 使得在某些情况下温度固定为冰点以下。
     */
    enum class TemperatureModifier { None, Frozen };

    Precipitation precipitation = Precipitation::Rain;
    f32 temperature = 0.5f;
    TemperatureModifier temperatureModifier = TemperatureModifier::None;
    f32 downfall = 0.5f;

    BiomeClimate() = default;
    BiomeClimate(Precipitation precip, f32 temp, TemperatureModifier modifier, f32 down)
        : precipitation(precip)
        , temperature(temp)
        , temperatureModifier(modifier)
        , downfall(down)
    {}

    /** 便捷构造函数，TemperatureModifier 默认为 None */
    BiomeClimate(Precipitation precip, f32 temp, f32 down)
        : precipitation(precip)
        , temperature(temp)
        , temperatureModifier(TemperatureModifier::None)
        , downfall(down)
    {}
};

/**
 * @brief 生物群系定义
 *
 * 存储单个生物群系的生成参数。
 */
class Biome {
public:
    /**
     * @brief 生物群系类别
     */
    enum class Category {
        None,
        Taiga,
        ExtremeHills,
        Jungle,
        Mesa,
        Plains,
        Savanna,
        Icy,
        TheEnd,
        Beach,
        Forest,
        Ocean,
        Desert,
        River,
        Swamp,
        Mushroom,
        Nether
    };

    /**
     * @brief 默认构造函数（用于容器）
     */
    Biome() = default;

    /**
     * @brief 生物群系构造函数
     * @param id 生物群系ID
     * @param name 生物群系名称
     */
    Biome(BiomeId id, const std::string& name) noexcept;

    // === 基本信息 ===
    [[nodiscard]] BiomeId id() const { return m_id; }
    [[nodiscard]] const std::string& name() const { return m_name; }
    [[nodiscard]] Category category() const { return m_category; }

    // === 地形参数 ===
    [[nodiscard]] f32 depth() const { return m_depth; }
    [[nodiscard]] f32 scale() const { return m_scale; }

    // === 气候参数 ===
    [[nodiscard]] const BiomeClimate& climate() const { return m_climate; }
    [[nodiscard]] f32 temperature() const { return m_climate.temperature; }
    [[nodiscard]] f32 humidity() const { return m_humidity; }
    [[nodiscard]] f32 continentalness() const { return m_continentalness; }
    [[nodiscard]] f32 erosion() const { return m_erosion; }

    /**
     * @brief 获取指定位置的温度
     *
     * 海拔不超过基准高度时返回基础温度，超过时温度随海拔降低。
     *
     * @param y Y坐标（高度）
     * @return 位置相关温度
     */
    [[nodiscard]] f32 getTemperature(i32 y) const
    {
        f32 temp = m_climate.temperature;

        // 海拔超过基准高度时降温
        if (y > TEMPERATURE_HEIGHT_BASE) {
            // 温度随高度线性降低
            // TODO: 和原版保持一致需要引入噪声函数，增加温度变化的随机性
            f32 heightFactor =
                static_cast<f32>(y - TEMPERATURE_HEIGHT_BASE) * TEMPERATURE_HEIGHT_FACTOR / TEMPERATURE_HEIGHT_DIVISOR;
            temp = std::max(0.0f, temp - heightFactor);
        }

        return temp;
    }

    /**
     * @brief 获取指定位置的温度（BlockPos版本）
     *
     * @param pos 方块位置
     * @return 位置相关温度
     */
    [[nodiscard]] f32 getTemperature(const BlockPos& pos) const { return getTemperature(pos.y); }

    /**
     * @brief 判断是否应该降雪
     *
     * @param y Y坐标
     * @return 是否应该降雪
     */
    [[nodiscard]] bool doesSnowGenerate(i32 y) const { return getTemperature(y) < SNOW_TEMPERATURE_THRESHOLD; }

    /**
     * @brief 判断水是否应该结冰
     *
     * @param y Y坐标
     * @return 是否应该结冰
     */
    [[nodiscard]] bool doesWaterFreeze(i32 y) const { return getTemperature(y) < FREEZE_TEMPERATURE_THRESHOLD; }

    // === 方块设置 ===
    [[nodiscard]] const BlockState* surfaceBlock() const { return m_surfaceBlock; }
    [[nodiscard]] const BlockState* subSurfaceBlock() const { return m_subSurfaceBlock; }
    [[nodiscard]] const BlockState* underWaterBlock() const { return m_underWaterBlock; }
    [[nodiscard]] const BlockState* bedrockBlock() const { return m_bedrockBlock; }

    // === 设置器 ===
    void setDepth(f32 value) { m_depth = value; }
    void setScale(f32 value) { m_scale = value; }
    void setCategory(Category cat) { m_category = cat; }
    void setClimate(const BiomeClimate& climate) { m_climate = climate; }
    void setTemperature(f32 value) { m_climate.temperature = value; }
    void setHumidity(f32 value) { m_humidity = value; }
    void setContinentalness(f32 value) { m_continentalness = value; }
    void setErosion(f32 value) { m_erosion = value; }
    void setSurfaceBlock(const BlockState* block) { m_surfaceBlock = block; }
    void setSubSurfaceBlock(const BlockState* block) { m_subSurfaceBlock = block; }
    void setUnderWaterBlock(const BlockState* block) { m_underWaterBlock = block; }
    void setBedrockBlock(const BlockState* block) { m_bedrockBlock = block; }

    // === 生成设置 ===
    [[nodiscard]] const BiomeGenerationSettings& generationSettings() const { return m_generationSettings; }
    [[nodiscard]] BiomeGenerationSettings& generationSettings() { return m_generationSettings; }
    void setGenerationSettings(BiomeGenerationSettings settings) { m_generationSettings = std::move(settings); }

    // === 生物生成设置 ===
    [[nodiscard]] const world::spawn::MobSpawnInfo& spawnInfo() const { return m_spawnInfo; }
    [[nodiscard]] world::spawn::MobSpawnInfo& spawnInfo() { return m_spawnInfo; }
    void setSpawnInfo(const world::spawn::MobSpawnInfo& info) { m_spawnInfo = info; }

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
    [[nodiscard]] const world::biome::BiomeEffects& effects() const { return m_effects; }
    [[nodiscard]] world::biome::BiomeEffects& effects() { return m_effects; }

    /**
     * @brief 设置生物群系视觉效果
     * @param effects 视觉效果配置
     */
    void setEffects(const world::biome::BiomeEffects& effects) { m_effects = effects; }

    /**
     * @brief 获取水体颜色
     * @return 水体颜色 (ARGB格式)
     */
    [[nodiscard]] u32 waterColor() const { return m_effects.waterColor(); }

    /**
     * @brief 获取水下雾颜色
     * @return 水下雾颜色 (ARGB格式)
     */
    [[nodiscard]] u32 waterFogColor() const { return m_effects.waterFogColor(); }

    /**
     * @brief 获取雾颜色
     * @return 雾颜色 (ARGB格式)
     */
    [[nodiscard]] u32 fogColor() const { return m_effects.fogColor(); }

    // === 环境音效 ===

    /**
     * @brief 获取环境音效配置
     * @return BiomeAmbientSounds 常量引用
     */
    [[nodiscard]] const world::biome::BiomeAmbientSounds& ambientSounds() const { return m_ambientSounds; }
    [[nodiscard]] world::biome::BiomeAmbientSounds& ambientSounds() { return m_ambientSounds; }

    /**
     * @brief 设置环境音效配置
     * @param sounds 环境音效配置
     */
    void setAmbientSounds(const world::biome::BiomeAmbientSounds& sounds) { m_ambientSounds = sounds; }

    /**
     * @brief 获取生物群系音乐
     *
     * 每个生物群系可以定义专属音乐（如玄武岩三角洲、绯红森林等）。
     * 某些生物群系（如诡异森林）没有音乐。
     *
     * @return 音乐配置，如果没有专属音乐返回空 optional
     */
    [[nodiscard]] const std::optional<world::biome::BiomeMusic>& getMusic() const { return m_ambientSounds.music(); }

private:
    BiomeId m_id = 0;
    std::string m_name;
    Category m_category = Category::None;

    // 地形参数
    f32 m_depth = 0.0f; ///< 深度/基础高度
    f32 m_scale = 0.0f; ///< 高度变化比例

    // 气候参数
    BiomeClimate m_climate;
    f32 m_humidity = 0.5f;
    f32 m_continentalness = 0.0f;
    f32 m_erosion = 0.0f;

    // 方块设置 - 使用BlockState指针，运行时从VanillaBlocks获取
    const BlockState* m_surfaceBlock = nullptr;
    const BlockState* m_subSurfaceBlock = nullptr;
    const BlockState* m_underWaterBlock = nullptr;
    const BlockState* m_bedrockBlock = nullptr;

    // 生成设置
    BiomeGenerationSettings m_generationSettings;

    // 生物生成设置
    world::spawn::MobSpawnInfo m_spawnInfo;
    f32 m_creatureSpawnProbability = 10.0f / 128.0f; ///< 动物生成概率，默认 ~7.8%

    // 视觉效果
    world::biome::BiomeEffects m_effects; ///< 生物群系视觉效果（水体颜色、雾颜色等）

    // 环境音效
    world::biome::BiomeAmbientSounds m_ambientSounds; ///< 生物群系环境音效配置
};

// ============================================================================
// 预定义生物群系ID常量
// 完整列表，ID 与原版完全一致
// 注意：这些 ID 与 BiomeValues.hpp 中的值保持一致
// ============================================================================

namespace Biomes {

// 基础生物群系 (0-13)
constexpr BiomeId Ocean = 0;
constexpr BiomeId Plains = 1;
constexpr BiomeId Desert = 2;
constexpr BiomeId Mountains = 3; // extreme_hills
constexpr BiomeId Forest = 4;
constexpr BiomeId Taiga = 5;
constexpr BiomeId Swamp = 6;
constexpr BiomeId River = 7;
constexpr BiomeId NetherWastes = 8;
constexpr BiomeId TheEnd = 9;
constexpr BiomeId FrozenOcean = 10;
constexpr BiomeId FrozenRiver = 11;
constexpr BiomeId SnowyPlains = 12; // snowy_tundra
constexpr BiomeId SnowyMountains = 13;

// 蘑菇岛 (14-15)
constexpr BiomeId MushroomFields = 14;
constexpr BiomeId MushroomFieldShore = 15;

// 海滩 (16)
constexpr BiomeId Beach = 16;

// 山地变体和丘陵 (17-20)
constexpr BiomeId DesertHills = 17;
constexpr BiomeId WoodedHills = 18; // 也称作 wooded_hills
constexpr BiomeId TaigaHills = 19;
constexpr BiomeId MountainEdge = 20; // 已弃用，但 ID 保留

// 丛林 (21-23)
constexpr BiomeId Jungle = 21;
constexpr BiomeId JungleHills = 22;
constexpr BiomeId JungleEdge = 23;

// 深海和石岸 (24-25)
constexpr BiomeId DeepOcean = 24;
constexpr BiomeId StoneShore = 25;

// 雪地海滩 (26)
constexpr BiomeId SnowyBeach = 26;

// 桦木森林 (27-28)
constexpr BiomeId BirchForest = 27;
constexpr BiomeId BirchForestHills = 28;

// 黑森林 (29)
constexpr BiomeId DarkForest = 29;

// 雪地针叶林 (30-31)
constexpr BiomeId SnowyTaiga = 30;
constexpr BiomeId SnowyTaigaHills = 31;

// 大型针叶林 (32-33)
constexpr BiomeId GiantTreeTaiga = 32;
constexpr BiomeId GiantTreeTaigaHills = 33;

// 热带草原 (34-36)
constexpr BiomeId WoodedMountains = 34; // extreme_hills_with_trees
constexpr BiomeId Savanna = 35;
constexpr BiomeId SavannaPlateau = 36;

// 恶地 (37-39)
constexpr BiomeId Badlands = 37;
constexpr BiomeId WoodedBadlandsPlateau = 38;
constexpr BiomeId BadlandsPlateau = 39;

// 末地生物群系 (40-43)
constexpr BiomeId SmallEndIslands = 40;
constexpr BiomeId EndMidlands = 41;
constexpr BiomeId EndHighlands = 42;
constexpr BiomeId EndBarrens = 43;

// 海洋温度变体 (44-50)
constexpr BiomeId WarmOcean = 44;
constexpr BiomeId LukewarmOcean = 45;
constexpr BiomeId ColdOcean = 46;
constexpr BiomeId DeepWarmOcean = 47;
constexpr BiomeId DeepLukewarmOcean = 48;
constexpr BiomeId DeepColdOcean = 49;
constexpr BiomeId DeepFrozenOcean = 50;

// TheVoid (55)
// 56-127 保留

// 变体生物群系（129-169，稀有变体）
constexpr BiomeId SunflowerPlains = 129;
constexpr BiomeId DesertLakes = 130;
constexpr BiomeId GravellyMountains = 131;
constexpr BiomeId FlowerForest = 132;
constexpr BiomeId TaigaMountains = 133;
constexpr BiomeId SwampHills = 134;
// 135-139 保留
constexpr BiomeId IceSpikes = 140;
// 141-148 保留
constexpr BiomeId ModifiedJungle = 149;
// 150 保留
constexpr BiomeId ModifiedJungleEdge = 151;
// 152-154 保留
constexpr BiomeId TallBirchForest = 155;
constexpr BiomeId TallBirchHills = 156;
constexpr BiomeId DarkForestHills = 157;
constexpr BiomeId SnowyTaigaMountains = 158;
// 159 保留
constexpr BiomeId GiantSpruceTaiga = 160;
constexpr BiomeId GiantSpruceTaigaHills = 161;
constexpr BiomeId ModifiedGravellyMountains = 162;
constexpr BiomeId ShatteredSavanna = 163;
constexpr BiomeId ShatteredSavannaPlateau = 164;
constexpr BiomeId ErodedBadlands = 165;
constexpr BiomeId ModifiedWoodedBadlandsPlateau = 166;
constexpr BiomeId ModifiedBadlandsPlateau = 167;
constexpr BiomeId BambooJungle = 168;
constexpr BiomeId BambooJungleHills = 169;

// 下界生物群系 (170-173)
constexpr BiomeId SoulSandValley = 170;
constexpr BiomeId CrimsonForest = 171;
constexpr BiomeId WarpedForest = 172;
constexpr BiomeId BasaltDeltas = 173;

// MC 1.18+ 新增生物群系 (174-185)
constexpr BiomeId Meadow = 174;
constexpr BiomeId Grove = 175;
constexpr BiomeId SnowySlopes = 176;
constexpr BiomeId JaggedPeaks = 177;
constexpr BiomeId FrozenPeaks = 178;
constexpr BiomeId StonyPeaks = 179;
constexpr BiomeId DripstoneCaves = 180;
constexpr BiomeId LushCaves = 181;
constexpr BiomeId DeepDark = 182;
constexpr BiomeId MangroveSwamp = 183;
constexpr BiomeId CherryGrove = 184;
constexpr BiomeId PaleGarden = 185;

// MC 1.18+ 重命名生物群系（使用新名称，旧ID不变）
constexpr BiomeId WindsweptHills = Mountains;                 // 原 Mountains (3)
constexpr BiomeId WindsweptForest = WoodedMountains;          // 原 WoodedMountains (34)
constexpr BiomeId WindsweptGravellyHills = GravellyMountains; // 原 GravellyMountains (131)
constexpr BiomeId StonyShore = StoneShore;                    // 原 StoneShore (25)
constexpr BiomeId OldGrowthPineTaiga = GiantTreeTaiga;        // 原 GiantTreeTaiga (32)
constexpr BiomeId OldGrowthSpruceTaiga = GiantSpruceTaiga;    // 原 GiantSpruceTaiga (160)
constexpr BiomeId OldGrowthBirchForest = TallBirchForest;     // 原 TallBirchForest (155)
constexpr BiomeId SparseJungle = JungleEdge;                  // 原 JungleEdge (23)
constexpr BiomeId WoodedBadlands = WoodedBadlandsPlateau;     // 原 WoodedBadlandsPlateau (38)
constexpr BiomeId WindsweptSavanna = ShatteredSavanna;        // 原 ShatteredSavanna (163)

// 生物群系总数（最大 ID + 1）
constexpr BiomeId Count = 186;

} // namespace Biomes

} // namespace mc
