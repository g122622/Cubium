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

#include "../../core/Types.hpp"
#include "../../util/math/MathConstants.hpp"
#include "../../util/math/Vector4.hpp"
#include "../../util/math/random/Random.hpp"
#include "ExperienceConstants.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace mc {
namespace entity {
namespace experience {

/**
 * @brief 经验工具函数
 *
 * 提供经验系统的辅助函数。
 */
namespace utils {

/**
 * @brief 获取经验分割值
 *
 * 将大量经验分割成适当大小的经验球。
 * 返回的值是单个经验球应有的经验量。
 *
 * 参考: ExperienceOrbEntity.getXPSplit()
 *
 * @param totalXp 要分割的总经验值
 * @return 单个经验球的经验值
 */
inline i32 getXPSplit(i32 totalXp)
{
    using namespace constants;

    // 从大到小查找合适的分割值
    for (i32 i = 0; i < XP_SPLIT_COUNT; ++i) {
        if (totalXp >= XP_SPLIT_VALUES[i]) {
            return XP_SPLIT_VALUES[i];
        }
    }

    return 1; // 最小返回1
}

/**
 * @brief 将经验值分割成多个经验球的值
 *
 * 将总经验值分割成适合生成经验球的列表。
 *
 * @param totalXp 总经验值
 * @param result 输出的经验球值列表
 */
inline void splitExperience(i32 totalXp, std::vector<i32>& result)
{
    result.clear();

    while (totalXp > 0) {
        i32 split = getXPSplit(totalXp);
        result.push_back(split);
        totalXp -= split;
    }
}

/**
 * @brief 获取经验球大小等级
 *
 * 根据经验值返回经验球的渲染大小等级 (0-10)。
 * 用于选择经验球的纹理。
 *
 * 参考: ExperienceOrbEntity.getTextureByXP()
 *
 * @param xpValue 经验值
 * @return 大小等级 (0-10)
 */
inline i32 getOrbSize(i32 xpValue)
{
    using namespace constants;

    if (xpValue >= XP_SPLIT_VALUES[0]) return 10; // 2477+
    if (xpValue >= XP_SPLIT_VALUES[1]) return 9;  // 1237-2476
    if (xpValue >= XP_SPLIT_VALUES[2]) return 8;  // 617-1236
    if (xpValue >= XP_SPLIT_VALUES[3]) return 7;  // 307-616
    if (xpValue >= XP_SPLIT_VALUES[4]) return 6;  // 149-306
    if (xpValue >= XP_SPLIT_VALUES[5]) return 5;  // 73-148
    if (xpValue >= XP_SPLIT_VALUES[6]) return 4;  // 37-72
    if (xpValue >= XP_SPLIT_VALUES[7]) return 3;  // 17-36
    if (xpValue >= XP_SPLIT_VALUES[8]) return 2;  // 7-16
    if (xpValue >= XP_SPLIT_VALUES[9]) return 1;  // 3-6
    return 0;                                     // 1-2
}

/**
 * @brief 计算经验球颜色
 *
 * 根据时间计算经验球的颜色动画。颜色在绿色和黄色之间循环，
 * 带有轻微的蓝色波动。
 *
 * 颜色公式（对齐 MC Java 版 ExperienceOrbRenderer.submit）：
 * - Red:   (sin(ageInTicks / 2 + 0) + 1) * 0.5 * 255
 * - Green: 255（固定）
 * - Blue:  (sin(ageInTicks / 2 + 4π/3) + 1) * 0.1 * 255
 * - Alpha: 128（半透明）
 *
 * @param time 时间参数 (ticks + partialTicks)
 * @return RGBA颜色向量 (范围 0.0-1.0)
 */
inline math::Vector4f calculateOrbColor(f64 time)
{
    f64 phase = time / 2.0;

    // 红色分量：在 0~255 之间波动，使颜色在绿色和黄色之间循环
    f32 red = static_cast<f32>((std::sin(phase) + 1.0) * 0.5 * 255.0) / 255.0f;

    // 绿色分量：固定满值
    f32 green = 1.0f;

    // 蓝色分量：轻微波动（0~约0.1），有 4π/3 相位偏移
    f32 blue = static_cast<f32>((std::sin(phase + math::PI * 4.0 / 3.0) + 1.0) * 0.1 * 255.0) / 255.0f;

    // Alpha：半透明（MC 原版为 128）
    f32 alpha = 128.0f / 255.0f;

    return math::Vector4f(red, green, blue, alpha);
}

/**
 * @brief 计算经验球精灵图集中图标的 UV 坐标
 *
 * 经验球纹理为 64x64 精灵图集，4列×3行布局，每个图标 16x16 像素。
 * 图标索引由 getOrbSize(xpValue) 确定，范围 0-10。
 *
 * 对齐 MC Java 版 ExperienceOrbRenderer.submit() 的 UV 计算。
 *
 * @param iconIndex 图标索引 (0-10，由 getOrbSize 返回)
 * @param[out] u0 UV 左边界
 * @param[out] v0 UV 上边界
 * @param[out] u1 UV 右边界
 * @param[out] v1 UV 下边界
 */
inline void calculateOrbIconUV(i32 iconIndex, f64& u0, f64& v0, f64& u1, f64& v1)
{
    // 经验球纹理为 64x64 精灵图集，4列×3行布局，每个图标 16x16 像素
    // 对齐 MC Java 版 ExperienceOrbRenderer.submit() 的 UV 计算
    constexpr i32 ATLAS_SIZE = 64;
    constexpr i32 ICON_SIZE = 16;
    constexpr i32 ICONS_PER_ROW = 4;

    const f64 iconU = static_cast<f64>((iconIndex % ICONS_PER_ROW) * ICON_SIZE);
    const f64 iconV = static_cast<f64>((iconIndex / ICONS_PER_ROW) * ICON_SIZE);
    const f64 atlasSize = static_cast<f64>(ATLAS_SIZE);

    u0 = iconU / atlasSize;
    v0 = iconV / atlasSize;
    u1 = (iconU + static_cast<f64>(ICON_SIZE)) / atlasSize;
    v1 = (iconV + static_cast<f64>(ICON_SIZE)) / atlasSize;
}

/**
 * @brief 生成随机矿石经验值
 *
 * @param rng 随机数生成器
 * @param oreType 矿石类型 (0=煤矿, 1=钻石, 2=绿宝石, 3=青金石, 4=下界石英, 5=下界金, 6=红石, 7=刷怪笼)
 * @return 经验值
 */
inline i32 randomOreExperience(math::Random& rng, i32 oreType)
{
    using namespace constants;

    switch (oreType) {
        case 0: // 煤矿
            return rng.nextInt(COAL_ORE_XP_MIN, COAL_ORE_XP_MAX);
        case 1: // 钻石矿
            return rng.nextInt(DIAMOND_ORE_XP_MIN, DIAMOND_ORE_XP_MAX);
        case 2: // 绿宝石矿
            return rng.nextInt(EMERALD_ORE_XP_MIN, EMERALD_ORE_XP_MAX);
        case 3: // 青金石矿
            return rng.nextInt(LAPIS_ORE_XP_MIN, LAPIS_ORE_XP_MAX);
        case 4: // 下界石英矿
            return rng.nextInt(NETHER_QUARTZ_ORE_XP_MIN, NETHER_QUARTZ_ORE_XP_MAX);
        case 5: // 下界金矿
            return rng.nextInt(NETHER_GOLD_ORE_XP_MIN, NETHER_GOLD_ORE_XP_MAX);
        case 6: // 红石矿
            return rng.nextInt(REDSTONE_ORE_XP_MIN, REDSTONE_ORE_XP_MAX);
        case 7: // 刷怪笼
            return rng.nextInt(SPAWNER_XP_MIN, SPAWNER_XP_MAX);
        default:
            return 0;
    }
}

/**
 * @brief 生成随机被动动物经验值
 *
 * @param rng 随机数生成器
 * @return 经验值 (1-3)
 */
inline i32 randomPassiveMobExperience(math::Random& rng)
{
    using namespace constants;
    return rng.nextInt(PASSIVE_MOB_XP_MIN, PASSIVE_MOB_XP_MAX);
}

/**
 * @brief 生成随机钓鱼经验值
 *
 * @param rng 随机数生成器
 * @return 经验值 (1-6)
 */
inline i32 randomFishingExperience(math::Random& rng)
{
    using namespace constants;
    return rng.nextInt(FISHING_XP_MIN, FISHING_XP_MAX);
}

/**
 * @brief 计算玩家死亡掉落经验
 *
 * @param level 玩家等级
 * @return 掉落经验值 (最大100)
 */
inline i32 calculateDeathDropXp(i32 level)
{
    using namespace constants;
    return std::min(level * DEATH_XP_PER_LEVEL, MAX_DEATH_XP_DROP);
}

/**
 * @brief 经验修补计算
 *
 * 计算用经验修复耐久需要消耗多少经验。
 *
 * @param durabilityToRepair 要修复的耐久值
 * @return 需要消耗的经验值
 */
inline i32 durabilityToXp(i32 durabilityToRepair)
{
    // 每点耐久需要 2 点经验
    // 参考: ExperienceOrbEntity.durabilityToXp
    return (durabilityToRepair + 1) / constants::XP_PER_DURABILITY;
}

/**
 * @brief 经验转换为可修复的耐久值
 *
 * @param xp 经验值
 * @return 可修复的耐久值
 */
inline i32 xpToDurability(i32 xp)
{
    // 每 2 点经验修复 1 点耐久
    return xp * constants::XP_PER_DURABILITY;
}

} // namespace utils

} // namespace experience
} // namespace entity
} // namespace mc
