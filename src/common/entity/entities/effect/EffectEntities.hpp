#pragma once

#include "../../core/Entity.hpp"
#include "../../../world/block/BlockPos.hpp"

namespace mc {

// Forward declarations
class Player;
class LivingEntity;

namespace entity {

/**
 * @brief 末影水晶实体
 *
 * 在末地生成，用于治愈末影龙。
 *
 * 参考 MC 1.16.5 EnderCrystalEntity
 */
class EnderCrystalEntity : public Entity {
public:
    EnderCrystalEntity();
    ~EnderCrystalEntity() override = default;

    void tick() override;

    [[nodiscard]] f32 width() const override;
    [[nodiscard]] f32 height() const override;
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const { return false; }

    /**
     * @brief 检查是否显示基岩
     */
    [[nodiscard]] bool shouldShowBottom() const { return m_showBottom; }
    void setShowBottom(bool show) { m_showBottom = show; }

    /**
     * @brief 设置关联的末地传送门位置
     */
    void setBeamTarget(BlockPos pos);
    [[nodiscard]] const BlockPos& getBeamTarget() const { return m_beamTarget; }
    [[nodiscard]] bool hasBeamTarget() const;

    /**
     * @brief 治愈末影龙
     */
    void healDragon();

    /**
     * @brief 爆炸
     */
    void explode();

private:
    BlockPos m_beamTarget;
    bool m_showBottom = false;
    i32 m_healCooldown = 0;
    static constexpr i32 HEAL_COOLDOWN = 10;
    static constexpr f32 EXPLOSION_RADIUS = 6.0f;
};

/**
 * @brief 闪电实体
 *
 * 雷暴天气时生成的闪电。
 *
 * 参考 MC 1.16.5 LightningBoltEntity
 */
class LightningBoltEntity : public Entity {
public:
    LightningBoltEntity();
    ~LightningBoltEntity() override = default;

    void tick() override;

    [[nodiscard]] f32 width() const override;
    [[nodiscard]] f32 height() const override;
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const { return false; }

    /**
     * @brief 设置是否只对实体造成伤害（不影响方块）
     */
    void setEffectOnly(bool effectOnly) { m_effectOnly = effectOnly; }
    [[nodiscard]] bool isEffectOnly() const { return m_effectOnly; }

    /**
     * @brief 获取闪电存在时间
     */
    [[nodiscard]] i32 getTicksLived() const { return m_ticksLived; }

    /**
     * @brief 伤害周围实体
     */
    void damageEntities();

    /**
     * @brief 在周围生成火焰
     */
    void spawnFire();

    /**
     * @brief 触发闪电事件
     */
    void triggerLightningEffect();

private:
    bool m_effectOnly = false;
    i32 m_ticksLived = 0;
    i32 m_flashCount = 0;
    bool m_hasStruck = false;
    static constexpr i32 LIFETIME = 30;
    static constexpr f32 DAMAGE_RADIUS = 3.0f;
    static constexpr f32 DAMAGE_AMOUNT = 5.0f;
};

/**
 * @brief 区域效果云实体
 *
 * 由滞留药水产生的效果云。
 *
 * 参考 MC 1.16.5 AreaEffectCloudEntity
 */
class AreaEffectCloudEntity : public Entity {
public:
    AreaEffectCloudEntity();
    ~AreaEffectCloudEntity() override = default;

    void tick() override;

    [[nodiscard]] f32 width() const override;
    [[nodiscard]] f32 height() const override;
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const { return false; }

    /**
     * @brief 设置效果半径
     */
    void setRadius(f32 radius) { m_radius = radius; }
    [[nodiscard]] f32 getRadius() const { return m_radius; }

    /**
     * @brief 设置持续时间
     */
    void setDuration(i32 duration) { m_duration = duration; }
    [[nodiscard]] i32 getDuration() const { return m_duration; }

    /**
     * @brief 设置等待时间
     */
    void setWaitTime(i32 waitTime) { m_waitTime = waitTime; }
    [[nodiscard]] i32 getWaitTime() const { return m_waitTime; }

    /**
     * @brief 设置效果颜色
     */
    void setColor(u32 color) { m_color = color; }
    [[nodiscard]] u32 getColor() const { return m_color; }

    /**
     * @brief 添加效果
     */
    // void addEffect(const EffectInstance& effect);

    /**
     * @brief 设置重新申请时间
     */
    void setReapplicationDelay(i32 delay) { m_reapplicationDelay = delay; }

private:
    void applyEffects();
    void updateRadius();

    f32 m_radius = 3.0f;
    f32 m_initialRadius = 3.0f;
    i32 m_duration = 600;
    i32 m_waitTime = 10;
    i32 m_reapplicationDelay = 20;
    i32 m_durationOnUse = 0;
    i32 m_ticksLived = 0;
    u32 m_color = 0;
    // std::vector<EffectInstance> m_effects;
    static constexpr f32 RADIUS_GROWTH = -0.005f;
};

/**
 * @brief 经验球实体
 *
 * 玩家拾取后获得经验值。
 *
 * 参考 MC 1.16.5 ExperienceOrbEntity
 */
class ExperienceOrbEntity : public Entity {
public:
    explicit ExperienceOrbEntity(i32 xpValue = 1);
    ~ExperienceOrbEntity() override = default;

    void tick() override;

    [[nodiscard]] f32 width() const override;
    [[nodiscard]] f32 height() const override;
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const { return false; }

    /**
     * @brief 获取经验值
     */
    [[nodiscard]] i32 getXpValue() const { return m_xpValue; }
    void setXpValue(i32 value) { m_xpValue = value; }

    /**
     * @brief 检查是否正在被玩家追踪
     */
    [[nodiscard]] bool isBeingTracked() const { return m_trackingPlayer != nullptr; }

    /**
     * @brief 分割经验球
     * @return 分割后的经验球
     */
    ExperienceOrbEntity* split();

    /**
     * @brief 计算经验值对应的颜色
     */
    [[nodiscard]] u32 getExperienceColor() const;

private:
    void followPlayer(Player* player);

    i32 m_xpValue;
    Player* m_trackingPlayer = nullptr;
    i32 m_trackingCooldown = 0;
    i32 m_despawnDelay = 6000;
    i32 m_collectDelay = 0;
    static constexpr f32 FOLLOW_RANGE = 8.0f;
    static constexpr f32 FOLLOW_SPEED = 0.5f;
    static constexpr i32 MAX_XP_VALUE = 2477;
};

/**
 * @brief 盔甲架实体
 *
 * 可以展示和穿戴盔甲的实体。
 *
 * 参考 MC 1.16.5 ArmorStandEntity
 */
class ArmorStandEntity : public Entity {
public:
    ArmorStandEntity();
    ~ArmorStandEntity() override = default;

    void tick() override;

    [[nodiscard]] f32 width() const override;
    [[nodiscard]] f32 height() const override;
    [[nodiscard]] bool isPushable() const { return !m_marker; }
    [[nodiscard]] bool canBeCollidedWith() const { return !m_marker; }

    /**
     * @brief 检查是否有重力
     */
    [[nodiscard]] bool hasGravity() const { return m_hasGravity; }
    void setGravity(bool gravity) { m_hasGravity = gravity; }

    /**
     * @brief 检查是否可见（非标记模式）
     */
    [[nodiscard]] bool isVisible() const { return !m_invisible && !m_marker; }

    /**
     * @brief 检查是否为标记模式
     */
    [[nodiscard]] bool isMarker() const { return m_marker; }
    void setMarker(bool marker) { m_marker = marker; }

    /**
     * @brief 检查是否有底座
     */
    [[nodiscard]] bool hasBasePlate() const { return m_basePlate; }
    void setBasePlate(bool basePlate) { m_basePlate = basePlate; }

    /**
     * @brief 检查是否显示手臂
     */
    [[nodiscard]] bool hasArms() const { return m_arms; }
    void setArms(bool arms) { m_arms = arms; }

    /**
     * @brief 设置头部旋转
     */
    void setHeadRotation(f32 x, f32 y, f32 z);
    void setBodyRotation(f32 x, f32 y, f32 z);
    void setLeftArmRotation(f32 x, f32 y, f32 z);
    void setRightArmRotation(f32 x, f32 y, f32 z);
    void setLeftLegRotation(f32 x, f32 y, f32 z);
    void setRightLegRotation(f32 x, f32 y, f32 z);

    /**
     * @brief 检查是否是小型
     */
    [[nodiscard]] bool isSmall() const { return m_small; }
    void setSmall(bool small) { m_small = small; }

private:
    bool m_hasGravity = true;
    bool m_invisible = false;
    bool m_marker = false;
    bool m_basePlate = true;
    bool m_arms = false;
    bool m_small = false;

    // 身体部位旋转（欧拉角）
    struct EulerAngles {
        f32 x = 0.0f, y = 0.0f, z = 0.0f;
    } m_head, m_body, m_leftArm, m_rightArm, m_leftLeg, m_rightLeg;
};

} // namespace entity
} // namespace mc
