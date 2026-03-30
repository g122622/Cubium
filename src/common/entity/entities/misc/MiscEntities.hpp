#pragma once

#include "../../core/Entity.hpp"
#include "../../world/block/BlockState.hpp"
#include <memory>

namespace mc {

// Forward declarations
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
    explicit FallingBlockEntity(const BlockState& block);
    ~FallingBlockEntity() override = default;

    void tick() override;

    [[nodiscard]] bool isPushable() const override { return false; }
    [[nodiscard]] bool canBeCollidedWith() const override { return false; }

    /**
     * @brief 获取方块状态
     */
    [[nodiscard]] const BlockState& getBlockState() const { return *m_block; }
    void setBlockState(const BlockState& block);

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

    std::unique_ptr<BlockState> m_block;
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
    TNTEntity(f64 x, f64 y, f64 z);
    ~TNTEntity() override = default;

    void tick() override;

    [[nodiscard]] bool isPushable() const override { return false; }
    [[nodiscard]] bool canBeCollidedWith() const override { return false; }

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
     */
    void ignite();

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
    Entity* m_owner = nullptr;
    static constexpr i32 DEFAULT_FUSE = 80; // 4秒
};

/**
 * @brief 末影之眼实体
 *
 * 投掷后的末影之眼，飞向要塞。
 *
 * 参考 MC 1.16.5 EyeOfEnderEntity
 */
class EyeOfEnderEntity : public Entity {
public:
    EyeOfEnderEntity();
    ~EyeOfEnderEntity() override = default;

    void tick() override;

    [[nodiscard]] bool isPushable() const override { return false; }
    [[nodiscard]] bool canBeCollidedWith() const override { return false; }

    /**
     * @brief 设置目标位置
     */
    void setTargetPos(f64 x, f64 y, f64 z);

    /**
     * @brief 设置是否应该掉落
     */
    void setShouldDrop(bool drop) { m_shatter = drop; }
    [[nodiscard]] bool shouldDrop() const { return m_shatter; }

    /**
     * @brief 获取生存时间
     */
    [[nodiscard]] i32 getLifeTime() const { return m_lifeTime; }

private:
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_lifeTime = 0;
    bool m_shatter = false;
    static constexpr i32 MAX_LIFE = 80;
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

    [[nodiscard]] bool isPushable() const override { return false; }
    [[nodiscard]] bool canBeCollidedWith() const override { return false; }

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

/**
 * @brief 唤魔者尖牙实体
 *
 * 唤魔者召唤的地刺攻击。
 *
 * 参考 MC 1.16.5 EvokerFangsEntity
 */
class EvokerFangsEntity : public Entity {
public:
    EvokerFangsEntity();
    EvokerFangsEntity(f64 x, f64 y, f64 z, f32 yaw, i32 delay);
    ~EvokerFangsEntity() override = default;

    void tick() override;

    [[nodiscard]] bool isPushable() const override { return false; }
    [[nodiscard]] bool canBeCollidedWith() const override { return false; }

    /**
     * @brief 设置召唤者
     */
    void setOwner(LivingEntity* owner) { m_owner = owner; }
    [[nodiscard]] LivingEntity* getOwner() const { return m_owner; }

    /**
     * @brief 设置伤害值
     */
    void setDamage(f32 damage) { m_damage = damage; }

private:
    void attackEntities();

    LivingEntity* m_owner = nullptr;
    i32 m_delay = 0;
    i32 m_ticksLived = 0;
    bool m_hasAttacked = false;
    f32 m_damage = 6.0f;
    static constexpr i32 WARMUP_DELAY = 14;
    static constexpr i32 LIFETIME = 22;
};

} // namespace entity
} // namespace mc
