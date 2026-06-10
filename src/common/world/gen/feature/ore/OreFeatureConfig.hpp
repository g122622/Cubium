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
 * copies of substantial portions of the Software.
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
#include "common/world/block/Block.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include "common/world/gen/feature/ruletest/RuleTests.hpp"
#include <memory>
#include <vector>

namespace mc {

class BlockState;

/**
 * @brief 矿石目标（目标规则 + 对应矿石方块）
 *
 * 每个目标定义了"匹配哪些方块 → 放置什么矿石"。
 * 例如：石头区域放铁矿，深板岩区域放深层铁矿。
 */
struct OreTarget {
    /// 目标方块规则（哪些方块可被替换）
    std::unique_ptr<world::gen::feature::ruletest::RuleTest> target;

    /// 匹配目标时放置的矿石方块状态
    const BlockState* state = nullptr;

    OreTarget(std::unique_ptr<world::gen::feature::ruletest::RuleTest> targetRule, const BlockState* oreState);
};

/**
 * @brief 矿石特征配置
 *
 * 定义矿石生成的参数。
 * 支持多目标列表：遍历 targets，使用第一个匹配的目标放置对应矿石。
 */
struct OreFeatureConfig : public IFeatureConfig {
    /// 多目标列表
    std::vector<OreTarget> targets;

    /// 矿脉大小（方块数量）
    i32 size;

    /**
     * @brief 空气暴露丢弃概率
     *
     * 当矿石方块相邻有空气时，以此概率跳过放置。
     * 0.0 = 不丢弃（默认），1.0 = 总是丢弃暴露在空气中的矿石。
     */
    f32 discardChanceOnAirExposure = 0.0f;

    /**
     * @brief 构造矿石配置（多目标）
     */
    OreFeatureConfig(std::vector<OreTarget> oreTargets, i32 veinSize, f32 discardChance = 0.0f);

    /**
     * @brief 构造矿石配置（单目标）
     */
    OreFeatureConfig(std::unique_ptr<world::gen::feature::ruletest::RuleTest> targetRule,
        const BlockState* oreState,
        i32 veinSize,
        f32 discardChance = 0.0f);

    /**
     * @brief 创建自然石头目标规则
     */
    static std::unique_ptr<world::gen::feature::ruletest::RuleTest> naturalStone();

    /**
     * @brief 创建深板岩目标规则
     */
    static std::unique_ptr<world::gen::feature::ruletest::RuleTest> deepslateStone();

    /**
     * @brief 创建同时匹配石头和深板岩的矿石目标列表
     */
    static std::vector<OreTarget> stoneAndDeepslateOre(const BlockState* stoneOre, const BlockState* deepslateOre);
};

} // namespace mc
