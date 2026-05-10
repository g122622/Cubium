#pragma once

#include "../../core/Entity.hpp"
#include "../../../world/block/BlockPos.hpp"
#include <memory>

namespace mc {

// Forward declarations
class Block;
class Player;
class LivingEntity;
class DamageSource;

namespace entity {

/**
 * @brief 下落方块实体
 *
 * 沙子、砾石等方块下落时创建的实体。
 *
 * 参考 MC 1.16.5 FallingBlockEntity
 */
class FallingBlockEntity : public Entity {
public:
    FallingBlockEntity();
    ~FallingBlockEntity() override = default;

    void tick() override;

    [[nodiscard]] f32 width() const override { return 0.98f; }
    [[nodiscard]] f32 height() const override { return 0.98f; }
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const { return false; }

    /**
     * @brief 设置方块ID
     */
    void setBlockId(u32 blockId) { m_blockId = blockId; }
    [[nodiscard]] u32 getBlockId() const { return m_blockId; }

    /**
     * @brief 设置是否在落地时造成伤害
     */
    void setHurtEntities(bool hurt) { m_hurtEntities = hurt; }
    [[nodiscard]] bool shouldHurtEntities() const { return m_hurtEntities; }

    /**
     * @brief 设置下落起始位置（用于计算伤害）
     */
    void setFallStartPos(f64 y) { m_fallStartY = y; }

    /**
     * @brief 检查是否应该放置方块
     */
    [[nodiscard]] bool shouldPlaceBlock() const { return m_placeBlock; }

private:
    void handleLanding();

    u32 m_blockId = 0;  // 方块ID
    bool m_hurtEntities = false;
    bool m_placeBlock = true;
    f64 m_fallStartY = 0.0;
    i32 m_fallTime = 0;
    static constexpr f32 HURT_AMOUNT = 2.0f;
    static constexpr i32 MAX_HURT_AMOUNT = 40;
};

/**
 * @brief TNT实体
 *
 * 被激活的TNT方块，倒计时后爆炸。
 *
 * 参考 MC 1.16.5 TNTEntity
 */
class TNTEntity : public Entity {
public:
    TNTEntity();
    TNTEntity(LegacyEntityType type, EntityId id);
    ~TNTEntity() override = default;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    void tick() override;

    [[nodiscard]] f32 width() const override { return 0.98f; }
    [[nodiscard]] f32 height() const override { return 0.98f; }
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const { return false; }

    /**
     * @brief 获取爆炸倒计时
     */
    [[nodiscard]] i32 getFuse() const { return m_fuse; }
    void setFuse(i32 fuse) { m_fuse = fuse; }

    /**
     * @brief 设置爆炸半径
     */
    void setExplosionRadius(f32 radius) { m_explosionRadius = radius; }
    [[nodiscard]] f32 getExplosionRadius() const { return m_explosionRadius; }

    /**
     * @brief 点燃TNT
     *
     * 设置引信时间并开始倒计时。
     */
    void ignite();

    /**
     * @brief 设置点燃者
     * @param igniter 点燃TNT的实体（可为nullptr）
     *
     * 用于追踪爆炸责任的归属。
     */
    void setOwner(LivingEntity* igniter) { m_owner = igniter; }
    [[nodiscard]] LivingEntity* getOwner() const { return m_owner; }

    /**
     * @brief 爆炸
     */
    void explode();

    /**
     * @brief 检查是否已点燃
     */
    [[nodiscard]] bool isPrimed() const { return m_fuse > 0; }

private:
    i32 m_fuse = 0;
    f32 m_explosionRadius = 4.0f;
    bool m_exploded = false;
    LivingEntity* m_owner = nullptr;
    static constexpr i32 DEFAULT_FUSE = 80; // 4秒（80 ticks）
};

/**
 * @brief 潮涌核心实体
 *
 * 潮涌核心激活后产生的效果实体。
 *
 * 参考 MC 1.16.5 ConduitEntity
 */
class ConduitEntity : public Entity {
public:
    ConduitEntity();
    ~ConduitEntity() override = default;

    void tick() override;

    [[nodiscard]] f32 width() const override { return 2.0f; }
    [[nodiscard]] f32 height() const override { return 2.0f; }
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const { return false; }

    /**
     * @brief 设置激活状态
     */
    void setActive(bool active) { m_active = active; }
    [[nodiscard]] bool isActive() const { return m_active; }

    /**
     * @brief 设置目标（攻击目标）
     */
    void setTarget(LivingEntity* target);
    [[nodiscard]] LivingEntity* getTarget() const { return m_target; }

private:
    void applyEffects();
    void attackTarget();

    bool m_active = false;
    LivingEntity* m_target = nullptr;
    i32 m_effectCooldown = 0;
    i32 m_attackCooldown = 0;
    static constexpr f32 EFFECT_RADIUS = 42.0f;
    static constexpr f32 ATTACK_RADIUS = 8.0f;
    static constexpr i32 EFFECT_INTERVAL = 20;
    static constexpr i32 ATTACK_INTERVAL = 40;
};

/**
 * @brief 寂守者警告实体
 *
 * 寂守者检测到振动后产生的警告效果。
 * 不是实体，是效果的一种，但暂时放在这里。
 */
class WardenWarningEffect {
public:
    WardenWarningEffect() = default;
    ~WardenWarningEffect() = default;

    void tick();

    [[nodiscard]] i32 getWarningLevel() const { return m_warningLevel; }
    void increaseWarning();
    void decreaseWarning();

    [[nodiscard]] BlockPos getSourcePos() const { return m_sourcePos; }
    void setSourcePos(BlockPos pos) { m_sourcePos = pos; }

    [[nodiscard]] f32 getWarningRadius() const { return m_warningRadius; }

private:
    i32 m_warningLevel = 0;
    BlockPos m_sourcePos;
    f32 m_warningRadius = 10.0f;
    i32 m_cooldown = 0;
    static constexpr i32 MAX_WARNING = 4;
    static constexpr i32 DECREASE_INTERVAL = 200;
};

// 注意: EvokerFangsEntity 和 EyeOfEnderEntity 已移至
// src/common/entity/entities/projectile/OtherProjectiles.hpp 以避免重复定义

} // namespace entity
} // namespace mc
