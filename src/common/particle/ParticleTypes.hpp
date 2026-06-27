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
 * 对应 MC Java 版的 ParticleType / ParticleTypes 注册表。
 *
 * TODO: 当前枚举值采用自定义分组编号，与 MC Java 1.21.11 的协议 ID 不一致。
 *   MC 原版中粒子类型 ID 由注册顺序决定（0~114），而本项目采用按功能分组的编号方案。
 *   未来需统一为 MC 协议 ID 以保证网络兼容性。届时需同步修改：
 *   - ParticlePacket 的序列化/反序列化（VarInt 映射）
 *   - 客户端粒子渲染器的类型分发
 *   - 服务端粒子发送逻辑
 *   - 资源包中粒子定义的索引
 *
 * 分类：
 * - 0-9: 环境类粒子
 * - 10-19: 方块/物品类粒子
 * - 20-39: 效果类粒子
 * - 40-49: 液体滴落类粒子
 * - 50-59: 天气类粒子
 * - 60-69: 生物相关粒子
 * - 70-79: 特殊粒子
 * - 80-127: 预留扩展
 */
enum class ParticleTypeId : u16 {
    // ========================================================================
    // 环境类粒子 (0-9)
    // ========================================================================

    /// 环境实体效果（信标效果等）
    AmbientEntityEffect = 0,

    /// 水下气泡
    Bubble = 1,

    /// 气泡破裂
    BubblePop = 2,

    /// 气泡柱上升
    BubbleColumnUp = 3,

    /// 向下的水流
    CurrentDown = 4,

    /// 水下悬浮粒子
    Underwater = 5,

    /// 屏障粒子（显示屏障方块）
    Barrier = 6,

    /// 光标粒子（显示结构方块位置）
    Light = 7,

    /// 灵魂火焰
    SoulFireFlame = 8,

    /// 灵魂
    Soul = 9,

    // ========================================================================
    // 方块/物品类粒子 (10-19)
    // ========================================================================

    /// 方块粒子（带方块状态）
    Block = 10,

    /// 方块破坏粒子
    Breaking = 11,

    /// 下落灰尘粒子
    FallingDust = 12,

    /// 物品粒子（带物品数据）
    Item = 13,

    /// 史莱姆粒子
    ItemSlime = 14,

    /// 雪球粒子
    ItemSnowball = 15,

    /// 尘柱粒子（重锤砸地攻击产生，使用方块状态纹理）
    DustPillar = 16,

    // ========================================================================
    // 效果类粒子 (20-39)
    // ========================================================================

    /// 火焰粒子
    Flame = 20,

    /// 烟雾粒子
    Smoke = 21,

    /// 大烟雾粒子
    LargeSmoke = 22,

    /// 白色烟雾粒子（合成器发射、干涸恶魂方块等）
    WhiteSmoke = 56,

    /// 熔岩飞溅粒子
    Lava = 23,

    /// 传送门粒子
    Portal = 24,

    /// 反向传送门粒子
    ReversePortal = 25,

    /// 爆炸粒子
    Explosion = 26,

    /// 消散粒子
    Poof = 27,

    /// 暴击粒子
    Crit = 28,

    /// 附魔暴击粒子
    EnchantedHit = 29,

    /// 药水效果粒子
    Spell = 30,

    /// 瞬间药水效果粒子
    InstantSpell = 31,

    /// 实体效果粒子
    EntityEffect = 32,

    /// 红石粉尘粒子
    Redstone = 33,

    /// 附魔台符文粒子
    Enchant = 34,

    /// 扫荡攻击粒子
    SweepAttack = 35,

    /// 羊驼吐沫粒子
    Spit = 36,

    /// 鱿鱼墨汁粒子
    SquidInk = 37,

    /// 末影龙息粒子
    DragonBreath = 38,

    /// 末地烛粒子
    EndRod = 39,

    // ========================================================================
    // 液体滴落类粒子 (40-49)
    // ========================================================================

    /// 音符粒子（音符盒）
    Note = 40,

    /// 滴落的水
    DrippingWater = 41,

    /// 下落的水
    FallingWater = 42,

    /// 滴落的熔岩
    DrippingLava = 43,

    /// 下落的熔岩
    FallingLava = 44,

    /// 落地的熔岩
    LandingLava = 45,

    /// 滴落的蜂蜜
    DrippingHoney = 46,

    /// 下落的蜂蜜
    FallingHoney = 47,

    /// 落地的蜂蜜
    LandingHoney = 48,

    /// 滴落的黑曜石眼泪（哭泣的黑曜石）
    DrippingObsidianTear = 49,

    /// 下落的黑曜石眼泪
    FallingObsidianTear = 50,

    // ========================================================================
    // 天气类粒子 (51-59)
    // ========================================================================

    /// 雨滴粒子
    Rain = 51,

    /// 雪花粒子
    Snowflake = 52,

    /// 水溅粒子
    Splash = 53,

    /// 云朵粒子
    Cloud = 54,

    /// 钓鱼粒子（水面涟漪效果）
    Fishing = 55,

    /// 菌丝粒子（幻翼翼尖、菌丝方块表面，MC 原版 SuspendedTownParticle）
    Mycelium = 57,

    // ========================================================================
    // 生物相关粒子 (60-69)
    // ========================================================================

    /// 爱心粒子
    Heart = 60,

    /// 愤怒村民粒子
    AngryVillager = 61,

    /// 开心村民粒子
    HappyVillager = 62,

    /// 喷嚏粒子（熊猫）
    Sneeze = 63,

    /// 海豚粒子
    Dolphin = 64,

    // ========================================================================
    // 特殊粒子 (70-79)
    // ========================================================================

    /// 不死图腾粒子
    TotemOfUndying = 70,

    /// 闪光粒子
    Flash = 71,

    /// 守护者外观粒子
    ElderGuardian = 72,

    /// 鹦鹉螺粒子
    Nautilus = 73,

    /// 烟花粒子
    Firework = 74,

    // ========================================================================
    // 下界更新粒子 (80-99)
    // ========================================================================

    /// 灰烬粒子
    Ash = 80,

    /// 白色灰烬粒子
    WhiteAsh = 81,

    /// 绯红孢子
    CrimsonSpore = 82,

    /// 诡异孢子
    WarpedSpore = 83,

    /// 落地的黑曜石眼泪
    LandingObsidianTear = 84,

    /// 染色粒子
    Dust = 85,

    /// 颜色过渡的染色粒子
    DustColorTransition = 86,

    /// 振动粒子
    Vibration = 87,

    /// 荧光墨囊粒子
    GlowSquidInk = 88,

    /// 荧光地衣粒子
    Glow = 89,

    /// 蜡烛粒子
    WaxOff = 90,

    /// 蜡烛粒子
    WaxOn = 91,

    /// 潮涌核心粒子
    SculkSoul = 92,

    /// 幽匿块粒子
    SculkCharge = 93,

    /// 幽匿感测体粒子
    SculkChargePop = 94,

    /// 幽匿尖啸体粒子
    Shriek = 95,

    /// 樱桃树叶粒子
    CherryLeaves = 96,

    /// 刷沙粒子
    DrippingCherryLeaves = 97,

    /// 下落的樱花树叶
    FallingCherryLeaves = 98,

    /// 落地的樱花树叶
    LandingCherryLeaves = 99,

    /// 孢子花掉落粒子
    FallingSporeBlossom = 105,

    /// 孢子花空气粒子
    SporeBlossomAir = 106,

    /// 风爆发射器粒子（小型）
    GustEmitterSmall = 107,

    /// 风爆发射器粒子（大型）
    GustEmitterLarge = 108,

    /// 滴落的滴水石水（钟乳石滴水）
    /// TODO: 与 OminousSpawning 枚举值冲突（均为 109），需要与 MC 原版协议 ID 对齐时一并修复。
    ///   MC Java 1.21.11 中粒子类型 ID 由注册顺序决定（0~114），当前项目采用自定义分组编号，
    ///   两者不一致，未来需统一为协议 ID 以保证网络兼容性。
    DrippingDripstoneWater = 109,

    /// 下落的滴水石水
    FallingDripstoneWater = 110,

    /// 滴落的滴水石熔岩（钟乳石滴熔岩）
    DrippingDripstoneLava = 111,

    /// 下落的滴水石熔岩
    FallingDripstoneLava = 112,

    // ========================================================================
    // 扩展粒子 (100-112)
    // ========================================================================

    /// 营火烟雾（普通）
    CampfireCozy = 100,

    /// 营火烟雾（信号）
    CampfireSignal = 101,

    /// 大型爆炸粒子
    LargeExplosion = 102,

    /// 巨型爆炸粒子（元粒子）
    HugeExplosion = 103,

    /// 物品拾取粒子
    ItemPickup = 104,

    /// 不祥生成粒子（不祥物品生成器周围生成的不祥粒子）
    /// TODO: 与 DrippingDripstoneWater 枚举值冲突（均为 109），需要与 MC 原版协议 ID 对齐时一并修复。
    ///   MC Java 1.21.11 中粒子类型 ID 由注册顺序决定（0~114），当前项目采用自定义分组编号，
    ///   两者不一致，未来需统一为协议 ID 以保证网络兼容性。
    OminousSpawning = 109,

    // ========================================================================
    // 计数与无效值
    // ========================================================================

    /// 粒子类型总数
    Count = 128,

    /// 无效的粒子类型
    Invalid = static_cast<u16>(-1)
};

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
 * @brief 检查粒子是否需要方块状态数据
 *
 * @param id 粒子类型 ID
 * @return 是否需要方块状态
 */
[[nodiscard]] constexpr bool requiresBlockState(ParticleTypeId id)
{
    return id == ParticleTypeId::Block || id == ParticleTypeId::Breaking || id == ParticleTypeId::FallingDust ||
        id == ParticleTypeId::DustPillar;
}

/**
 * @brief 检查粒子是否需要物品数据
 *
 * @param id 粒子类型 ID
 * @return 是否需要物品数据
 */
[[nodiscard]] constexpr bool requiresItemData(ParticleTypeId id)
{
    return id == ParticleTypeId::Item || id == ParticleTypeId::ItemSlime || id == ParticleTypeId::ItemSnowball;
}

/**
 * @brief 检查粒子是否需要红石颜色数据
 *
 * @param id 粒子类型 ID
 * @return 是否需要红石颜色数据
 */
[[nodiscard]] constexpr bool requiresDustColor(ParticleTypeId id)
{
    return id == ParticleTypeId::Redstone || id == ParticleTypeId::Dust || id == ParticleTypeId::DustColorTransition;
}

/**
 * @brief 检查粒子类型是否需要振动数据（目标位置 + 到达时间）
 *
 * @param id 粒子类型 ID
 * @return 是否需要振动数据
 */
[[nodiscard]] constexpr bool requiresVibrationData(ParticleTypeId id)
{
    return id == ParticleTypeId::Vibration;
}

} // namespace mc::particle
