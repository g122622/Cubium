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

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/world/biome/Biomes.hpp"
#include "common/world/block/BlockState.hpp"
#include <functional>
#include <limits>
#include <unordered_map>
#include <vector>

namespace mc::world::gen::density {
class NoiseChunk;
}

namespace mc::world::gen {
class RandomState;
}

namespace mc::world::gen::noise {
class NormalNoise;
}

// 前向声明：缓存的 key/value 类型。完整定义在 SurfaceCondition.hpp 中。
// SurfaceRuleContext 与 SurfaceCondition 互相引用，这里用前向声明打破循环依赖，
// 实现放各自的 .cpp（可 include 对方头文件）。
// 这些前向声明位于 mc::world::gen::surface 命名空间内（见下方 namespace 块）。

namespace mc::world::gen::surface {

class SurfaceCondition;
class LazyXZCondition;
class LazyYCondition;

/**
 * @brief SurfaceRules 上下文（MC 1.21 SurfaceRules.Context）
 *
 * 维护当前位置的状态（stoneDepth、waterHeight、biome等），
 * 在 SurfaceSystem 遍历区块方块时逐个更新。
 * 条件和规则通过 const 引用访问上下文来判断和返回结果。
 */
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
     * @param clayBands 恶地陶土带（维度级产物，所有权在 SurfaceSystem，本类只借用）
     * @param noiseChunk NoiseChunk 引用，用于查询 preliminarySurfaceLevel
     * @param positionalRandom 位置随机工厂（MC: noiseRandom，用于 getSurfaceDepth 抖动）
     * @param randomState RandomState 引用，用于噪声名称查找和随机工厂查找
     * @param heightProvider 高度查询回调（用于 steep 条件）
     */
    SurfaceRuleContext(i32 seaLevel,
        i32 minY,
        i32 height,
        const world::gen::noise::NormalNoise* surfaceDepthNoise,
        const world::gen::noise::NormalNoise* surfaceSecondaryNoise,
        const world::gen::noise::NormalNoise* clayBandsOffsetNoise,
        const std::vector<const BlockState*>& clayBands,
        const density::NoiseChunk& noiseChunk,
        const math::PositionalRandomFactory& positionalRandom,
        world::gen::RandomState* randomState,
        HeightProvider heightProvider = nullptr);

    /** 更新 XZ 坐标（每列开始时调用） */
    void updateXZ(i32 blockX, i32 blockZ);

    /** 更新 Y 相关状态（每个方块调用） */
    void updateY(i32 stoneDepthAbove, i32 stoneDepthBelow, i32 waterHeight, i32 blockX, i32 blockY, i32 blockZ);

    // ========== 条件缓存（MC 1.21 SurfaceRules.LazyCondition） ==========
    // MC 1.21: XZ-only 条件（NoiseThresholdCondition/Hole/Steep）每列只求值一次，
    // Y 依赖条件每 Y 步只求值一次。原版将缓存放在 per-call 的 LazyCondition 实例中；
    // 本项目规则树跨线程共享，故缓存放在 per-call 的 SurfaceRuleContext 里，
    // 以 condition 对象的 this 指针为 key。updateXZ 使两者均失效（列变了 Y 也变），
    // updateY 仅使 Y 缓存失效（XZ 缓存跨 Y 步复用）。

    /** 查询/求值 XZ-only 条件：当前列内命中缓存则直接返回，否则调 cond.compute 并缓存。 */
    [[nodiscard]] bool cachedXZ(const SurfaceCondition* self, const LazyXZCondition& cond) const;
    /** 查询/求值 Y 依赖条件：当前 Y 步内命中缓存则直接返回，否则调 cond.compute 并缓存。 */
    [[nodiscard]] bool cachedY(const SurfaceCondition* self, const LazyYCondition& cond) const;

    /**
     * @brief 按 SurfaceCondition 身份解析并缓存随机工厂
     *
     * 等价于原版 SurfaceRules.ConditionSource.apply() —— 原版在**每区块**调用 apply()
     * 时解析一次工厂并捕获进闭包，compute() 内不再解析。本项目规则树在构造期一次性
     * 建好并跨 RandomState/线程共享，没有 apply() 这个每区块时机，故把等价缓存放在
     * per-chunk 的 SurfaceRuleContext 上（以 condition 身份为 key 惰性解析一次）。
     *
     * 【不变量】缓存的是 const 指针，指向 RandomState 持有的工厂。SurfaceRuleContext 由
     * buildSurface 在栈上创建，其生命周期严格嵌套在 RandomState 之内，故该指针不可能
     * 跨 RandomState 悬垂。这与 gen/README「容易踩的坑」第 14 条禁止在**共享规则树节点**上
     * 缓存裸指针并不冲突（此处缓存位于 per-chunk 对象上）。
     *
     * @param self 条件对象身份（this 指针），作为缓存 key
     * @param name 条件持有的随机工厂名（如 "minecraft:bedrock_floor"）
     * @return 该条件对应随机工厂的引用
     */
    [[nodiscard]] const math::PositionalRandomFactory& resolvedRandomFactory(
        const SurfaceCondition* self, const std::string& name) const;

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

    /** 判断温度是否足够冷以降雪 */
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
    /// NoiseThresholdCondition 在 compute() 经此现解析 NormalNoise；
    /// VerticalGradientCondition 的随机工厂则经 resolvedRandomFactory 在本 ctx 上缓存
    /// （缓存仅存在于 per-chunk 的 ctx 内，不会随 RandomState 销毁而悬垂）。
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

    // 条件缓存脏标记计数器（MC 1.21: SurfaceRules.Context.lastUpdateXZ/lastUpdateY）
    // updateXZ 时两者均自增（列变 → Y 缓存失效）；updateY 时仅 m_updateCounterY 自增。
    u64 m_updateCounterXZ = 0;
    u64 m_updateCounterY = 0;

    // per-call 条件缓存：condition 身份(this 指针) → {stamp, value}
    // stamp 与 m_updateCounterXZ 或 m_updateCounterY 比对，命中则复用 value。
    // SurfaceRuleContext 每次 buildSurface 新建，单线程独占，无需同步。
    struct ConditionCacheEntry {
        u64 stamp;
        bool value;
    };
    mutable std::unordered_map<const SurfaceCondition*, ConditionCacheEntry> m_conditionCache;

    // per-chunk 随机工厂解析缓存：condition 身份 → RandomState 持有的工厂指针。
    // 生命周期安全（见 resolvedRandomFactory 文档）。ctx 单线程独占，无需同步。
    mutable std::unordered_map<const SurfaceCondition*, const math::PositionalRandomFactory*> m_resolvedFactoryCache;

    // 缓存
    mutable bool m_surfaceSecondaryCached = false;
    mutable f64 m_surfaceSecondaryValue = 0.0;
    mutable i64 m_lastPreliminarySurfaceCellOrigin = std::numeric_limits<i64>::min();
    mutable i64 m_lastMinSurfaceLevelXZ = std::numeric_limits<i64>::min();
    mutable i32 m_preliminarySurfaceCache[4] = {};
    mutable i32 m_minSurfaceLevel = 0;

    // 恶地陶土带（MC: SurfaceSystem.clayBands）。维度级生成一次，所有权在 SurfaceSystem；
    // SurfaceSystem 的生命周期长于本 ctx，故此处只持引用。
    const std::vector<const BlockState*>& m_clayBands;
};

} // namespace mc::world::gen::surface
