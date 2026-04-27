#pragma once

#include "../../core/Types.hpp"
#include <memory>
#include <string>

namespace mc {

// 前向声明
class Entity;
class LivingEntity;

/**
 * @brief 伤害类型枚举
 *
 * 定义不同类型的伤害来源
 */
enum class DamageType : u8 {
    // 环境伤害
    InFire,         // 在火焰中
    OnFire,         // 燃烧
    Lava,           // 岩浆
    HotFloor,       // 岩浆块
    Drown,          // 溺水
    Starve,         // 饥饿
    Cactus,         // 仙人掌
    Fall,           // 摔落
    FlyIntoWall,    // 撞墙（鞘翅飞行）
    OutOfWorld,     // 虚空
    Generic,        // 通用伤害
    Magic,          // 魔法伤害
    Wither,         // 凋零
    Anvil,          // 铁砧
    FallingBlock,   // 坠落方块
    DragonBreath,   // 龙息
    Fireworks,      // 烟花

    // 实体伤害
    MobAttack,      // 生物攻击
    PlayerAttack,   // 玩家攻击
    Arrow,          // 箭矢
    Trident,        // 三叉戟
    MobProjectile,  // 生物投射物
    Fireball,       // 火球
    Thorns,         // 荆棘
    Explosion,      // 爆炸
    ExplosionPlayer // 玩家爆炸
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
    [[nodiscard]] virtual String deathMessageKey() const = 0;

    /**
     * @brief 是否来自实体
     */
    [[nodiscard]] virtual bool isEntitySource() const { return false; }

    /**
     * @brief 是否来自玩家
     */
    [[nodiscard]] virtual bool isPlayerSource() const { return false; }

    /**
     * @brief 是否是摔落伤害
     */
    [[nodiscard]] bool isFall() const {
        return type() == DamageType::Fall || type() == DamageType::FlyIntoWall;
    }

    /**
     * @brief 是否是岩浆伤害
     */
    [[nodiscard]] bool isLava() const {
        return type() == DamageType::Lava;
    }

    /**
     * @brief 是否是仙人掌伤害
     */
    [[nodiscard]] bool isCactus() const {
        return type() == DamageType::Cactus;
    }

    /**
     * @brief 是否是饥饿伤害
     */
    [[nodiscard]] bool isStarve() const {
        return type() == DamageType::Starve;
    }

    /**
     * @brief 是否是溺水伤害
     */
    [[nodiscard]] bool isDrown() const {
        return type() == DamageType::Drown;
    }

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
    {}

    [[nodiscard]] std::unique_ptr<DamageSource> clone() const override {
        return std::make_unique<EnvironmentalDamage>(m_type);
    }

    [[nodiscard]] DamageType type() const override { return m_type; }

    [[nodiscard]] bool bypassesArmor() const override {
        return m_type == DamageType::OutOfWorld ||
               m_type == DamageType::Starve ||
               m_type == DamageType::Drown ||
               m_type == DamageType::Fall ||
               m_type == DamageType::FlyIntoWall;
    }

    [[nodiscard]] bool bypassesInvulnerability() const override {
        return m_type == DamageType::OutOfWorld;
    }

    [[nodiscard]] bool canDamageCreative() const override {
        return m_type == DamageType::OutOfWorld;
    }

    [[nodiscard]] bool isFire() const override {
        return m_type == DamageType::InFire ||
               m_type == DamageType::OnFire ||
               m_type == DamageType::Lava ||
               m_type == DamageType::HotFloor;
    }

    [[nodiscard]] bool isMagic() const override {
        return m_type == DamageType::Magic ||
               m_type == DamageType::Wither;
    }

    [[nodiscard]] String deathMessageKey() const override {
        switch (m_type) {
            case DamageType::InFire: return "death.attack.inFire";
            case DamageType::OnFire: return "death.attack.onFire";
            case DamageType::Lava: return "death.attack.lava";
            case DamageType::HotFloor: return "death.attack.hotFloor";
            case DamageType::Drown: return "death.attack.drown";
            case DamageType::Starve: return "death.attack.starve";
            case DamageType::Cactus: return "death.attack.cactus";
            case DamageType::Fall: return "death.attack.fall";
            case DamageType::FlyIntoWall: return "death.attack.flyIntoWall";
            case DamageType::OutOfWorld: return "death.attack.outOfWorld";
            case DamageType::Generic: return "death.attack.generic";
            case DamageType::Magic: return "death.attack.magic";
            case DamageType::Wither: return "death.attack.wither";
            case DamageType::Anvil: return "death.attack.anvil";
            case DamageType::FallingBlock: return "death.attack.fallingBlock";
            case DamageType::DragonBreath: return "death.attack.dragonBreath";
            case DamageType::Fireworks: return "death.attack.fireworks";
            default: return "death.attack.generic";
        }
    }

private:
    DamageType m_type;
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
    {}

    [[nodiscard]] std::unique_ptr<DamageSource> clone() const override {
        return std::make_unique<EntityDamageSource>(m_type, m_source);
    }

    [[nodiscard]] DamageType type() const override { return m_type; }

    [[nodiscard]] Entity* source() const override { return m_source; }
    [[nodiscard]] Entity* directSource() const override { return m_source; }

    [[nodiscard]] bool isFire() const override {
        return m_type == DamageType::Fireball;
    }

    [[nodiscard]] bool isProjectile() const override {
        return m_type == DamageType::Arrow ||
               m_type == DamageType::Trident ||
               m_type == DamageType::MobProjectile ||
               m_type == DamageType::Fireball;
    }

    [[nodiscard]] bool isExplosion() const override {
        return m_type == DamageType::Explosion ||
               m_type == DamageType::ExplosionPlayer;
    }

    [[nodiscard]] bool isEntitySource() const override { return true; }

    [[nodiscard]] bool isPlayerSource() const override {
        return m_type == DamageType::PlayerAttack;
    }

    [[nodiscard]] String deathMessageKey() const override {
        switch (m_type) {
            case DamageType::MobAttack: return "death.attack.mob";
            case DamageType::PlayerAttack: return "death.attack.player";
            case DamageType::Arrow: return "death.attack.arrow";
            case DamageType::Trident: return "death.attack.trident";
            case DamageType::MobProjectile: return "death.attack.mobProjectile";
            case DamageType::Fireball: return "death.attack.fireball";
            case DamageType::Thorns: return "death.attack.thorns";
            case DamageType::Explosion: return "death.attack.explosion";
            case DamageType::ExplosionPlayer: return "death.attack.explosion.player";
            default: return "death.attack.generic";
        }
    }

private:
    DamageType m_type;
    Entity* m_source;
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
    {}

    [[nodiscard]] std::unique_ptr<DamageSource> clone() const override {
        return std::make_unique<IndirectEntityDamageSource>(m_type, m_source, m_directSource, m_isPlayer);
    }

    [[nodiscard]] DamageType type() const override { return m_type; }

    [[nodiscard]] Entity* source() const override { return m_source; }
    [[nodiscard]] Entity* directSource() const override { return m_directSource; }
    [[nodiscard]] Entity* getEntity() const override { return m_source; }

    [[nodiscard]] bool isFire() const override {
        return m_type == DamageType::Fireball;
    }

    [[nodiscard]] bool isProjectile() const override {
        return m_type == DamageType::Arrow ||
               m_type == DamageType::Trident ||
               m_type == DamageType::MobProjectile ||
               m_type == DamageType::Fireball;
    }

    [[nodiscard]] bool isExplosion() const override {
        return m_type == DamageType::Explosion ||
               m_type == DamageType::ExplosionPlayer;
    }

    [[nodiscard]] bool isEntitySource() const override { return true; }

    [[nodiscard]] bool isPlayerSource() const override {
        return m_isPlayer;
    }

    [[nodiscard]] String deathMessageKey() const override {
        switch (m_type) {
            case DamageType::Arrow: return "death.attack.arrow.item";
            case DamageType::Trident: return "death.attack.trident.item";
            case DamageType::Fireball: return "death.attack.fireball.item";
            default: return "death.attack.generic";
        }
    }

private:
    DamageType m_type;
    Entity* m_source;           // 伤害来源（如射箭的玩家）
    Entity* m_directSource;     // 直接来源（如箭矢实体）
    bool m_isPlayer;            // 是否来自玩家
};

// ============================================================================
// 伤害来源工厂函数
// ============================================================================

namespace DamageSources {

/** 创建火焰伤害 */
inline EnvironmentalDamage inFire() { return EnvironmentalDamage(DamageType::InFire); }

/** 创建燃烧伤害 */
inline EnvironmentalDamage onFire() { return EnvironmentalDamage(DamageType::OnFire); }

/** 创建岩浆伤害 */
inline EnvironmentalDamage lava() { return EnvironmentalDamage(DamageType::Lava); }

/** 创建溺水伤害 */
inline EnvironmentalDamage drown() { return EnvironmentalDamage(DamageType::Drown); }

/** 创建饥饿伤害 */
inline EnvironmentalDamage starve() { return EnvironmentalDamage(DamageType::Starve); }

/** 创建仙人掌伤害 */
inline EnvironmentalDamage cactus() { return EnvironmentalDamage(DamageType::Cactus); }

/** 创建摔落伤害 */
inline EnvironmentalDamage fall() { return EnvironmentalDamage(DamageType::Fall); }

/** 创建撞墙伤害 */
inline EnvironmentalDamage flyIntoWall() { return EnvironmentalDamage(DamageType::FlyIntoWall); }

/** 创建虚空伤害 */
inline EnvironmentalDamage outOfWorld() { return EnvironmentalDamage(DamageType::OutOfWorld); }

/** 创建通用伤害 */
inline EnvironmentalDamage generic() { return EnvironmentalDamage(DamageType::Generic); }

/** 创建魔法伤害 */
inline EnvironmentalDamage magic() { return EnvironmentalDamage(DamageType::Magic); }

/** 创建凋零伤害 */
inline EnvironmentalDamage wither() { return EnvironmentalDamage(DamageType::Wither); }

/** 创建生物攻击伤害 */
inline EntityDamageSource mobAttack(Entity* mob) {
    return EntityDamageSource(DamageType::MobAttack, mob);
}

/** 创建玩家攻击伤害 */
inline EntityDamageSource playerAttack(Entity* player) {
    return EntityDamageSource(DamageType::PlayerAttack, player);
}

/** 创建箭矢伤害 */
inline IndirectEntityDamageSource arrow(Entity* arrow, Entity* shooter, bool isPlayer = false) {
    return IndirectEntityDamageSource(DamageType::Arrow, shooter, arrow, isPlayer);
}

/** 创建三叉戟伤害 */
inline IndirectEntityDamageSource trident(Entity* trident, Entity* thrower, bool isPlayer = false) {
    return IndirectEntityDamageSource(DamageType::Trident, thrower, trident, isPlayer);
}

/** 创建荆棘伤害 */
inline EntityDamageSource thorns(Entity* owner) {
    return EntityDamageSource(DamageType::Thorns, owner);
}

/** 创建爆炸伤害 */
inline EntityDamageSource explosion(Entity* source) {
    return EntityDamageSource(DamageType::Explosion, source);
}

} // namespace DamageSources

} // namespace mc
