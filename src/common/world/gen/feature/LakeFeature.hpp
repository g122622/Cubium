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

#include "common/core/Types.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include <memory>
#include <string>

// Forward declarations
namespace mc {
class Block;
class BlockState;
class IWorldWriter;
} // namespace mc

namespace mc::world::gen::feature::lake {

/**
 * @brief 湖泊特征配置
 *
 * 参考 MC 1.21.11: LakeFeature.Configuration
 * 配置湖泊或熔岩湖的流体和边界方块。
 */
struct LakeFeatureConfig {
    const Block* fluidBlock;       ///< 流体方块（用于类型比较）
    const Block* borderBlock;      ///< 边界方块（用于类型比较，MC 1.21 中可为 AIR 表示无边界）
    const BlockState* fluidState;  ///< 流体方块状态
    const BlockState* borderState; ///< 边界方块状态

    LakeFeatureConfig(const Block* fluid = nullptr, const Block* border = nullptr)
        : fluidBlock(fluid)
        , borderBlock(border)
        , fluidState(fluid ? &fluid->defaultState() : nullptr)
        , borderState(border ? &border->defaultState() : nullptr)
    {}

    LakeFeatureConfig(const LakeFeatureConfig&) = default;
    LakeFeatureConfig(LakeFeatureConfig&&) noexcept = default;
    LakeFeatureConfig& operator=(const LakeFeatureConfig&) = default;
    LakeFeatureConfig& operator=(LakeFeatureConfig&&) noexcept = default;
};

/**
 * @brief 湖泊特征
 *
 * 使用 MC 1.21.11 的 16x8x16 布尔数组雕刻算法生成湖泊。
 * 算法通过生成 4~7 个随机椭球体来创建不规则形状，
 * 然后验证边界并放置流体和边界方块。
 *
 * 参考 MC 1.21.11: LakeFeature.place()
 */
class LakeFeature {
public:
    /**
     * @brief 构造函数
     * @param config 配置
     */
    explicit LakeFeature(const LakeFeatureConfig& config);

    /**
     * @brief 生成湖泊
     * @param world 世界区域
     * @param rng 随机数生成器
     * @param x 中心 X 坐标
     * @param y 中心 Y 坐标
     * @param z 中心 Z 坐标
     * @return 是否成功生成
     */
    bool place(WorldGenRegion& world, math::Random& rng, i32 x, i32 y, i32 z);

    /**
     * @brief 创建水湖配置
     */
    static LakeFeatureConfig createWaterLake();

    /**
     * @brief 创建熔岩湖配置
     */
    static LakeFeatureConfig createLavaLake();

private:
    /**
     * @brief 检查方块是否可被湖泊替换
     *
     * 参考 MC 1.21.11: LakeFeature.canReplaceBlock()
     * 使用 FEATURES_CANNOT_REPLACE 标签（如存在）或回退到简单检查。
     */
    [[nodiscard]] static bool canReplaceBlock(const BlockState& state);

    LakeFeatureConfig m_config;
};

/**
 * @brief 创建水湖特征
 */
std::unique_ptr<LakeFeature> createWaterLakeFeature();

/**
 * @brief 创建熔岩湖特征
 */
std::unique_ptr<LakeFeature> createLavaLakeFeature();

} // namespace mc::world::gen::feature::lake

namespace mc {

/**
 * @brief 配置化湖泊特征
 *
 * 将 LakeFeature 适配到 ConfiguredFeatureBase 流水线，
 * 用于在 Lakes 阶段统一注册和触发。
 */
class ConfiguredLakeFeature : public ConfiguredFeatureBase {
public:
    /**
     * @brief 构造配置化湖泊特征
     * @param config 湖泊配置
     * @param featureName 特征名称
     * @param chance 触发概率分母（1/chance）
     * @param minY 采样最小高度（含）
     * @param maxY 采样最大高度（含）
     */
    ConfiguredLakeFeature(
        world::gen::feature::lake::LakeFeatureConfig config, const char* featureName, i32 chance, i32 minY, i32 maxY);

    /**
     * @brief 在区块中尝试放置湖泊
     *
     * @note 本方法会自行做概率门控和高度采样。
     */
    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::Lakes; }

private:
    mutable world::gen::feature::lake::LakeFeature m_feature;
    std::string m_name;
    i32 m_chance;
    i32 m_minY;
    i32 m_maxY;
    bool m_isLava;
};

} // namespace mc
