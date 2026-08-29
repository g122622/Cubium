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
#include "../../util/math/Vector3.hpp"
#include <memory>
#include <optional>
#include <string>

namespace mc {

// 前向声明
class Entity;
class LivingEntity;
class DamageTypeTag;

// ============================================================================
// 保护附魔伤害类型标志位
// 用于 ProtectionEnchantment 计算伤害减免
// ============================================================================
namespace DamageFlags {
constexpr u32 FIRE = 0x01;       // 火焰/岩浆: InFire, OnFire, Lava, HotFloor
constexpr u32 FALL = 0x04;       // 摔落: Fall, FlyIntoWall
constexpr u32 EXPLOSION = 0x08;  // 爆炸: Explosion, ExplosionPlayer
constexpr u32 PROJECTILE = 0x10; // 弹射物: Arrow, Trident, MobProjectile, Fireball
} // namespace DamageFlags

/**
 * @brief 伤害类型枚举
 *
 * 定义不同类型的伤害来源。
 */
enum class DamageType : u8 {
    // 环境伤害
    InFire,            // 在火焰中
    Campfire,          // 营火
    LightningBolt,     // 闪电
    OnFire,            // 燃烧
    Lava,              // 岩浆
    HotFloor,          // 岩浆块
    InWall,            // 窒息（在方块内）
    Cramming,          // 拥挤伤害（实体过多）
    Drown,             // 溺水
    Starve,            // 饥饿
    Cactus,            // 仙人掌
    Fall,              // 摔落
    EnderPearl,        // 末影珍珠摔落伤害
    FlyIntoWall,       // 撞墙（鞘翅飞行）
    OutOfWorld,        // 虚空
    Generic,           // 通用伤害
    Magic,             // 魔法伤害
    Wither,            // 凋零
    DragonBreath,      // 龙息
    Dryout,            // 干涸伤害（鱼离开水）
    SweetBerryBush,    // 甜浆果丛
    Freeze,            // 冰冻伤害（细雪冰冻）
    Stalagmite,        // 石笋摔落伤害（踩在朝上的滴石尖端上）
    FallingBlock,      // 坠落方块
    FallingAnvil,      // 坠落铁砧
    FallingStalactite, // 坠落钟乳石伤害（钟乳石掉落砸中实体）

    // 实体伤害
    Sting,                // 蜜蜂蛰刺
    MobAttack,            // 生物攻击
    MobAttackNoAggro,     // 生物攻击（不激怒）
    PlayerAttack,         // 玩家攻击
    Spear,                // 三叉戟（矛）近战攻击
    Arrow,                // 箭矢
    Trident,              // 三叉戟（投射物）
    MobProjectile,        // 生物投射物
    Spit,                 // 羊驼喷吐
    Fireworks,            // 烟花
    UnattributedFireball, // 无归属火球（无射击者）
    Fireball,             // 火球
    WitherSkull,          // 凋灵之首
    Thrown,               // 投掷物（雪球、鸡蛋、末影珍珠等通用投掷伤害）
    IndirectMagic,        // 间接魔法伤害（药水等）
    Thorns,               // 荆棘
    Explosion,            // 爆炸
    ExplosionPlayer,      // 玩家爆炸
    SonicBoom,            // 监守者音爆攻击
    BadRespawnPoint,      // 床重生爆炸（"-intentional_game_design"）
    WindBurst,            // 风弹风爆
    MaceSmash,            // 重锤下落攻击

    // 边界与通用击杀
    OutsideBorder, // 世界边界外伤害
    GenericKill,   // 通用击杀（/kill 命令）
};

/**
 * @brief 伤害类型的难度缩放策略
 *
 * 对齐 MC Java 1.21.11 DamageType 的 scaling 字段（数据包 damage_type 目录下各 JSON 的 "scaling"）。
 * vanilla DamageSource.scalesWithDifficulty()（DamageSource.java:90-96）据此动态判定：
 *   - NEVER  → false（永不缩放）
 *   - WHEN_CAUSED_BY_LIVING_NON_PLAYER → causingEntity instanceof LivingEntity
 *                                        && !(causingEntity instanceof Player)（非玩家生物造成时才缩放）
 *   - ALWAYS → true（无条件缩放）
 */
enum class DamageScaling : u8 {
    Never,
    WhenCausedByLivingNonPlayer,
    Always,
};

/**
 * @brief 查询伤害类型的难度缩放策略（对齐 vanilla 数据包 scaling 字段）
 *
 * 把 MC 1.21.11 数据包 damage_type 目录下各 JSON 的 "scaling" 值固化进代码，与 bypassesArmor()/
 * isFire() 等"硬编码对齐数据包标签/字段"范式一致。数据包内 scaling 取值统计：
 *   - ALWAYS：explosion, player_explosion, sonic_boom, bad_respawn_point（共 4 类）
 *   - WHEN_CAUSED_BY_LIVING_NON_PLAYER：其余全部（含 fall/drown/cactus/in_fire/mob_attack 等）
 *   - NEVER：无（vanilla 1.21.11 数据包无 NEVER scaling 的伤害类型）
 *
 * 注：Cubium DamageType 是硬编码枚举，未接入数据驱动的 scaling 字段加载（见任务 #352 伤害类型
 * 数据驱动审计）。当前把数据包值固化在此，未来若接入 DamageType 数据驱动加载，可改为读取字段。
 */
DamageScaling damageScaling(DamageType type) noexcept;

/**
 * @brief 伤害来源基类
 *
 * 定义伤害的来源和类型，用于计算伤害、死亡消息等。
 */
class DamageSource {
public:
    virtual ~DamageSource() = default;

    /**
     * @brief 克隆伤害来源
     * @return 伤害来源的副本
     */
    [[nodiscard]] virtual std::unique_ptr<DamageSource> clone() const = 0;

    /**
     * @brief 获取伤害类型
     */
    [[nodiscard]] virtual DamageType type() const = 0;

    /**
     * @brief 获取伤害来源实体（如果有）
     * @return 伤害来源实体，没有则返回nullptr
     */
    [[nodiscard]] virtual Entity* source() const { return nullptr; }

    /**
     * @brief 获取直接伤害来源实体
     * @return 直接造成伤害的实体，没有则返回nullptr
     */
    [[nodiscard]] virtual Entity* directSource() const { return nullptr; }

    /**
     * @brief 获取造成伤害的实体
     *
     * 对于直接伤害返回source()，对于间接伤害返回source()
     * CombatTracker使用此方法获取攻击者
     */
    [[nodiscard]] virtual Entity* getEntity() const { return source(); }

    /**
     * @brief 获取真正的伤害来源
     *
     * 返回造成伤害的实体，用于 HurtByTargetGoal 等目标选择。
     */
    [[nodiscard]] virtual Entity* getTrueSource() const { return getEntity(); }

    /**
     * @brief 获取伤害来源的世界位置（DamageSource.getSourcePosition）
     *
     * 用于计算受伤方向（hurtDir / damageTilt）。优先返回显式记录的位置，
     * 否则回退到直接来源实体（directSource）的位置。无实体来源时返回 nullopt。
     */
    [[nodiscard]] virtual std::optional<math::Vector3f> sourcePosition() const { return std::nullopt; }

    /**
     * @brief 是否可以绕过护甲
     */
    [[nodiscard]] virtual bool bypassesArmor() const { return false; }

    /**
     * @brief 是否可以绕过无敌
     * 忽略药水效果和附魔
     */
    [[nodiscard]] virtual bool bypassesInvulnerability() const { return false; }

    /**
     * @brief 是否可以在创造模式下造成伤害
     */
    [[nodiscard]] virtual bool canDamageCreative() const { return false; }

    /**
     * @brief 是否是火焰伤害
     */
    [[nodiscard]] virtual bool isFire() const { return false; }

    /**
     * @brief 是否是投射物伤害
     */
    [[nodiscard]] virtual bool isProjectile() const { return false; }

    /**
     * @brief 是否是魔法伤害
     */
    [[nodiscard]] virtual bool isMagic() const { return false; }

    /**
     * @brief 是否是爆炸伤害
     */
    [[nodiscard]] virtual bool isExplosion() const { return false; }

    /**
     * @brief 获取死亡消息键
     */
    [[nodiscard]] virtual std::string deathMessageKey() const = 0;

    /**
     * @brief 是否来自实体
     */
    [[nodiscard]] virtual bool isEntitySource() const { return false; }

    /**
     * @brief 是否来自玩家
     */
    [[nodiscard]] virtual bool isPlayerSource() const { return false; }

    /**
     * @brief 是否受难度缩放
     */
    [[nodiscard]] virtual bool isDifficultyScaled() const { return false; }

    /**
     * @brief 该伤害是否随难度缩放（对齐 vanilla DamageSource.scalesWithDifficulty()，
     *        DamageSource.java:90-96）
     *
     * vanilla 依据伤害类型的 scaling 字段（NEVER/WHEN_CAUSED_BY_LIVING_NON_PLAYER/ALWAYS）
     * 在 hurt 时动态判定是否按难度调整伤害值。语义：
     *   - NEVER  → false
     *   - WHEN_CAUSED_BY_LIVING_NON_PLAYER → causingEntity 是非玩家 LivingEntity 时 true
     *   - ALWAYS → true
     *
     * 此前 Cubium 用静态 m_difficultyScaled flag 表达（仅在 8 个工厂设置），但 flag 是死代码——
     * Player::hurt 与整个 hurt 链路从未读取它，难度缩放从未生效。且静态 flag 无法表达
     * WHEN_CAUSED_BY_LIVING_NON_PLAYER 的"由玩家造成时不缩放"动态语义（如玩家射的箭被
     * mobProjectile 工厂 setDifficultyScaled 后，flag=true，但 vanilla 此时应 false）。
     *
     * 现改为数据驱动动态判定：基类默认实现查 damageScaling(type())，并按 getTrueSource()
     * （vanilla causingEntity）是否为非玩家 LivingEntity 决定 WHEN_CAUSED_BY_LIVING_NON_PLAYER
     * 分支。实现在 DamageSource.cpp（需 LivingEntity/Player 完整定义做 dynamic_cast）。
     */
    [[nodiscard]] virtual bool scalesWithDifficulty() const;

    /**
     * @brief 该伤害是否由创造模式玩家造成（对齐 vanilla DamageSource.isCreativePlayer()，
     *        DamageSource.java:98-100）。
     *
     * vanilla 实现：`this.getEntity() instanceof Player player && player.getAbilities().instabuild`，
     * 即伤害的造成者（causingEntity，对应 Cubium getEntity()/getTrueSource()）是创造模式玩家。
     *
     * 语义用途：Entity.isInvulnerableToBase:2920 的 invulnerable 守卫含 `!isCreativePlayer()`——
     * 创造模式玩家造成的伤害绕过实体的 invulnerable 标志（NBT Invulnerable）。例如末影龙复活
     * 仪式中基座末影水晶被 SpikeFeature 设为 invulnerable（普通玩家无法击毁），但创造玩家
     * vanilla 中应能直接击毁。Cubium 此前 isInvulnerableTo 的 invulnerable 分支漏此守卫，
     * 致创造玩家也无法伤害 invulnerable 实体，偏离 vanilla。
     *
     * 实现在 DamageSource.cpp（需 Player 完整定义做 dynamic_cast）。
     */
    [[nodiscard]] virtual bool isCreativePlayer() const;

    /**
     * @brief 是否是荆棘伤害
     */
    [[nodiscard]] virtual bool isThornsDamage() const { return false; }

    /**
     * @brief 获取饥饿消耗值
     * 玩家受伤时会消耗饱食度，默认 0.1，护甲穿透伤害为 0.0
     */
    [[nodiscard]] virtual f32 hungerDamage() const { return 0.1f; }

    /**
     * @brief 是否忽略药水效果和附魔
     */
    [[nodiscard]] virtual bool isDamageAbsolute() const { return false; }

    /**
     * @brief 是否是摔落伤害
     * 与 MC 1.21.11 DamageTypeTags.IS_FALL 标签保持一致：
     * fall, ender_pearl, stalagmite
     */
    [[nodiscard]] bool isFall() const
    {
        return type() == DamageType::Fall || type() == DamageType::EnderPearl || type() == DamageType::Stalagmite;
    }

    /**
     * @brief 是否是岩浆伤害
     */
    [[nodiscard]] bool isLava() const { return type() == DamageType::Lava; }

    /**
     * @brief 是否是仙人掌伤害
     */
    [[nodiscard]] bool isCactus() const { return type() == DamageType::Cactus; }

    /**
     * @brief 是否是饥饿伤害
     */
    [[nodiscard]] bool isStarve() const { return type() == DamageType::Starve; }

    /**
     * @brief 是否是溺水伤害
     */
    [[nodiscard]] bool isDrown() const { return type() == DamageType::Drown; }

    /**
     * @brief 是否是甜浆果丛伤害
     */
    [[nodiscard]] bool isSweetBerryBush() const { return type() == DamageType::SweetBerryBush; }

    /**
     * @brief 是否是冰冻伤害
     *
     * 冰冻伤害对冻结额外伤害标签中的实体（烈焰人、岩浆怪、炽足兽）造成5倍伤害。
     * 玩家的冰冻伤害可通过 freeze_damage 游戏规则禁用。
     */
    [[nodiscard]] virtual bool isFreezing() const { return type() == DamageType::Freeze; }

    /**
     * @brief 检查伤害类型是否属于指定标签
     *
     * 对应 MC 1.21.11 的 DamageSource.is(DamageTypeTag) 方法。
     * 例如 source.is(DamageTypeTags::BYPASSES_WOLF_ARMOR()) 判断伤害是否绕过狼铠。
     *
     * @param tag 伤害类型标签
     * @return 是否在标签中
     */
    [[nodiscard]] bool is(const DamageTypeTag& tag) const;

protected:
    DamageSource() = default;
};

/**
 * @brief 环境伤害来源
 *
 * 非实体造成的伤害，如火焰、摔落、溺水等。
 */
class EnvironmentalDamage : public DamageSource {
public:
    explicit EnvironmentalDamage(DamageType type)
        : m_type(type)
        , m_hungerDamage(0.1f)
        , m_isDamageAbsolute(false)
    {
        // bypassesArmor 的伤害类型饥饿消耗为 0
        if (bypassesArmor()) {
            m_hungerDamage = 0.0f;
        }
        // 虚空和饥饿伤害是绝对伤害（忽略药水/附魔）
        if (type == DamageType::OutOfWorld || type == DamageType::Starve) {
            m_isDamageAbsolute = true;
        }
    }

    [[nodiscard]] std::unique_ptr<DamageSource> clone() const override
    {
        return std::make_unique<EnvironmentalDamage>(m_type);
    }

    [[nodiscard]] DamageType type() const override { return m_type; }

    [[nodiscard]] bool bypassesArmor() const override
    {
        // 与 MC 1.21.11 DamageTypeTags.BYPASSES_ARMOR 标签保持一致：
        // on_fire, in_wall, cramming, drown, fly_into_wall, generic, wither,
        // dragon_breath, starve, fall, ender_pearl, freeze, stalagmite,
        // magic, indirect_magic, out_of_world, generic_kill, sonic_boom, outside_border
        // 注：indirect_magic 由 IndirectEntityDamageSource 通过 setBypassesArmor() 处理
        // 注：on_fire（着火状态伤害）绕过护甲——此前实现漏 DamageType::OnFire，致实体着火时
        //     on_fire 伤害被 applyArmorCalculations 错误减免护甲，偏离 vanilla（vanilla 中 on_fire
        //     仅由火焰保护附魔减免，盔甲本身不减）。已补 OnFire 对齐数据包 bypasses_armor.json 成员集。
        // TODO: 此处硬编码 DamageType 列表代标签查询，属扩展性偏差——数据包扩展 BYPASSES_ARMOR
        //       成员时此处失效。未来应改为查 DamageTypeTags::BYPASSES_ARMOR() 标签（对齐 isFire()/
        //       isProjectile() 的标签查询模式），标签未初始化时回退硬编码列表保底。
        return m_type == DamageType::OnFire || m_type == DamageType::OutOfWorld || m_type == DamageType::Starve ||
            m_type == DamageType::Drown || m_type == DamageType::Fall || m_type == DamageType::FlyIntoWall ||
            m_type == DamageType::InWall || m_type == DamageType::Cramming || m_type == DamageType::Generic ||
            m_type == DamageType::Magic || m_type == DamageType::Wither || m_type == DamageType::DragonBreath ||
            m_type == DamageType::Stalagmite || m_type == DamageType::Freeze || m_type == DamageType::EnderPearl ||
            m_type == DamageType::IndirectMagic || m_type == DamageType::GenericKill ||
            m_type == DamageType::SonicBoom || m_type == DamageType::OutsideBorder;
    }

    [[nodiscard]] bool bypassesInvulnerability() const override
    {
        // 与 MC 1.21.11 DamageTypeTags.BYPASSES_INVULNERABILITY 标签保持一致：
        // out_of_world, generic_kill
        return m_type == DamageType::OutOfWorld || m_type == DamageType::GenericKill;
    }

    [[nodiscard]] bool canDamageCreative() const override { return m_type == DamageType::OutOfWorld; }

    [[nodiscard]] bool isFire() const override
    {
        // 与 MC 1.21.11 DamageTypeTags.IS_FIRE 标签保持一致：
        // in_fire, campfire, on_fire, lava, hot_floor, unattributed_fireball, fireball
        // 注：fireball 和 unattributed_fireball 通常通过 EntityDamageSource/IndirectEntityDamageSource
        // 创建（由各自的 isFire() 覆写处理），但为保持一致性，
        // EnvironmentalDamage 也对这两种类型返回 true
        return m_type == DamageType::InFire || m_type == DamageType::Campfire || m_type == DamageType::OnFire ||
            m_type == DamageType::Lava || m_type == DamageType::HotFloor || m_type == DamageType::Fireball ||
            m_type == DamageType::UnattributedFireball;
    }

    [[nodiscard]] bool isMagic() const override
    {
        return m_type == DamageType::Magic || m_type == DamageType::Wither || m_type == DamageType::IndirectMagic;
    }

    [[nodiscard]] bool isExplosion() const override
    {
        // 与 MC 1.21.11 DamageTypeTags.IS_EXPLOSION 标签保持一致：
        // fireworks, explosion, player_explosion, bad_respawn_point
        // 注：此前实现遗漏 Fireworks，导致烟花爆炸伤害不被识别为爆炸——连带使末影龙爆炸免疫
        // （ALWAYS_HURTS_ENDER_DRAGONS 成员=#is_explosion 含 fireworks）失效、烟花不走爆炸保护附魔、
        // 末影水晶被烟花炸毁时误触二次爆炸等偏差。补 Fireworks 对齐 vanilla 成员集。
        return m_type == DamageType::Fireworks || m_type == DamageType::Explosion ||
            m_type == DamageType::ExplosionPlayer || m_type == DamageType::BadRespawnPoint;
    }

    [[nodiscard]] bool isFreezing() const override { return m_type == DamageType::Freeze; }

    [[nodiscard]] f32 hungerDamage() const override { return m_hungerDamage; }

    [[nodiscard]] bool isDamageAbsolute() const override { return m_isDamageAbsolute; }

    [[nodiscard]] std::string deathMessageKey() const override
    {
        switch (m_type) {
            case DamageType::InFire:
                return "death.attack.inFire";
            case DamageType::Campfire:
                return "death.attack.inFire";
            case DamageType::LightningBolt:
                return "death.attack.lightningBolt";
            case DamageType::OnFire:
                return "death.attack.onFire";
            case DamageType::Lava:
                return "death.attack.lava";
            case DamageType::HotFloor:
                return "death.attack.hotFloor";
            case DamageType::InWall:
                return "death.attack.inWall";
            case DamageType::Cramming:
                return "death.attack.cramming";
            case DamageType::Drown:
                return "death.attack.drown";
            case DamageType::Starve:
                return "death.attack.starve";
            case DamageType::Cactus:
                return "death.attack.cactus";
            case DamageType::Fall:
                return "death.attack.fall";
            case DamageType::EnderPearl:
                return "death.attack.fall";
            case DamageType::FlyIntoWall:
                return "death.attack.flyIntoWall";
            case DamageType::OutOfWorld:
                return "death.attack.outOfWorld";
            case DamageType::Generic:
                return "death.attack.generic";
            case DamageType::Magic:
                return "death.attack.magic";
            case DamageType::Wither:
                return "death.attack.wither";
            case DamageType::DragonBreath:
                return "death.attack.dragonBreath";
            case DamageType::Dryout:
                return "death.attack.dryout";
            case DamageType::SweetBerryBush:
                return "death.attack.sweetBerryBush";
            case DamageType::Freeze:
                return "death.attack.freeze";
            case DamageType::Stalagmite:
                return "death.attack.stalagmite";
            case DamageType::FallingBlock:
                return "death.attack.fallingBlock";
            case DamageType::FallingAnvil:
                return "death.attack.anvil";
            case DamageType::FallingStalactite:
                return "death.attack.fallingStalactite";
            case DamageType::WindBurst:
                return "death.attack.windBurst";
            case DamageType::MaceSmash:
                return "death.attack.mace_smash";
            case DamageType::SonicBoom:
                return "death.attack.sonic_boom";
            case DamageType::BadRespawnPoint:
                return "death.attack.badRespawnPoint";
            case DamageType::OutsideBorder:
                return "death.attack.outsideBorder";
            case DamageType::GenericKill:
                return "death.attack.genericKill";
            // 实体伤害类型的死亡消息键由 EntityDamageSource/IndirectEntityDamageSource 处理
            case DamageType::Sting:
            case DamageType::MobAttack:
            case DamageType::MobAttackNoAggro:
            case DamageType::PlayerAttack:
            case DamageType::Spear:
            case DamageType::Arrow:
            case DamageType::Trident:
            case DamageType::MobProjectile:
            case DamageType::Spit:
            case DamageType::Fireworks:
            case DamageType::UnattributedFireball:
            case DamageType::Fireball:
            case DamageType::WitherSkull:
            case DamageType::Thrown:
            case DamageType::IndirectMagic:
            case DamageType::Thorns:
            case DamageType::Explosion:
            case DamageType::ExplosionPlayer:
                return "death.attack.generic";
            default:
                return "death.attack.generic";
        }
    }

private:
    DamageType m_type;
    f32 m_hungerDamage;
    bool m_isDamageAbsolute;
};

/**
 * @brief 实体伤害来源
 *
 * 由实体造成的伤害，如生物攻击、玩家攻击等。
 */
class EntityDamageSource : public DamageSource {
public:
    EntityDamageSource(DamageType type, Entity* source)
        : m_type(type)
        , m_source(source)
        , m_isThornsDamage(false)
        , m_isMagic(false)
        , m_isExplosion(false)
    {}

    [[nodiscard]] std::unique_ptr<DamageSource> clone() const override
    {
        auto result = std::make_unique<EntityDamageSource>(m_type, m_source);
        result->m_isThornsDamage = m_isThornsDamage;
        result->m_isMagic = m_isMagic;
        result->m_isExplosion = m_isExplosion;
        return result;
    }

    [[nodiscard]] DamageType type() const override { return m_type; }

    [[nodiscard]] Entity* source() const override { return m_source; }
    [[nodiscard]] Entity* directSource() const override { return m_source; }
    [[nodiscard]] Entity* getTrueSource() const override { return m_source; }

    /// 直接来源实体位置（EntityDamageSource 回退 directEntity.position()）。
    [[nodiscard]] std::optional<math::Vector3f> sourcePosition() const override;

    [[nodiscard]] bool isFire() const override { return m_type == DamageType::Fireball; }

    // isProjectile 查 DamageTypeTags::IS_PROJECTILE 标签（对齐 vanilla source.is(IS_PROJECTILE)），
    // 实现在 DamageSource.cpp（需 DamageTypeTags 完整定义）。此前硬编码 Arrow/Trident/MobProjectile/
    // Fireball 四类型是"硬编码代标签"缺陷变体——IS_PROJECTILE 标签成员集还含 WitherSkull/Thrown/
    // WindBurst/UnattributedFireball，硬编码漏掉它们致对应伤害源 isProjectile() 返 false。
    [[nodiscard]] bool isProjectile() const override;

    [[nodiscard]] bool isExplosion() const override { return m_isExplosion; }

    [[nodiscard]] bool isMagic() const override { return m_isMagic; }

    [[nodiscard]] bool isEntitySource() const override { return true; }

    [[nodiscard]] bool isPlayerSource() const override { return m_type == DamageType::PlayerAttack; }

    [[nodiscard]] bool isThornsDamage() const override { return m_isThornsDamage; }

    /**
     * @brief 设置为荆棘伤害
     */
    EntityDamageSource& setThornsDamage()
    {
        m_isThornsDamage = true;
        return *this;
    }

    /**
     * @brief 设置为魔法伤害
     */
    EntityDamageSource& setMagicDamage()
    {
        m_isMagic = true;
        return *this;
    }

    /**
     * @brief 设置为爆炸伤害
     */
    EntityDamageSource& setExplosion()
    {
        m_isExplosion = true;
        return *this;
    }

    [[nodiscard]] std::string deathMessageKey() const override
    {
        switch (m_type) {
            case DamageType::MobAttack:
            case DamageType::MobAttackNoAggro:
                return "death.attack.mob";
            case DamageType::PlayerAttack:
                return "death.attack.player";
            case DamageType::Arrow:
                return "death.attack.arrow";
            case DamageType::Trident:
                return "death.attack.trident";
            case DamageType::MobProjectile:
            case DamageType::Spit:
                return "death.attack.mobProjectile";
            case DamageType::Fireball:
            case DamageType::UnattributedFireball:
                return "death.attack.fireball";
            case DamageType::WitherSkull:
                return "death.attack.witherSkull";
            case DamageType::Thrown:
                return "death.attack.thrown";
            case DamageType::Thorns:
                return "death.attack.thorns";
            case DamageType::Explosion:
                return "death.attack.explosion";
            case DamageType::ExplosionPlayer:
                return "death.attack.explosion.player";
            case DamageType::Sting:
                return "death.attack.sting";
            case DamageType::Spear:
                return "death.attack.spear";
            case DamageType::SonicBoom:
                return "death.attack.sonic_boom";
            case DamageType::BadRespawnPoint:
                return "death.attack.badRespawnPoint";
            case DamageType::Fireworks:
                return "death.attack.fireworks";
            case DamageType::LightningBolt:
                return "death.attack.lightningBolt";
            default:
                return "death.attack.generic";
        }
    }

protected:
    DamageType m_type;
    Entity* m_source;
    bool m_isThornsDamage;
    bool m_isMagic;
    bool m_isExplosion;
};

/**
 * @brief 间接实体伤害来源
 *
 * 由实体间接造成的伤害，如箭矢（由弓射出）、药水等。
 */
class IndirectEntityDamageSource : public DamageSource {
public:
    IndirectEntityDamageSource(DamageType type, Entity* source, Entity* directSource, bool isPlayer = false)
        : m_type(type)
        , m_source(source)
        , m_directSource(directSource)
        , m_isPlayer(isPlayer)
        , m_isProjectile(false)
        , m_isFire(false)
        , m_isExplosion(false)
        , m_isMagic(false)
    {}

    [[nodiscard]] std::unique_ptr<DamageSource> clone() const override
    {
        auto result = std::make_unique<IndirectEntityDamageSource>(m_type, m_source, m_directSource, m_isPlayer);
        result->m_isProjectile = m_isProjectile;
        result->m_isFire = m_isFire;
        result->m_isExplosion = m_isExplosion;
        result->m_isMagic = m_isMagic;
        return result;
    }

    [[nodiscard]] DamageType type() const override { return m_type; }

    [[nodiscard]] Entity* source() const override { return m_source; }
    [[nodiscard]] Entity* directSource() const override { return m_directSource; }
    [[nodiscard]] Entity* getEntity() const override { return m_source; }

    /// 直接来源（投射物本身）位置（IndirectEntityDamageSource 回退 directEntity.position()）。
    [[nodiscard]] std::optional<math::Vector3f> sourcePosition() const override;

    /**
     * @brief 获取真正的伤害来源
     * 返回间接来源（射击者）
     */
    [[nodiscard]] Entity* getTrueSource() const override { return m_source; }

    [[nodiscard]] bool isFire() const override
    {
        // 与 MC 1.21.11 DamageTypeTags.IS_FIRE 标签保持一致：
        // fireball 和 unattributed_fireball 始终是火焰伤害
        // m_isFire 标志允许其他类型（如 ExplosionPlayer 通过 setFireDamage）标记为火焰
        return m_isFire || m_type == DamageType::Fireball || m_type == DamageType::UnattributedFireball;
    }

    [[nodiscard]] bool isProjectile() const override;

    [[nodiscard]] bool isExplosion() const override { return m_isExplosion; }

    [[nodiscard]] bool isMagic() const override { return m_isMagic; }

    [[nodiscard]] bool isEntitySource() const override { return true; }

    [[nodiscard]] bool isPlayerSource() const override { return m_isPlayer; }

    [[nodiscard]] bool bypassesArmor() const override { return m_bypassesArmor; }

    /**
     * @brief 设置为投射物伤害
     */
    IndirectEntityDamageSource& setProjectile()
    {
        m_isProjectile = true;
        return *this;
    }

    /**
     * @brief 设置为火焰伤害
     */
    IndirectEntityDamageSource& setFireDamage()
    {
        m_isFire = true;
        return *this;
    }

    /**
     * @brief 设置为爆炸伤害
     */
    IndirectEntityDamageSource& setExplosion()
    {
        m_isExplosion = true;
        return *this;
    }

    /**
     * @brief 设置为魔法伤害
     */
    IndirectEntityDamageSource& setMagicDamage()
    {
        m_isMagic = true;
        return *this;
    }

    /**
     * @brief 设置绕过护甲
     */
    IndirectEntityDamageSource& setBypassesArmor()
    {
        m_bypassesArmor = true;
        return *this;
    }

    [[nodiscard]] std::string deathMessageKey() const override
    {
        switch (m_type) {
            case DamageType::Arrow:
                return "death.attack.arrow.item";
            case DamageType::Trident:
                return "death.attack.trident.item";
            case DamageType::Fireball:
            case DamageType::UnattributedFireball:
                return "death.attack.fireball.item";
            case DamageType::WitherSkull:
                return "death.attack.witherSkull.item";
            case DamageType::Thrown:
                return "death.attack.thrown.item";
            case DamageType::IndirectMagic:
                return "death.attack.indirectMagic";
            case DamageType::SonicBoom:
                return "death.attack.sonic_boom.item";
            case DamageType::BadRespawnPoint:
                return "death.attack.badRespawnPoint.link";
            default:
                return "death.attack.generic";
        }
    }

private:
    DamageType m_type;
    Entity* m_source;       // 伤害来源（如射箭的玩家）
    Entity* m_directSource; // 直接来源（如箭矢实体）
    bool m_isPlayer;        // 是否来自玩家
    bool m_isProjectile;
    bool m_isFire;
    bool m_isExplosion;
    bool m_isMagic;
    bool m_bypassesArmor = false;
};

// ============================================================================
// 伤害来源工厂函数
// ============================================================================

namespace DamageSources {

/** 创建火焰伤害 */
inline EnvironmentalDamage inFire()
{
    return EnvironmentalDamage(DamageType::InFire);
}

/** 创建燃烧伤害 */
inline EnvironmentalDamage onFire()
{
    return EnvironmentalDamage(DamageType::OnFire);
}

/** 创建岩浆伤害 */
inline EnvironmentalDamage lava()
{
    return EnvironmentalDamage(DamageType::Lava);
}

/** 创建溺水伤害 */
inline EnvironmentalDamage drown()
{
    return EnvironmentalDamage(DamageType::Drown);
}

/** 创建饥饿伤害 */
inline EnvironmentalDamage starve()
{
    return EnvironmentalDamage(DamageType::Starve);
}

/** 创建仙人掌伤害 */
inline EnvironmentalDamage cactus()
{
    return EnvironmentalDamage(DamageType::Cactus);
}

/** 创建烫脚伤害（踩在岩浆块上） */
inline EnvironmentalDamage hotFloor()
{
    return EnvironmentalDamage(DamageType::HotFloor);
}

/** 创建摔落伤害 */
inline EnvironmentalDamage fall()
{
    return EnvironmentalDamage(DamageType::Fall);
}

/** 创建撞墙伤害 */
inline EnvironmentalDamage flyIntoWall()
{
    return EnvironmentalDamage(DamageType::FlyIntoWall);
}

/** 创建虚空伤害 */
inline EnvironmentalDamage outOfWorld()
{
    return EnvironmentalDamage(DamageType::OutOfWorld);
}

/** 创建通用伤害 */
inline EnvironmentalDamage generic()
{
    return EnvironmentalDamage(DamageType::Generic);
}

/** 创建魔法伤害 */
inline EnvironmentalDamage magic()
{
    return EnvironmentalDamage(DamageType::Magic);
}

/** 创建凋零伤害 */
inline EnvironmentalDamage wither()
{
    return EnvironmentalDamage(DamageType::Wither);
}

/**
 * @brief 创建生物攻击伤害
 * 生物攻击受难度缩放
 */
inline EntityDamageSource mobAttack(Entity* mob)
{
    return EntityDamageSource(DamageType::MobAttack, mob);
}

/**
 * @brief 创建玩家攻击伤害
 * 玩家攻击不受难度缩放
 */
inline EntityDamageSource playerAttack(Entity* player)
{
    return EntityDamageSource(DamageType::PlayerAttack, player);
}

/**
 * @brief 创建箭矢伤害
 * 箭矢是投射物
 */
inline IndirectEntityDamageSource arrow(Entity* arrow, Entity* shooter, bool isPlayer = false)
{
    return IndirectEntityDamageSource(DamageType::Arrow, shooter, arrow, isPlayer).setProjectile();
}

/**
 * @brief 创建三叉戟伤害
 * 三叉戟是投射物
 */
inline IndirectEntityDamageSource trident(Entity* trident, Entity* thrower, bool isPlayer = false)
{
    return IndirectEntityDamageSource(DamageType::Trident, thrower, trident, isPlayer).setProjectile();
}

/**
 * @brief 创建荆棘伤害
 * 荆棘伤害是魔法伤害
 */
inline EntityDamageSource thorns(Entity* owner)
{
    return EntityDamageSource(DamageType::Thorns, owner).setThornsDamage().setMagicDamage();
}

/**
 * @brief 创建爆炸伤害（无来源）
 * 爆炸伤害受难度缩放
 */
inline EnvironmentalDamage explosion()
{
    return EnvironmentalDamage(DamageType::Explosion);
}

/**
 * @brief 创建实体爆炸伤害（由非玩家实体引起的爆炸，如末影水晶爆炸）
 * @param source 爆炸来源实体（如末影水晶）
 */
inline EntityDamageSource explosion(Entity* source)
{
    return EntityDamageSource(DamageType::Explosion, source).setExplosion();
}

/**
 * @brief 创建实体爆炸伤害（带来源实体和造成者，如末影水晶被玩家破坏后的爆炸）
 * @param source 爆炸来源实体（如末影水晶）
 * @param cause 爆炸造成者实体（如玩家），如果为 nullptr 则使用 source 作为造成者
 */
inline IndirectEntityDamageSource explosion(Entity* source, Entity* cause)
{
    return IndirectEntityDamageSource(DamageType::Explosion, cause, source).setExplosion();
}

/**
 * @brief 创建实体爆炸伤害
 * 玩家爆炸伤害使用 explosion.player
 */
inline EntityDamageSource explosionPlayer(Entity* player)
{
    return EntityDamageSource(DamageType::ExplosionPlayer, player).setExplosion();
}

/** 创建窒息伤害（在方块内） */
inline EnvironmentalDamage inWall()
{
    return EnvironmentalDamage(DamageType::InWall);
}

/** 创建拥挤伤害 */
inline EnvironmentalDamage cramming()
{
    return EnvironmentalDamage(DamageType::Cramming);
}

/** 创建干涸伤害 */
inline EnvironmentalDamage dryout()
{
    return EnvironmentalDamage(DamageType::Dryout);
}

/** 创建闪电伤害 */
inline EntityDamageSource lightningBolt(Entity* lightning)
{
    return EntityDamageSource(DamageType::LightningBolt, lightning);
}

/** 创建甜浆果丛伤害 */
inline EnvironmentalDamage sweetBerryBush()
{
    return EnvironmentalDamage(DamageType::SweetBerryBush);
}

/** 创建冰冻伤害（细雪冰冻） */
inline EnvironmentalDamage freeze()
{
    return EnvironmentalDamage(DamageType::Freeze);
}

/**
 * @brief 创建蜜蜂蛰刺伤害
 * 蜜蜂蛰刺受难度缩放
 */
inline EntityDamageSource sting(Entity* bee)
{
    return EntityDamageSource(DamageType::Sting, bee);
}

/** 创建坠落铁砧伤害 */
inline EnvironmentalDamage anvil()
{
    return EnvironmentalDamage(DamageType::FallingAnvil);
}

/** 创建坠落铁砧伤害（与 anvil() 同义，对齐 MC 1.21.11 命名） */
inline EnvironmentalDamage fallingAnvil()
{
    return EnvironmentalDamage(DamageType::FallingAnvil);
}

/** 创建坠落方块伤害 */
inline EnvironmentalDamage fallingBlock()
{
    return EnvironmentalDamage(DamageType::FallingBlock);
}

/** 创建石笋摔落伤害（踩在朝上的滴石尖端上） */
inline EnvironmentalDamage stalagmite()
{
    return EnvironmentalDamage(DamageType::Stalagmite);
}

/** 创建坠落钟乳石伤害 */
inline EntityDamageSource fallingStalactite(Entity* stalactite)
{
    return EntityDamageSource(DamageType::FallingStalactite, stalactite);
}

/** 创建龙息伤害 */
inline EnvironmentalDamage dragonBreath()
{
    return EnvironmentalDamage(DamageType::DragonBreath);
}

/** 创建烟花伤害 */
inline EnvironmentalDamage fireworks()
{
    return EnvironmentalDamage(DamageType::Fireworks);
}

/**
 * @brief 创建投射物伤害
 * 投射物受难度缩放
 */
inline IndirectEntityDamageSource mobProjectile(Entity* projectile, Entity* shooter)
{
    return IndirectEntityDamageSource(DamageType::MobProjectile, shooter, projectile).setProjectile();
}

/**
 * @brief 创建火球伤害
 * 火球是投射物和火焰伤害
 */
inline IndirectEntityDamageSource fireball(Entity* fireball, Entity* shooter, bool isPlayer = false)
{
    return IndirectEntityDamageSource(DamageType::Fireball, shooter, fireball, isPlayer)
        .setProjectile()
        .setFireDamage();
}

/**
 * @brief 创建间接魔法伤害
 * 间接魔法伤害绕过护甲
 */
inline IndirectEntityDamageSource indirectMagic(Entity* source, Entity* caster)
{
    return IndirectEntityDamageSource(DamageType::IndirectMagic, caster, source).setBypassesArmor().setMagicDamage();
}

/**
 * @brief 创建风爆伤害
 * 风弹命中实体时造成的伤害。
 * 风爆伤害由风弹弹射物间接造成，需要追踪发射者。
 * 与 MC 1.21.11 一致：风爆伤害不绕过护甲（不在 DamageTypeTags::BYPASSES_ARMOR 中）。
 *
 * @param windCharge 风弹弹射物实体
 * @param shooter 发射者（玩家或旋风人），可能为空
 * @param isPlayer 发射者是否为玩家
 */
inline IndirectEntityDamageSource windBurst(Entity* windCharge, Entity* shooter, bool isPlayer = false)
{
    // WindBurst（风弹命中）在 DamageTypeTags::IS_PROJECTILE 标签内，是投射物伤害。
    // setProjectile 设 m_isProjectile 标志位作 isProjectile() 的保底（标签未初始化时回退），
    // 与 arrow/trident/fireball/spit/witherSkull/thrown 等投射物工厂一致。
    // 此前漏调 setProjectile，且 isProjectile 旧实现只查 m_isProjectile 标志（不查标签），
    // 致风爆伤害 isProjectile() 返 false、弹射物保护附魔 EPF 不设 PROJECTILE 位失效。
    return IndirectEntityDamageSource(DamageType::WindBurst, shooter, windCharge, isPlayer).setProjectile();
}

/**
 * @brief 创建重锤砸地攻击伤害
 *
 * 当重锤下落攻击（Smash Attack）触发时使用此伤害类型。
 * exhaustion 系数为 0.1。
 *
 * @param attacker 攻击者（玩家）
 */
inline EntityDamageSource maceSmash(Entity* attacker)
{
    return EntityDamageSource(DamageType::MaceSmash, attacker);
}

// ============================================================================
// 新增伤害源工厂函数（对齐 MC 1.21.11 DamageTypes 注册表）
// ============================================================================

/** 创建营火伤害（属于火焰伤害） */
inline EnvironmentalDamage campfire()
{
    return EnvironmentalDamage(DamageType::Campfire);
}

/** 创建末影珍珠摔落伤害（属于摔落伤害，绕过护甲） */
inline EnvironmentalDamage enderPearl()
{
    return EnvironmentalDamage(DamageType::EnderPearl);
}

/**
 * @brief 创建生物攻击伤害（不激怒目标）
 * 用于铁傀儡等生物的攻击，不会激怒目标生物
 */
inline EntityDamageSource mobAttackNoAggro(Entity* mob)
{
    return EntityDamageSource(DamageType::MobAttackNoAggro, mob);
}

/**
 * @brief 创建矛（三叉戟近战）伤害
 * 三叉戟近战攻击使用此伤害类型，受难度缩放
 */
inline EntityDamageSource spear(Entity* attacker)
{
    return EntityDamageSource(DamageType::Spear, attacker);
}

/**
 * @brief 创建羊驼喷吐伤害
 * 羊驼喷吐物命中实体时造成的间接伤害，受难度缩放
 */
inline IndirectEntityDamageSource spit(Entity* spitEntity, Entity* shooter)
{
    return IndirectEntityDamageSource(DamageType::Spit, shooter, spitEntity).setProjectile();
}

/**
 * @brief 创建无归属火球伤害
 * 用于无射击者的火球（如恶魂火球被偏转后）造成的伤害
 */
inline IndirectEntityDamageSource unattributedFireball(Entity* fireball, Entity* shooter, bool isPlayer = false)
{
    return IndirectEntityDamageSource(DamageType::UnattributedFireball, shooter, fireball, isPlayer)
        .setProjectile()
        .setFireDamage();
}

/**
 * @brief 创建凋灵之首伤害
 * 凋灵发射的凋灵之首命中实体时造成的伤害
 */
inline IndirectEntityDamageSource witherSkull(Entity* skull, Entity* shooter)
{
    return IndirectEntityDamageSource(DamageType::WitherSkull, shooter, skull).setProjectile();
}

/**
 * @brief 创建通用投掷伤害
 * 雪球、鸡蛋、末影珍珠等通用投掷物命中实体时造成的伤害（通常为0）
 */
inline IndirectEntityDamageSource thrown(Entity* projectile, Entity* thrower)
{
    return IndirectEntityDamageSource(DamageType::Thrown, thrower, projectile).setProjectile();
}

/**
 * @brief 创建音爆伤害
 * 监守者音爆攻击造成的伤害，绕过护甲和附魔
 */
inline IndirectEntityDamageSource sonicBoom(Entity* guardian, Entity* target)
{
    return IndirectEntityDamageSource(DamageType::SonicBoom, guardian, target).setBypassesArmor();
}

/** 创建床重生爆炸伤害（在下界或末地使用床时） */
inline EnvironmentalDamage badRespawnPoint()
{
    return EnvironmentalDamage(DamageType::BadRespawnPoint);
}

/** 创建世界边界外伤害 */
inline EnvironmentalDamage outsideBorder()
{
    return EnvironmentalDamage(DamageType::OutsideBorder);
}

/** 创建通用击杀伤害（/kill 命令使用，绕过无敌） */
inline EnvironmentalDamage genericKill()
{
    return EnvironmentalDamage(DamageType::GenericKill);
}

} // namespace DamageSources

} // namespace mc
