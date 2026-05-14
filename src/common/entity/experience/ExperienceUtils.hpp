#pragma once

#include "../../core/Types.hpp"
#include "../../util/math/random/Random.hpp"
#include "ExperienceConstants.hpp"

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
 * 根据时间和经验值计算经验球的颜色。
 * 产生绿色主色调的波动效果。
 *
 * 参考: ExperienceOrbEntity.render()
 *
 * @param xpValue 经验值 (影响颜色深浅)
 * @param time 时间参数 (ticks + partialTicks)
 * @return RGBA颜色值 (32位)
 */
inline u32 calculateOrbColor(i32 xpValue, f32 time)
{
    // 经验球颜色动画
    // 主色调是绿色，有轻微的红色和蓝色波动
    f32 phase = time / 2.0f;

    // 红色分量：轻微波动
    f32 red = (std::sin(phase) + 1.0f) * 0.5f * 0.3f;

    // 绿色分量：主要颜色，接近满值
    f32 green = 1.0f;

    // 蓝色分量：轻微波动，有相位偏移
    f32 blue = (std::sin(phase + 4.1887903f) + 1.0f) * 0.5f * 0.2f;

    // 根据经验值调整亮度
    i32 size = getOrbSize(xpValue);
    f32 brightness = 0.7f + static_cast<f32>(size) * 0.03f;

    // 转换为 RGBA
    u8 r = static_cast<u8>(std::min(255.0f, red * 255.0f * brightness));
    u8 g = static_cast<u8>(std::min(255.0f, green * 255.0f * brightness));
    u8 b = static_cast<u8>(std::min(255.0f, blue * 255.0f * brightness));
    u8 a = 255; // 完全不透明

    return (static_cast<u32>(a) << 24) | (static_cast<u32>(b) << 16) | (static_cast<u32>(g) << 8) | static_cast<u32>(r);
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
