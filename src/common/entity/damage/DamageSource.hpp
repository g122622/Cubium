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
#include <memory>
#include <string>

namespace mc {

// 前向声明
class Entity;
class LivingEntity;

// ============================================================================
// 保护附魔伤害类型标志位
// 用于 ProtectionEnchantment 计算伤害减免
// 参考 MC 1.16.5 ProtectionEnchantment
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
 * 参考 MC 1.16.5 DamageSource。
 */
enum class DamageType : u8 {
    // 环境伤害
    InFire,       // 在火焰中
    OnFire,       // 燃烧
    Lava,         // 岩浆
    HotFloor,     // 岩浆块
    Drown,        // 溺水
    Starve,       // 饥饿
    Cactus,       // 仙人掌
    Fall,         // 摔落
    FlyIntoWall,  // 撞墙（鞘翅飞行）
    OutOfWorld,   // 虚空
    Generic,      // 通用伤害
    Magic,        // 魔法伤害
    Wither,       // 凋零
    Anvil,        // 铁砧
    FallingBlock, // 坠落方块
    DragonBreath, // 龙息
    Fireworks,    // 烟花

    // 新增环境伤害类型（MC 1.16.5）
    InWall,         // 窒息（在方块内）
    Cramming,       // 拥挤伤害（实体过多）
    Dryout,         // 干涸伤害（鱼离开水）
    LightningBolt,  // 闪电
    SweetBerryBush, // 甜浆果丛

    // 实体伤害
    MobAttack,       // 生物攻击
    PlayerAttack,    // 玩家攻击
    Arrow,           // 箭矢
    Trident,         // 三叉戟
    MobProjectile,   // 生物投射物
    Fireball,        // 火球
    Thorns,          // 荆棘
    Explosion,       // 爆炸
    ExplosionPlayer, // 玩家爆炸

    // 新增实体伤害类型（MC 1.16.5）
    Sting, // 蜜蜂蛰刺
};

/**
 * @brief 伤害来源基类
 *
 * 定义伤害的来源和类型，用于计算伤害、死亡消息等。
 *
 * 参考 MC 1.16.5 DamageSource
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
     * MC 1.16.5: DamageSource.getTrueSource()
     * 返回造成伤害的实体。这是 Target Goals (如 HurtByTargetGoal) 使用的接口。
     * 默认实现返回 getEntity()。
     */
    [[nodiscard]] virtual Entity* getTrueSource() const { return getEntity(); }

    /**
     * @brief 是否可以绕过护甲
     */
    [[nodiscard]] virtual bool bypassesArmor() const { return false; }

    /**
     * @brief 是否可以绕过无敌
     * MC 1.16.5: isDamageAbsolute() - 忽略药水效果和附魔
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
     * MC 1.16.5: isDifficultyScaled()
     */
    [[nodiscard]] virtual bool isDifficultyScaled() const { return false; }

    /**
     * @brief 是否是荆棘伤害
     * MC 1.16.5: getIsThornsDamage()
     */
    [[nodiscard]] virtual bool isThornsDamage() const { return false; }

    /**
     * @brief 获取饥饿消耗值
     * MC 1.16.5: getHungerDamage()
     * 玩家受伤时会消耗饱食度，默认 0.1，护甲穿透伤害为 0.0
     */
    [[nodiscard]] virtual f32 hungerDamage() const { return 0.1f; }

    /**
     * @brief 是否忽略药水效果和附魔
     * MC 1.16.5: isDamageAbsolute()
     */
    [[nodiscard]] virtual bool isDamageAbsolute() const { return false; }

    /**
     * @brief 是否是摔落伤害
     */
    [[nodiscard]] bool isFall() const { return type() == DamageType::Fall || type() == DamageType::FlyIntoWall; }

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
        // MC 1.16.5: bypassesArmor 的伤害类型饥饿消耗为 0
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
        return m_type == DamageType::OutOfWorld || m_type == DamageType::Starve || m_type == DamageType::Drown ||
            m_type == DamageType::Fall || m_type == DamageType::FlyIntoWall || m_type == DamageType::InWall ||
            m_type == DamageType::Cramming || m_type == DamageType::Generic || m_type == DamageType::Magic ||
            m_type == DamageType::Wither || m_type == DamageType::DragonBreath;
    }

    [[nodiscard]] bool bypassesInvulnerability() const override { return m_type == DamageType::OutOfWorld; }

    [[nodiscard]] bool canDamageCreative() const override { return m_type == DamageType::OutOfWorld; }

    [[nodiscard]] bool isFire() const override
    {
        return m_type == DamageType::InFire || m_type == DamageType::OnFire || m_type == DamageType::Lava ||
            m_type == DamageType::HotFloor;
    }

    [[nodiscard]] bool isMagic() const override { return m_type == DamageType::Magic || m_type == DamageType::Wither; }

    [[nodiscard]] f32 hungerDamage() const override { return m_hungerDamage; }

    [[nodiscard]] bool isDamageAbsolute() const override { return m_isDamageAbsolute; }

    [[nodiscard]] std::string deathMessageKey() const override
    {
        switch (m_type) {
            case DamageType::InFire:
                return "death.attack.inFire";
            case DamageType::OnFire:
                return "death.attack.onFire";
            case DamageType::Lava:
                return "death.attack.lava";
            case DamageType::HotFloor:
                return "death.attack.hotFloor";
            case DamageType::Drown:
                return "death.attack.drown";
            case DamageType::Starve:
                return "death.attack.starve";
            case DamageType::Cactus:
                return "death.attack.cactus";
            case DamageType::Fall:
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
            case DamageType::Anvil:
                return "death.attack.anvil";
            case DamageType::FallingBlock:
                return "death.attack.fallingBlock";
            case DamageType::DragonBreath:
                return "death.attack.dragonBreath";
            case DamageType::Fireworks:
                return "death.attack.fireworks";
            case DamageType::InWall:
                return "death.attack.inWall";
            case DamageType::Cramming:
                return "death.attack.cramming";
            case DamageType::Dryout:
                return "death.attack.dryout";
            case DamageType::LightningBolt:
                return "death.attack.lightningBolt";
            case DamageType::SweetBerryBush:
                return "death.attack.sweetBerryBush";
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
        , m_difficultyScaled(false)
        , m_isMagic(false)
        , m_isExplosion(false)
    {}

    [[nodiscard]] std::unique_ptr<DamageSource> clone() const override
    {
        auto result = std::make_unique<EntityDamageSource>(m_type, m_source);
        result->m_isThornsDamage = m_isThornsDamage;
        result->m_difficultyScaled = m_difficultyScaled;
        result->m_isMagic = m_isMagic;
        result->m_isExplosion = m_isExplosion;
        return result;
    }

    [[nodiscard]] DamageType type() const override { return m_type; }

    [[nodiscard]] Entity* source() const override { return m_source; }
    [[nodiscard]] Entity* directSource() const override { return m_source; }
    [[nodiscard]] Entity* getTrueSource() const override { return m_source; }

    [[nodiscard]] bool isFire() const override { return m_type == DamageType::Fireball; }

    [[nodiscard]] bool isProjectile() const override
    {
        return m_type == DamageType::Arrow || m_type == DamageType::Trident || m_type == DamageType::MobProjectile ||
            m_type == DamageType::Fireball;
    }

    [[nodiscard]] bool isExplosion() const override { return m_isExplosion; }

    [[nodiscard]] bool isMagic() const override { return m_isMagic; }

    [[nodiscard]] bool isEntitySource() const override { return true; }

    [[nodiscard]] bool isPlayerSource() const override { return m_type == DamageType::PlayerAttack; }

    /**
     * @brief 是否受难度缩放
     * MC 1.16.5: 非玩家生物攻击受难度缩放
     */
    [[nodiscard]] bool isDifficultyScaled() const override
    {
        // MC 1.16.5: 非玩家的 LivingEntity 攻击受难度缩放
        return m_difficultyScaled;
    }

    [[nodiscard]] bool isThornsDamage() const override { return m_isThornsDamage; }

    /**
     * @brief 设置为荆棘伤害
     * MC 1.16.5: setIsThornsDamage()
     */
    EntityDamageSource& setThornsDamage()
    {
        m_isThornsDamage = true;
        return *this;
    }

    /**
     * @brief 设置受难度缩放
     * MC 1.16.5: setDifficultyScaled()
     */
    EntityDamageSource& setDifficultyScaled()
    {
        m_difficultyScaled = true;
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
                return "death.attack.mob";
            case DamageType::PlayerAttack:
                return "death.attack.player";
            case DamageType::Arrow:
                return "death.attack.arrow";
            case DamageType::Trident:
                return "death.attack.trident";
            case DamageType::MobProjectile:
                return "death.attack.mobProjectile";
            case DamageType::Fireball:
                return "death.attack.fireball";
            case DamageType::Thorns:
                return "death.attack.thorns";
            case DamageType::Explosion:
                return "death.attack.explosion";
            case DamageType::ExplosionPlayer:
                return "death.attack.explosion.player";
            case DamageType::Sting:
                return "death.attack.sting";
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
    bool m_difficultyScaled;
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
        , m_difficultyScaled(false)
    {}

    [[nodiscard]] std::unique_ptr<DamageSource> clone() const override
    {
        auto result = std::make_unique<IndirectEntityDamageSource>(m_type, m_source, m_directSource, m_isPlayer);
        result->m_isProjectile = m_isProjectile;
        result->m_isFire = m_isFire;
        result->m_isExplosion = m_isExplosion;
        result->m_isMagic = m_isMagic;
        result->m_difficultyScaled = m_difficultyScaled;
        return result;
    }

    [[nodiscard]] DamageType type() const override { return m_type; }

    [[nodiscard]] Entity* source() const override { return m_source; }
    [[nodiscard]] Entity* directSource() const override { return m_directSource; }
    [[nodiscard]] Entity* getEntity() const override { return m_source; }

    /**
     * @brief 获取真正的伤害来源
     * MC 1.16.5: getTrueSource() 返回间接来源（射击者）
     */
    [[nodiscard]] Entity* getTrueSource() const override { return m_source; }

    [[nodiscard]] bool isFire() const override { return m_isFire; }

    [[nodiscard]] bool isProjectile() const override { return m_isProjectile; }

    [[nodiscard]] bool isExplosion() const override { return m_isExplosion; }

    [[nodiscard]] bool isMagic() const override { return m_isMagic; }

    [[nodiscard]] bool isEntitySource() const override { return true; }

    [[nodiscard]] bool isPlayerSource() const override { return m_isPlayer; }

    [[nodiscard]] bool bypassesArmor() const override { return m_bypassesArmor; }

    [[nodiscard]] bool isDifficultyScaled() const override { return m_difficultyScaled; }

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

    /**
     * @brief 设置受难度缩放
     */
    IndirectEntityDamageSource& setDifficultyScaled()
    {
        m_difficultyScaled = true;
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
                return "death.attack.fireball.item";
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
    bool m_difficultyScaled;
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
 * MC 1.16.5: 生物攻击受难度缩放
 */
inline EntityDamageSource mobAttack(Entity* mob)
{
    return EntityDamageSource(DamageType::MobAttack, mob).setDifficultyScaled();
}

/**
 * @brief 创建玩家攻击伤害
 * MC 1.16.5: 玩家攻击不受难度缩放
 */
inline EntityDamageSource playerAttack(Entity* player)
{
    return EntityDamageSource(DamageType::PlayerAttack, player);
}

/**
 * @brief 创建箭矢伤害
 * MC 1.16.5: 箭矢是投射物
 */
inline IndirectEntityDamageSource arrow(Entity* arrow, Entity* shooter, bool isPlayer = false)
{
    return IndirectEntityDamageSource(DamageType::Arrow, shooter, arrow, isPlayer).setProjectile();
}

/**
 * @brief 创建三叉戟伤害
 * MC 1.16.5: 三叉戟是投射物
 */
inline IndirectEntityDamageSource trident(Entity* trident, Entity* thrower, bool isPlayer = false)
{
    return IndirectEntityDamageSource(DamageType::Trident, thrower, trident, isPlayer).setProjectile();
}

/**
 * @brief 创建荆棘伤害
 * MC 1.16.5: 荆棘伤害是魔法伤害
 */
inline EntityDamageSource thorns(Entity* owner)
{
    return EntityDamageSource(DamageType::Thorns, owner).setThornsDamage().setMagicDamage();
}

/**
 * @brief 创建爆炸伤害（无来源）
 * MC 1.16.5: 爆炸伤害受难度缩放
 */
inline EnvironmentalDamage explosion()
{
    return EnvironmentalDamage(DamageType::Explosion);
}

/**
 * @brief 创建实体爆炸伤害
 * MC 1.16.5: 玩家爆炸伤害使用 explosion.player
 */
inline EntityDamageSource explosionPlayer(Entity* player)
{
    return EntityDamageSource(DamageType::ExplosionPlayer, player).setDifficultyScaled().setExplosion();
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

/**
 * @brief 创建蜜蜂蛰刺伤害
 * MC 1.16.5: 蜜蜂蛰刺受难度缩放
 */
inline EntityDamageSource sting(Entity* bee)
{
    return EntityDamageSource(DamageType::Sting, bee).setDifficultyScaled();
}

/** 创建铁砧伤害 */
inline EnvironmentalDamage anvil()
{
    return EnvironmentalDamage(DamageType::Anvil);
}

/** 创建坠落方块伤害 */
inline EnvironmentalDamage fallingBlock()
{
    return EnvironmentalDamage(DamageType::FallingBlock);
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
 * MC 1.16.5: 投射物受难度缩放
 */
inline IndirectEntityDamageSource mobProjectile(Entity* projectile, Entity* shooter)
{
    return IndirectEntityDamageSource(DamageType::MobProjectile, shooter, projectile)
        .setProjectile()
        .setDifficultyScaled();
}

/**
 * @brief 创建火球伤害
 * MC 1.16.5: 火球是投射物和火焰伤害
 */
inline IndirectEntityDamageSource fireball(Entity* fireball, Entity* shooter, bool isPlayer = false)
{
    return IndirectEntityDamageSource(DamageType::Fireball, shooter, fireball, isPlayer)
        .setProjectile()
        .setFireDamage();
}

/**
 * @brief 创建间接魔法伤害
 * MC 1.16.5: 间接魔法伤害绕过护甲
 */
inline IndirectEntityDamageSource indirectMagic(Entity* source, Entity* caster)
{
    return IndirectEntityDamageSource(DamageType::Magic, caster, source).setBypassesArmor().setMagicDamage();
}

} // namespace DamageSources

} // namespace mc
