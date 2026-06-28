/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

namespace mc::particle {

/**
 * @brief 粒子类型 ID 枚举
 *
 * 定义所有粒子类型的唯一标识符。此枚举定义在 common 层，
 * 供 common/client/server 三层共同使用。
 *
 * 对应 MC Java 1.21.11 的 ParticleType / ParticleTypes 注册表。
 * 枚举值与 MC 协议 ID 完全一致（由注册顺序决定，0~114），
 * 因此可直接用于网络序列化，无需额外的映射层。
 *
 * 分类（基于 MC 功能分组，不影响枚举值）：
 * - 0~2: 方块类粒子（需要方块状态数据）
 * - 3~9: 环境类粒子
 * - 10~13: 液体滴落类粒子
 * - 14~15: 染色粒子（需要颜色数据）
 * - 16~28: 效果类粒子
 * - 29~31: 方块/物品/烟花粒子
 * - 32~52: 更多效果和物品粒子
 * - 53~69: 烟雾/天气/生物粒子
 * - 70~79: 水下/营地/蜂蜜粒子
 * - 80~98: 下界/末地/幽匿/滴水石粒子
 * - 99~114: 铜蚀/幽匿/试炼/不祥粒子
 */
enum class ParticleTypeId : u16 {
    // ========================================================================
    // 方块类粒子 (0-2)
    // ========================================================================

    /// 愤怒村民粒子（村民不满时的愤怒气泡）
    AngryVillager = 0,

    /// 方块粒子（带方块状态，用于方块破坏和放置）
    Block = 1,

    /// 方块标记粒子（带方块状态，用于结构方块等标记显示）
    BlockMarker = 2,

    // ========================================================================
    // 环境类粒子 (3-9)
    // ========================================================================

    /// 水下气泡
    Bubble = 3,

    /// 云朵粒子
    Cloud = 4,

    /// 铜火火焰粒子
    CopperFireFlame = 5,

    /// 暴击粒子
    Crit = 6,

    /// 伤害指示器粒子（实体受伤时弹出）
    DamageIndicator = 7,

    /// 末影龙息粒子
    DragonBreath = 8,

    // ========================================================================
    // 液体滴落类粒子 (9-13)
    // ========================================================================

    /// 滴落的熔岩
    DrippingLava = 9,

    /// 下落的熔岩
    FallingLava = 10,

    /// 落地的熔岩
    LandingLava = 11,

    /// 滴落的水
    DrippingWater = 12,

    /// 下落的水
    FallingWater = 13,

    // ========================================================================
    // 染色粒子 (14-15)
    // ========================================================================

    /// 染色粒子（带颜色数据，包含红石粉尘粒子）
    Dust = 14,

    /// 颜色过渡的染色粒子
    DustColorTransition = 15,

    // ========================================================================
    // 效果类粒子 (16-28)
    // ========================================================================

    /// 药水效果粒子（带药水类型数据）
    Spell = 16,

    /// 守卫者外观粒子
    ElderGuardian = 17,

    /// 附魔暴击粒子
    EnchantedHit = 18,

    /// 附魔台符文粒子
    Enchant = 19,

    /// 末地烛粒子
    EndRod = 20,

    /// 实体效果粒子（带颜色数据，信标效果等）
    EntityEffect = 21,

    /// 巨型爆炸粒子（explosion_emitter）
    HugeExplosion = 22,

    /// 爆炸粒子
    Explosion = 23,

    /// 风爆粒子
    Gust = 24,

    /// 小型风爆粒子
    SmallGust = 25,

    /// 大型风爆发射器粒子
    GustEmitterLarge = 26,

    /// 小型风爆发射器粒子
    GustEmitterSmall = 27,

    /// 声波轰击粒子（监守者远程攻击）
    SonicBoom = 28,

    // ========================================================================
    // 方块/物品/烟花粒子 (29-31)
    // ========================================================================

    /// 下落灰尘粒子（带方块状态）
    FallingDust = 29,

    /// 烟花粒子
    Firework = 30,

    /// 钓鱼粒子（水面涟漪效果）
    Fishing = 31,

    // ========================================================================
    // 火焰/效果粒子 (32-52)
    // ========================================================================

    /// 火焰粒子
    Flame = 32,

    /// 虫蚀方块粒子（蠹虫出现时）
    Infested = 33,

    /// 樱花树叶粒子
    CherryLeaves = 34,

    /// 苍白橡树树叶粒子
    PaleOakLeaves = 35,

    /// 着色树叶粒子（带颜色数据）
    TintedLeaves = 36,

    /// 幽匿灵魂粒子
    SculkSoul = 37,

    /// 幽匿充能粒子（带充能数据）
    SculkCharge = 38,

    /// 幽匿充能弹出粒子
    SculkChargePop = 39,

    /// 灵魂火焰
    SoulFireFlame = 40,

    /// 灵魂
    Soul = 41,

    /// 闪光粒子（带颜色数据）
    Flash = 42,

    /// 开心村民粒子
    HappyVillager = 43,

    /// 堆肥桶粒子
    Composter = 44,

    /// 爱心粒子
    Heart = 45,

    /// 瞬间药水效果粒子（带药水类型数据）
    InstantSpell = 46,

    /// 物品粒子（带物品数据）
    Item = 47,

    /// 振动粒子（带目标位置和到达时间数据）
    Vibration = 48,

    /// 轨迹粒子（带颜色/目标数据）
    Trail = 49,

    /// 史莱姆粒子
    ItemSlime = 50,

    /// 蛛网物品粒子
    ItemCobweb = 51,

    /// 雪球粒子
    ItemSnowball = 52,

    // ========================================================================
    // 烟雾/天气/生物粒子 (53-69)
    // ========================================================================

    /// 大烟雾粒子
    LargeSmoke = 53,

    /// 熔岩飞溅粒子
    Lava = 54,

    /// 菌丝粒子
    Mycelium = 55,

    /// 音符粒子（音符盒）
    Note = 56,

    /// 消散粒子
    Poof = 57,

    /// 传送门粒子
    Portal = 58,

    /// 雨滴粒子
    Rain = 59,

    /// 烟雾粒子
    Smoke = 60,

    /// 白色烟雾粒子
    WhiteSmoke = 61,

    /// 喷嚏粒子（熊猫）
    Sneeze = 62,

    /// 羊驼吐沫粒子
    Spit = 63,

    /// 鱿鱼墨汁粒子
    SquidInk = 64,

    /// 扫荡攻击粒子
    SweepAttack = 65,

    /// 不死图腾粒子
    TotemOfUndying = 66,

    /// 水下悬浮粒子
    Underwater = 67,

    /// 水溅粒子
    Splash = 68,

    /// 女巫粒子
    Witch = 69,

    // ========================================================================
    // 水下/营地/蜂蜜粒子 (70-79)
    // ========================================================================

    /// 气泡破裂
    BubblePop = 70,

    /// 向下的水流
    CurrentDown = 71,

    /// 气泡柱上升
    BubbleColumnUp = 72,

    /// 鹦鹉螺粒子
    Nautilus = 73,

    /// 海豚粒子
    Dolphin = 74,

    /// 营火烟雾（普通）
    CampfireCozy = 75,

    /// 营火烟雾（信号）
    CampfireSignal = 76,

    /// 滴落的蜂蜜
    DrippingHoney = 77,

    /// 下落的蜂蜜
    FallingHoney = 78,

    /// 落地的蜂蜜
    LandingHoney = 79,

    // ========================================================================
    // 花蜜/孢子/下界粒子 (80-98)
    // ========================================================================

    /// 下落的花蜜粒子（蜜蜂相关）
    FallingNectar = 80,

    /// 孢子花掉落粒子
    FallingSporeBlossom = 81,

    /// 灰烬粒子
    Ash = 82,

    /// 绯红孢子
    CrimsonSpore = 83,

    /// 诡异孢子
    WarpedSpore = 84,

    /// 孢子花空气粒子
    SporeBlossomAir = 85,

    /// 滴落的黑曜石眼泪
    DrippingObsidianTear = 86,

    /// 下落的黑曜石眼泪
    FallingObsidianTear = 87,

    /// 落地的黑曜石眼泪
    LandingObsidianTear = 88,

    /// 反向传送门粒子
    ReversePortal = 89,

    /// 白色灰烬粒子
    WhiteAsh = 90,

    /// 小型火焰粒子（蜡烛等）
    SmallFlame = 91,

    /// 雪花粒子
    Snowflake = 92,

    /// 滴落的滴水石熔岩
    DrippingDripstoneLava = 93,

    /// 下落的滴水石熔岩
    FallingDripstoneLava = 94,

    /// 滴落的滴水石水
    DrippingDripstoneWater = 95,

    /// 下落的滴水石水
    FallingDripstoneWater = 96,

    /// 荧光墨囊粒子
    GlowSquidInk = 97,

    /// 荧光地衣粒子
    Glow = 98,

    // ========================================================================
    // 铜蚀/幽匿/试炼/不祥粒子 (99-114)
    // ========================================================================

    /// 蜡烛涂抹粒子（上蜡）
    WaxOn = 99,

    /// 蜡烛涂抹粒子（除蜡）
    WaxOff = 100,

    /// 电火花粒子（避雷针等）
    ElectricSpark = 101,

    /// 刮擦粒子（铜氧化去除）
    Scrape = 102,

    /// 幽匿尖啸体粒子（带延迟数据）
    Shriek = 103,

    /// 蛋破裂粒子
    EggCrack = 104,

    /// 尘柱粒子
    DustPlume = 105,

    /// 试炼刷怪笼检测粒子
    TrialSpawnerDetection = 106,

    /// 试炼刷怪笼检测粒子（不祥）
    TrialSpawnerDetectionOminous = 107,

    /// 宝库连接粒子
    VaultConnection = 108,

    /// 尘柱粒子（带方块状态，重锤砸地攻击产生）
    DustPillar = 109,

    /// 不祥生成粒子
    OminousSpawning = 110,

    /// 袭击预兆粒子
    RaidOmen = 111,

    /// 试炼预兆粒子
    TrialOmen = 112,

    /// 方块碎裂粒子（带方块状态）
    BlockCrumble = 113,

    /// 萤火虫粒子
    Firefly = 114,

    // ========================================================================
    // 项目内部扩展粒子（不在 MC 协议中，用于内部渲染等）
    // ========================================================================

    /// 方块破坏粒子（项目内部，MC 中 Block 兼用此功能）
    Breaking = 115,

    /// 屏障粒子（显示屏障方块，项目内部）
    Barrier = 116,

    /// 光标粒子（显示结构方块位置，项目内部）
    Light = 117,

    /// 红石粉尘粒子（项目内部兼容，MC 中由 Dust + 颜色数据实现）
    Redstone = 118,

    /// 大型爆炸粒子（项目内部，MC 中为 HugeExplosion / explosion_emitter）
    LargeExplosion = 119,

    /// 物品拾取粒子（项目内部）
    ItemPickup = 120,

    /// 滴落的樱花树叶（项目内部，MC 中仅有 CherryLeaves）
    DrippingCherryLeaves = 121,

    /// 下落的樱花树叶（项目内部，MC 中仅有 CherryLeaves）
    FallingCherryLeaves = 122,

    /// 落地的樱花树叶（项目内部，MC 中仅有 CherryLeaves）
    LandingCherryLeaves = 123,

    // ========================================================================
    // 计数与无效值
    // ========================================================================

    /// 粒子类型总数（含内部扩展）
    Count = 124,

    /// 无效的粒子类型
    Invalid = static_cast<u16>(-1)
};

/**
 * @brief MC 协议中粒子类型的最大有效 ID
 *
 * MC Java 1.21.11 中粒子类型 ID 范围为 0~114，
 * 超过此范围的值为本项目内部扩展，不应用于网络通信。
 */
inline constexpr u16 PROTOCOL_PARTICLE_TYPE_COUNT = 115;

/**
 * @brief 检查粒子类型 ID 是否有效
 *
 * @param id 粒子类型 ID
 * @return 是否有效
 */
[[nodiscard]] constexpr bool isValidParticleType(ParticleTypeId id)
{
    return static_cast<u16>(id) < static_cast<u16>(ParticleTypeId::Count);
}

/**
 * @brief 检查粒子类型 ID 是否为 MC 协议中定义的类型
 *
 * 协议 ID 范围为 0~114，超出此范围的为本项目内部扩展类型，
 * 不应用于网络序列化。
 *
 * @param id 粒子类型 ID
 * @return 是否为协议类型
 */
[[nodiscard]] constexpr bool isProtocolParticleType(ParticleTypeId id)
{
    return static_cast<u16>(id) < PROTOCOL_PARTICLE_TYPE_COUNT;
}

/**
 * @brief 检查粒子是否需要方块状态数据
 *
 * @param id 粒子类型 ID
 * @return 是否需要方块状态
 */
[[nodiscard]] constexpr bool requiresBlockState(ParticleTypeId id)
{
    return id == ParticleTypeId::Block || id == ParticleTypeId::BlockMarker || id == ParticleTypeId::FallingDust ||
        id == ParticleTypeId::DustPillar || id == ParticleTypeId::BlockCrumble ||
        id == ParticleTypeId::Breaking; // 内部扩展
}

/**
 * @brief 检查粒子是否需要物品数据
 *
 * @param id 粒子类型 ID
 * @return 是否需要物品数据
 */
[[nodiscard]] constexpr bool requiresItemData(ParticleTypeId id)
{
    return id == ParticleTypeId::Item || id == ParticleTypeId::ItemSlime || id == ParticleTypeId::ItemSnowball ||
        id == ParticleTypeId::ItemCobweb;
}

/**
 * @brief 检查粒子是否需要颜色数据
 *
 * 包括 Dust（红石粉尘粒子）、DustColorTransition（颜色过渡）、
 * EntityEffect（实体效果粒子）、Flash（闪光粒子）和 TintedLeaves（着色树叶）。
 *
 * @param id 粒子类型 ID
 * @return 是否需要颜色数据
 */
[[nodiscard]] constexpr bool requiresColorData(ParticleTypeId id)
{
    return id == ParticleTypeId::Dust || id == ParticleTypeId::DustColorTransition ||
        id == ParticleTypeId::EntityEffect || id == ParticleTypeId::Flash || id == ParticleTypeId::TintedLeaves;
}

/**
 * @brief 检查粒子是否需要红石颜色数据（向后兼容）
 *
 * @param id 粒子类型 ID
 * @return 是否需要红石颜色数据
 */
[[nodiscard]] constexpr bool requiresDustColor(ParticleTypeId id)
{
    return id == ParticleTypeId::Dust || id == ParticleTypeId::DustColorTransition ||
        id == ParticleTypeId::Redstone; // 内部扩展：红石粉尘粒子
}

/**
 * @brief 检查粒子是否需要药水类型数据
 *
 * @param id 粒子类型 ID
 * @return 是否需要药水类型数据
 */
[[nodiscard]] constexpr bool requiresSpellData(ParticleTypeId id)
{
    return id == ParticleTypeId::Spell || id == ParticleTypeId::InstantSpell;
}

/**
 * @brief 检查粒子是否需要振动数据（目标位置 + 到达时间）
 *
 * @param id 粒子类型 ID
 * @return 是否需要振动数据
 */
[[nodiscard]] constexpr bool requiresVibrationData(ParticleTypeId id)
{
    return id == ParticleTypeId::Vibration;
}

/**
 * @brief 检查粒子是否需要幽匿充能数据
 *
 * @param id 粒子类型 ID
 * @return 是否需要幽匿充能数据
 */
[[nodiscard]] constexpr bool requiresSculkChargeData(ParticleTypeId id)
{
    return id == ParticleTypeId::SculkCharge;
}

/**
 * @brief 检查粒子是否需要尖啸延迟数据
 *
 * @param id 粒子类型 ID
 * @return 是否需要尖啸延迟数据
 */
[[nodiscard]] constexpr bool requiresShriekData(ParticleTypeId id)
{
    return id == ParticleTypeId::Shriek;
}

/**
 * @brief 检查粒子是否需要轨迹数据
 *
 * @param id 粒子类型 ID
 * @return 是否需要轨迹数据
 */
[[nodiscard]] constexpr bool requiresTrailData(ParticleTypeId id)
{
    return id == ParticleTypeId::Trail;
}

/**
 * @brief 检查粒子是否需要力量数据
 *
 * DragonBreath 粒子在 MC 中使用 PowerParticleOption。
 *
 * @param id 粒子类型 ID
 * @return 是否需要力量数据
 */
[[nodiscard]] constexpr bool requiresPowerData(ParticleTypeId id)
{
    return id == ParticleTypeId::DragonBreath;
}

} // namespace mc::particle
