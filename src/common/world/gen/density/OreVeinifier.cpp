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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "OreVeinifier.hpp"

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Xoroshiro128ppRandom.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/BaseBlocks.hpp"
#include "common/world/block/registry/CopperBlocks.hpp"
#include "common/world/block/registry/DeepslateBlocks.hpp"
#include "common/world/block/registry/TuffBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include <algorithm>
#include <cstdlib>

namespace mc::world::gen::density {

// ============================================================================
// MC 1.21.11 OreVeinifier 常量
// ============================================================================

/// veinToggle 绝对值 + edgeRoundoff < 此阈值时跳过
static constexpr f64 VEININESS_THRESHOLD = 0.4;

/// 边缘衰减起始距离（距 Y 边界多少格开始衰减）
static constexpr i32 EDGE_ROUNDOFF_BEGIN = 20;

/// 最大边缘衰减值
static constexpr f64 MAX_EDGE_ROUNDOFF = 0.2;

/// 矿脉固体度（nextFloat > 此值时跳过，即约 70% 通过率）
static constexpr f64 VEIN_SOLIDNESS = 0.7;

/// 最低丰富度
static constexpr f64 MIN_RICHNESS = 0.1;

/// 最高丰富度
static constexpr f64 MAX_RICHNESS = 0.3;

/// 丰富度阈值（veinToggle 绝对值从此值开始增加）
static constexpr f64 MAX_RICHNESS_THRESHOLD = 0.6;

/// 原始矿石块概率
static constexpr f64 CHANCE_OF_RAW_ORE_BLOCK = 0.02;

/// gap noise 低于此值时不放置矿石
static constexpr f64 SKIP_ORE_IF_GAP_NOISE_BELOW = -0.3;

// ============================================================================
// VeinType Y 范围
// ============================================================================

/// 铜矿脉 Y 范围: 0 ~ 50
static constexpr i32 COPPER_MIN_Y = 0;
static constexpr i32 COPPER_MAX_Y = 50;

/// 铁矿脉 Y 范围: -60 ~ -8
static constexpr i32 IRON_MIN_Y = -60;
static constexpr i32 IRON_MAX_Y = -8;

// ============================================================================
// 构造函数
// ============================================================================

OreVeinifier::OreVeinifier(const DensityFunction& veinToggle,
    const DensityFunction& veinRidged,
    const DensityFunction& veinGap,
    const math::PositionalRandomFactory& randomFactory)
    : m_veinToggle(veinToggle)
    , m_veinRidged(veinRidged)
    , m_veinGap(veinGap)
    , m_randomFactory(randomFactory)
{}

// ============================================================================
// calculate — MC 1.21.11 OreVeinifier.create() lambda
// ============================================================================

const BlockState* OreVeinifier::calculate(i32 blockX, i32 blockY, i32 blockZ, f64 /*density*/)
{
    ensureInitialized();

    // MC: 计算 veinToggle 噪声值
    const f64 d0 = m_veinToggle.compute(blockX, blockY, blockZ);

    // 确定矿脉类型：正=铜，负=铁
    const bool isCopper = d0 > 0.0;

    // veinToggle 绝对值
    const f64 absToggle = std::abs(d0);

    // 检查 Y 范围约束
    const i32 maxY = isCopper ? COPPER_MAX_Y : IRON_MAX_Y;
    const i32 minY = isCopper ? COPPER_MIN_Y : IRON_MIN_Y;

    const i32 distToTop = maxY - blockY;
    const i32 distToBottom = blockY - minY;

    // Y 超出范围则跳过
    if (distToTop < 0 || distToBottom < 0) {
        return nullptr;
    }

    // 边缘衰减：接近 Y 边界时变薄
    const f64 edgeRoundoff = math::clampedMap(static_cast<f64>(std::min(distToTop, distToBottom)),
        0.0,
        static_cast<f64>(EDGE_ROUNDOFF_BEGIN),
        -MAX_EDGE_ROUNDOFF,
        0.0);

    // veinToggle 太弱则跳过
    if (absToggle + edgeRoundoff < VEININESS_THRESHOLD) {
        return nullptr;
    }

    // veinRidged >= 0 表示不在矿脉脊线内（先检查，避免昂贵的 RNG 创建）
    const f64 veinRidgedValue = m_veinRidged.compute(blockX, blockY, blockZ);
    if (veinRidgedValue >= 0.0) {
        return nullptr;
    }

    // 位置化随机决定是否放置（延迟到密度检查通过后）
    auto rng = m_randomFactory.at(blockX, blockY, blockZ);
    if (rng->nextFloat() > static_cast<f32>(VEIN_SOLIDNESS)) {
        return nullptr; // ~30% 跳过
    }

    // 计算丰富度
    const f64 richness =
        math::clampedMap(absToggle, VEININESS_THRESHOLD, MAX_RICHNESS_THRESHOLD, MIN_RICHNESS, MAX_RICHNESS);

    // gap noise 过滤
    const f64 gapValue = m_veinGap.compute(blockX, blockY, blockZ);

    if (rng->nextFloat() < static_cast<f32>(richness) && gapValue > SKIP_ORE_IF_GAP_NOISE_BELOW) {
        // 放置矿石：2% 概率放原始矿石块，98% 放普通矿石
        if (isCopper) {
            // 铜矿：Y >= 0 使用 copper_ore，Y < 0 使用 deepslate_copper_ore
            if (rng->nextFloat() < static_cast<f32>(CHANCE_OF_RAW_ORE_BLOCK)) {
                return m_rawCopperBlock;
            }
            return (blockY < 0) ? m_deepslateCopperOre : m_copperOre;
        } else {
            // 铁矿：Y >= 0 使用 iron_ore（不在此 Y 范围内，但防御性处理），Y < 0 使用 deepslate_iron_ore
            if (rng->nextFloat() < static_cast<f32>(CHANCE_OF_RAW_ORE_BLOCK)) {
                return m_rawIronBlock;
            }
            return (blockY < 0) ? m_deepslateIronOre : m_ironOre;
        }
    }

    // 填充物：铜矿脉用花岗岩，铁矿脉用凝灰岩
    return isCopper ? m_granite : m_tuff;
}

// ============================================================================
// 懒初始化
// ============================================================================

void OreVeinifier::ensureInitialized()
{
    if (m_initialized) {
        return;
    }

    m_copperOre = VanillaBlocks::getState(block_registry::BaseBlocks::COPPER_ORE);
    m_deepslateCopperOre = VanillaBlocks::getState(block_registry::DeepslateBlocks::DEEPSLATE_COPPER_ORE);
    m_rawCopperBlock = VanillaBlocks::getState(block_registry::CopperBlocks::RAW_COPPER_BLOCK);
    m_granite = VanillaBlocks::getState(block_registry::BaseBlocks::GRANITE);
    m_ironOre = VanillaBlocks::getState(block_registry::BaseBlocks::IRON_ORE);
    m_deepslateIronOre = VanillaBlocks::getState(block_registry::DeepslateBlocks::DEEPSLATE_IRON_ORE);
    m_rawIronBlock = VanillaBlocks::getState(block_registry::CopperBlocks::RAW_IRON_BLOCK);
    m_tuff = VanillaBlocks::getState(block_registry::TuffBlocks::TUFF);

    m_initialized = true;
}

} // namespace mc::world::gen::density
