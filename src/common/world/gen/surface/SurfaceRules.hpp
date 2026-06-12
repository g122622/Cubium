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
 */

// TODO 这个文件太大，需要拆解成一个类对应一个文件

#pragma once

#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/world/biome/Biomes.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace mc::world::gen::density {
class NoiseChunk;
}

namespace mc::world::gen {
class RandomState;
}

namespace mc::world::chunk {
class ChunkPrimer;
}

namespace mc::world::gen::surface {

// Forward declarations
class SurfaceRuleContext;
using ChunkPrimer = ::mc::world::chunk::ChunkPrimer;

// ============================================================================
// CaveSurface — 地表/洞穴顶部方向枚举
// ============================================================================

enum class CaveSurface : u8 { Floor, Ceiling };

// ============================================================================
// VerticalAnchor — Y 坐标锚点（MC 1.18+）
// ============================================================================

enum class VerticalAnchorType : u8 { Absolute, AboveBottom, BelowTop };

struct VerticalAnchor {
    VerticalAnchorType type;
    i32 value;

    static VerticalAnchor absolute(i32 y) { return {VerticalAnchorType::Absolute, y}; }
    static VerticalAnchor aboveBottom(i32 offset) { return {VerticalAnchorType::AboveBottom, offset}; }
    static VerticalAnchor belowTop(i32 offset) { return {VerticalAnchorType::BelowTop, offset}; }

    /** 将锚点解析为世界 Y 坐标 */
    i32 resolveY(i32 minY, i32 height) const;
};

// ============================================================================
// SurfaceCondition — MC 1.21 SurfaceRules 条件接口
// ============================================================================

class SurfaceCondition {
public:
    virtual ~SurfaceCondition() = default;

    /** 在当前 context 下评估条件 */
    [[nodiscard]] virtual bool test(const SurfaceRuleContext& ctx) const = 0;
};

// ============================================================================
// SurfaceRule — MC 1.21 SurfaceRules 规则接口
// ============================================================================

class SurfaceRule {
public:
    virtual ~SurfaceRule() = default;

    /** 尝试在指定位置应用规则，返回方块状态或 nullptr */
    [[nodiscard]] virtual const BlockState* tryApply(
        i32 blockX, i32 blockY, i32 blockZ, const SurfaceRuleContext& ctx) const = 0;
};

// ============================================================================
// SurfaceRuleContext — MC 1.21 SurfaceRules 上下文
//
// MC 中 ConditionSource.apply(Context) 创建绑定 Context 的 Condition。
// 这里我们简化为直接在 Condition 中持有 const Context 引用。
// ============================================================================

class SurfaceRuleContext {
public:
    /**
     * @brief 高度查询回调（用于 steep 条件计算斜率）
     * MC 1.21: SurfaceRules.SteepCondition 使用相邻列高度差判断陡峭度。
     * 参数: (worldX, worldZ) → 高度值（WorldSurfaceWG 高度图 + 1）
     */
    using HeightProvider = std::function<i32(i32, i32)>;

    /**
     * @brief 构建表面规则上下文
     * @param seaLevel 海平面高度
     * @param minY 世界最低 Y
     * @param height 世界高度
     * @param surfaceDepthNoise 地表深度噪声（MC: Noises.SURFACE）
     * @param surfaceSecondaryNoise 地表次要噪声（MC: Noises.SURFACE_SECONDARY）
     * @param clayBandsOffsetNoise 陶土带偏移噪声
     * @param noiseChunk NoiseChunk 引用，用于查询 preliminarySurfaceLevel
     * @param positionalRandom 位置随机工厂（MC: noiseRandom，用于 getSurfaceDepth 抖动和 clayBands 种子）
     * @param randomState RandomState 引用，用于噪声名称查找和随机工厂查找
     * @param heightProvider 高度查询回调（用于 steep 条件）
     */
    SurfaceRuleContext(i32 seaLevel,
        i32 minY,
        i32 height,
        const world::gen::noise::NormalNoise* surfaceDepthNoise,
        const world::gen::noise::NormalNoise* surfaceSecondaryNoise,
        const world::gen::noise::NormalNoise* clayBandsOffsetNoise,
        const density::NoiseChunk& noiseChunk,
        const math::PositionalRandomFactory& positionalRandom,
        world::gen::RandomState* randomState,
        HeightProvider heightProvider = nullptr);

    /** 更新 XZ 坐标（每列开始时调用） */
    void updateXZ(i32 blockX, i32 blockZ);

    /** 更新 Y 相关状态（每个方块调用） */
    void updateY(i32 stoneDepthAbove, i32 stoneDepthBelow, i32 waterHeight, i32 blockX, i32 blockY, i32 blockZ);

    // ========== 访问器 ==========

    [[nodiscard]] i32 blockX() const { return m_blockX; }
    [[nodiscard]] i32 blockY() const { return m_blockY; }
    [[nodiscard]] i32 blockZ() const { return m_blockZ; }
    [[nodiscard]] i32 stoneDepthAbove() const { return m_stoneDepthAbove; }
    [[nodiscard]] i32 stoneDepthBelow() const { return m_stoneDepthBelow; }
    [[nodiscard]] i32 waterHeight() const { return m_waterHeight; }
    [[nodiscard]] i32 surfaceDepth() const { return m_surfaceDepth; }
    [[nodiscard]] i32 seaLevel() const { return m_seaLevel; }
    [[nodiscard]] i32 minY() const { return m_minY; }
    [[nodiscard]] i32 height() const { return m_height; }
    [[nodiscard]] BiomeId biome() const { return m_biome; }
    [[nodiscard]] world::gen::RandomState* randomState() const { return m_randomState; }

    void setBiome(BiomeId biome) { m_biome = biome; }

    /** 地表次要噪声值（MC: getSurfaceSecondary） */
    [[nodiscard]] f64 surfaceSecondary() const;

    /** 获取 bandlands 方块（MC: SurfaceSystem.getBand） */
    [[nodiscard]] const BlockState* getBand(i32 blockY) const;

    /** 判断位置是否在预备表面之上 */
    [[nodiscard]] bool abovePreliminarySurface() const;

    /** 判断位置是否陡峭 */
    [[nodiscard]] bool steep() const;

    /** 判断温度是否足够冷以降雪
     * MC 1.21: 使用 Climate.Sampler.temperature() 在 quart 坐标采样，
     * 当 temperature < 0.0 时返回 true。
     * 当前简化实现基于生物群系判断。
     */
    [[nodiscard]] bool temperature() const;

    /** 判断是否为 hole（surfaceDepth <= 0） */
    [[nodiscard]] bool hole() const { return m_surfaceDepth <= 0; }

    /** 获取最小表面高度（MC: SurfaceRules.Context.getMinSurfaceLevel） */
    [[nodiscard]] i32 minSurfaceLevel() const { return _minSurfaceLevel(); }

private:
    [[nodiscard]] i32 _minSurfaceLevel() const;

    i32 m_seaLevel;
    i32 m_minY;
    i32 m_height;

    // 噪声（不拥有）
    const world::gen::noise::NormalNoise* m_surfaceDepthNoise;
    const world::gen::noise::NormalNoise* m_surfaceSecondaryNoise;
    const world::gen::noise::NormalNoise* m_clayBandsOffsetNoise;

    /// NoiseChunk 引用，用于查询 preliminarySurfaceLevel（MC 1.21: SurfaceRules.Context.noiseChunk）
    const density::NoiseChunk& m_noiseChunk;

    /// 位置随机工厂（MC: noiseRandom），用于 getSurfaceDepth 抖动等
    const math::PositionalRandomFactory& m_positionalRandom;

    /// RandomState 引用，用于噪声名称查找和随机工厂查找（MC 1.21）
    /// 非const：NoiseThresholdCondition::test() 需要通过 getOrCreateNoise() 填充缓存
    world::gen::RandomState* m_randomState;

    /// 高度查询回调（用于 steep 条件）
    HeightProvider m_heightProvider;

    // 当前位置状态
    i32 m_blockX = 0;
    i32 m_blockZ = 0;
    i32 m_blockY = 0;
    i32 m_stoneDepthAbove = 0;
    i32 m_stoneDepthBelow = 0;
    i32 m_waterHeight = 0;
    i32 m_surfaceDepth = 0;
    BiomeId m_biome = 0;

    // 缓存
    mutable bool m_surfaceSecondaryCached = false;
    mutable f64 m_surfaceSecondaryValue = 0.0;
    mutable i64 m_lastXZ = -1;
    mutable i64 m_lastPreliminarySurfaceCellOrigin = std::numeric_limits<i64>::min();
    mutable i64 m_lastMinSurfaceLevelXZ = std::numeric_limits<i64>::min();
    mutable i32 m_preliminarySurfaceCache[4] = {};
    mutable i32 m_minSurfaceLevel = 0;

    // Bandlands 陶土带
    std::vector<const BlockState*> m_clayBands;
    void generateClayBands(const math::PositionalRandomFactory& random);
};

// ============================================================================
// 条件实现
// ============================================================================

/** 石块深度检查（MC: StoneDepthCheck） */
class StoneDepthCondition final : public SurfaceCondition {
public:
    StoneDepthCondition(i32 offset, bool addSurfaceDepth, i32 secondaryDepthRange, CaveSurface surface)
        : m_offset(offset)
        , m_addSurfaceDepth(addSurfaceDepth)
        , m_secondaryDepthRange(secondaryDepthRange)
        , m_surface(surface)
    {}

    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const override;

private:
    i32 m_offset;
    bool m_addSurfaceDepth;
    i32 m_secondaryDepthRange;
    CaveSurface m_surface;
};

/** Y 高度检查（MC: YCondition） */
class YCondition final : public SurfaceCondition {
public:
    YCondition(VerticalAnchor anchor, i32 surfaceDepthMultiplier, bool addStoneDepth)
        : m_anchor(anchor)
        , m_surfaceDepthMultiplier(surfaceDepthMultiplier)
        , m_addStoneDepth(addStoneDepth)
    {}

    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const override;

private:
    VerticalAnchor m_anchor;
    i32 m_surfaceDepthMultiplier;
    bool m_addStoneDepth;
};

/** 水面检查（MC: WaterCondition） */
class WaterCondition final : public SurfaceCondition {
public:
    WaterCondition(i32 offset, i32 surfaceDepthMultiplier, bool addStoneDepth)
        : m_offset(offset)
        , m_surfaceDepthMultiplier(surfaceDepthMultiplier)
        , m_addStoneDepth(addStoneDepth)
    {}

    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const override;

private:
    i32 m_offset;
    i32 m_surfaceDepthMultiplier;
    bool m_addStoneDepth;
};

/** 生物群系检查 */
class BiomeCondition final : public SurfaceCondition {
public:
    explicit BiomeCondition(std::vector<BiomeId> biomes)
        : m_biomes(std::move(biomes))
    {}

    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const override;

private:
    std::vector<BiomeId> m_biomes;
};

/** NOT 条件 */
class NotCondition final : public SurfaceCondition {
public:
    explicit NotCondition(std::unique_ptr<SurfaceCondition> condition)
        : m_condition(std::move(condition))
    {}

    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const override { return !m_condition->test(ctx); }

private:
    std::unique_ptr<SurfaceCondition> m_condition;
};

/** 噪声阈值条件（MC: NoiseThresholdConditionSource）
 *  MC 1.21: 存储噪声名称，在 test() 时通过 RandomState 查找缓存实例。
 */
class NoiseThresholdCondition final : public SurfaceCondition {
public:
    NoiseThresholdCondition(std::string noiseName, f64 minThreshold, f64 maxThreshold)
        : m_noiseName(std::move(noiseName))
        , m_minThreshold(minThreshold)
        , m_maxThreshold(maxThreshold)
    {}

    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const override;

private:
    std::string m_noiseName;
    f64 m_minThreshold;
    f64 m_maxThreshold;
};

/** 垂直梯度条件（MC: VerticalGradientConditionSource）— 用于基岩层等
 *  MC 1.21: 存储随机工厂名称，通过 RandomState 查找 PositionalRandomFactory。
 */
class VerticalGradientCondition final : public SurfaceCondition {
public:
    VerticalGradientCondition(std::string randomName, VerticalAnchor trueAtAndBelow, VerticalAnchor falseAtAndAbove)
        : m_randomName(std::move(randomName))
        , m_trueAtAndBelow(trueAtAndBelow)
        , m_falseAtAndAbove(falseAtAndAbove)
    {}

    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const override;

private:
    std::string m_randomName;
    VerticalAnchor m_trueAtAndBelow;
    VerticalAnchor m_falseAtAndAbove;
};

/** 陡峭条件 */
class SteepCondition final : public SurfaceCondition {
public:
    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const override;
};

/** 温度条件（是否冷到可以降雪） */
class TemperatureCondition final : public SurfaceCondition {
public:
    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const override;
};

/** Hole 条件（surfaceDepth <= 0） */
class HoleCondition final : public SurfaceCondition {
public:
    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const override;
};

/** 预备表面以上条件 */
class AbovePreliminarySurfaceCondition final : public SurfaceCondition {
public:
    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const override;
};

// ============================================================================
// 规则实现
// ============================================================================

/** 条件规则：if 条件为真则应用 */
class IfTrueRule final : public SurfaceRule {
public:
    IfTrueRule(std::unique_ptr<SurfaceCondition> condition, std::unique_ptr<SurfaceRule> thenRule)
        : m_condition(std::move(condition))
        , m_thenRule(std::move(thenRule))
    {}

    [[nodiscard]] const BlockState* tryApply(
        i32 blockX, i32 blockY, i32 blockZ, const SurfaceRuleContext& ctx) const override
    {
        if (m_condition->test(ctx)) {
            return m_thenRule->tryApply(blockX, blockY, blockZ, ctx);
        }
        return nullptr;
    }

private:
    std::unique_ptr<SurfaceCondition> m_condition;
    std::unique_ptr<SurfaceRule> m_thenRule;
};

/** 序列规则：按顺序尝试规则，返回第一个非空结果 */
class SequenceRule final : public SurfaceRule {
public:
    explicit SequenceRule(std::vector<std::unique_ptr<SurfaceRule>> rules)
        : m_rules(std::move(rules))
    {}

    [[nodiscard]] const BlockState* tryApply(
        i32 blockX, i32 blockY, i32 blockZ, const SurfaceRuleContext& ctx) const override
    {
        for (const auto& rule : m_rules) {
            const BlockState* result = rule->tryApply(blockX, blockY, blockZ, ctx);
            if (result != nullptr) {
                return result;
            }
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<SurfaceRule>> m_rules;
};

/** 方块规则：返回固定方块状态 */
class BlockRule final : public SurfaceRule {
public:
    explicit BlockRule(const BlockState* blockState)
        : m_blockState(blockState)
    {}

    [[nodiscard]] const BlockState* tryApply(i32, i32, i32, const SurfaceRuleContext&) const override
    {
        return m_blockState;
    }

private:
    const BlockState* m_blockState;
};

/** Bandlands 规则（MC: Badlands 陶土带） */
class BandlandsRule final : public SurfaceRule {
public:
    [[nodiscard]] const BlockState* tryApply(
        i32 blockX, i32 blockY, i32 blockZ, const SurfaceRuleContext& ctx) const override;
};

// ============================================================================
// SurfaceRules 工厂命名空间
// ============================================================================

namespace SurfaceRules {

// ========== 常用条件快捷方式 ==========

/** ON_FLOOR: stoneDepthCheck(0, false, Floor) — 在表面 */
[[nodiscard]] std::unique_ptr<SurfaceCondition> onFloor();

/** UNDER_FLOOR: stoneDepthCheck(0, true, Floor) — 地表下方 */
[[nodiscard]] std::unique_ptr<SurfaceCondition> underFloor();

/** DEEP_UNDER_FLOOR: stoneDepthCheck(0, true, 6, Floor) — 地表深下方 */
[[nodiscard]] std::unique_ptr<SurfaceCondition> deepUnderFloor();

/** VERY_DEEP_UNDER_FLOOR: stoneDepthCheck(0, true, 30, Floor) — 地表极深下方 */
[[nodiscard]] std::unique_ptr<SurfaceCondition> veryDeepUnderFloor();

/** ON_CEILING: stoneDepthCheck(0, false, Ceiling) — 在洞穴顶部 */
[[nodiscard]] std::unique_ptr<SurfaceCondition> onCeiling();

/** UNDER_CEILING: stoneDepthCheck(0, true, Ceiling) — 洞穴顶部下方 */
[[nodiscard]] std::unique_ptr<SurfaceCondition> underCeiling();

// ========== 条件工厂 ==========

[[nodiscard]] std::unique_ptr<SurfaceCondition> stoneDepthCheck(
    i32 offset, bool addSurfaceDepth, i32 secondaryDepthRange, CaveSurface surface);

[[nodiscard]] std::unique_ptr<SurfaceCondition> yBlockCheck(VerticalAnchor anchor, i32 surfaceDepthMultiplier);

[[nodiscard]] std::unique_ptr<SurfaceCondition> yStartCheck(VerticalAnchor anchor, i32 surfaceDepthMultiplier);

[[nodiscard]] std::unique_ptr<SurfaceCondition> waterBlockCheck(i32 offset, i32 surfaceDepthMultiplier);

[[nodiscard]] std::unique_ptr<SurfaceCondition> waterStartCheck(i32 offset, i32 surfaceDepthMultiplier);

[[nodiscard]] std::unique_ptr<SurfaceCondition> isBiome(std::vector<BiomeId> biomes);

[[nodiscard]] std::unique_ptr<SurfaceCondition> notCondition(std::unique_ptr<SurfaceCondition> condition);

[[nodiscard]] std::unique_ptr<SurfaceCondition> noiseCondition(
    std::string noiseName, f64 minThreshold, f64 maxThreshold = 1e30);

[[nodiscard]] std::unique_ptr<SurfaceCondition> verticalGradient(
    std::string randomName, VerticalAnchor trueAtAndBelow, VerticalAnchor falseAtAndAbove);

[[nodiscard]] std::unique_ptr<SurfaceCondition> steep();

[[nodiscard]] std::unique_ptr<SurfaceCondition> temperature();

[[nodiscard]] std::unique_ptr<SurfaceCondition> hole();

[[nodiscard]] std::unique_ptr<SurfaceCondition> abovePreliminarySurface();

// ========== 规则工厂 ==========

[[nodiscard]] std::unique_ptr<SurfaceRule> blockState(const BlockState* state);

[[nodiscard]] std::unique_ptr<SurfaceRule> ifTrue(
    std::unique_ptr<SurfaceCondition> condition, std::unique_ptr<SurfaceRule> thenRule);

[[nodiscard]] std::unique_ptr<SurfaceRule> sequence(std::vector<std::unique_ptr<SurfaceRule>> rules);

/** 变参 sequence：直接传入 unique_ptr 规则，避免 initializer_list 复制问题 */
template <typename... Rules>
[[nodiscard]] std::unique_ptr<SurfaceRule> sequence(Rules... rules)
{
    std::vector<std::unique_ptr<SurfaceRule>> v;
    v.reserve(sizeof...(rules));
    (v.push_back(std::move(rules)), ...);
    return sequence(std::move(v));
}

[[nodiscard]] std::unique_ptr<SurfaceRule> bandlands();

// ========== 维度规则树 ==========

/** 创建主世界表面规则（MC 1.21 SurfaceRuleData.overworld()） */
[[nodiscard]] std::unique_ptr<SurfaceRule> overworld();

/** 创建下界表面规则 */
[[nodiscard]] std::unique_ptr<SurfaceRule> nether();

/** 创建末地表面规则 */
[[nodiscard]] std::unique_ptr<SurfaceRule> end();

} // namespace SurfaceRules

// ============================================================================
// SurfaceSystem — MC 1.21 SurfaceRules 执行器
// ============================================================================

class SurfaceSystem {
public:
    /**
     * @brief 构造 SurfaceSystem
     * @param surfaceRule 表面规则树
     * @param defaultBlock 默认方块（石头等）
     * @param defaultFluid 默认流体（水等）
     * @param seaLevel 海平面高度
     * @param minY 世界最低 Y
     * @param height 世界高度
     * @param randomState RandomState 引用，用于噪声查找和随机工厂
     * @param positionalRandom 位置随机工厂（MC: noiseRandom，用于 getSurfaceDepth、clayBands、扩展等）
     */
    SurfaceSystem(std::unique_ptr<SurfaceRule> surfaceRule,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        i32 minY,
        i32 height,
        world::gen::RandomState& randomState,
        const math::PositionalRandomFactory& positionalRandom);

    /**
     * @brief 构建整个区块的表面
     * @param chunk 区块数据
     * @param getBiomeAt 获取指定位置的生物群系 ID
     * @param noiseChunk NoiseChunk 引用，用于 preliminarySurfaceLevel 查询
     */
    void buildSurface(ChunkPrimer& chunk,
        const std::function<BiomeId(i32, i32, i32)>& getBiomeAt,
        const density::NoiseChunk& noiseChunk) const;

private:
    /** 判断方块是否为"石头"（非空气、非流体） */
    bool isStone(const BlockState* state) const;

    /**
     * @brief 风蚀恶地地柱扩展（MC: SurfaceSystem.erodedBadlandsExtension）
     * 在 Eroded Badlands 生物群系中生成高耸的石柱/方山地貌。
     */
    void erodedBadlandsExtension(
        ChunkPrimer& chunk, i32 worldX, i32 worldZ, i32 surfaceY, i32 localX, i32 localZ) const;

    /**
     * @brief 冻洋冰山扩展（MC: SurfaceSystem.frozenOceanExtension）
     * 在 Frozen Ocean / Deep Frozen Ocean 生物群系中生成冰山。
     */
    void frozenOceanExtension(ChunkPrimer& chunk,
        i32 worldX,
        i32 worldZ,
        i32 surfaceY,
        i32 localX,
        i32 localZ,
        i32 minSurfaceLevel,
        bool isColdOcean,
        BiomeId biomeId) const;

    std::unique_ptr<SurfaceRule> m_surfaceRule;
    const BlockState* m_defaultBlock;
    const BlockState* m_defaultFluid;
    i32 m_seaLevel;
    i32 m_minY;
    i32 m_height;

    // MC 1.21: RandomState 引用，用于噪声查找
    world::gen::RandomState& m_randomState;

    // 位置随机工厂（MC: noiseRandom）
    math::PositionalRandomFactory m_positionalRandom;

    // MC 1.21: 噪声生成器（从 RandomState 获取，不拥有）
    const world::gen::noise::NormalNoise* m_surfaceDepthNoise = nullptr;
    const world::gen::noise::NormalNoise* m_surfaceSecondaryNoise = nullptr;
    const world::gen::noise::NormalNoise* m_clayBandsOffsetNoise = nullptr;

    // Badlands 和冰山噪声（从 RandomState 获取，不拥有）
    const world::gen::noise::NormalNoise* m_badlandsPillarNoise = nullptr;
    const world::gen::noise::NormalNoise* m_badlandsPillarRoofNoise = nullptr;
    const world::gen::noise::NormalNoise* m_badlandsSurfaceNoise = nullptr;
    const world::gen::noise::NormalNoise* m_icebergPillarNoise = nullptr;
    const world::gen::noise::NormalNoise* m_icebergPillarRoofNoise = nullptr;
    const world::gen::noise::NormalNoise* m_icebergSurfaceNoise = nullptr;
};

} // namespace mc::world::gen::surface
