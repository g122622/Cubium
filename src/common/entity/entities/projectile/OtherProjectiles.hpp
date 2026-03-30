#pragma once

#include "ThrowableEntity.hpp"
#include <memory>

namespace mc {
namespace entity {

/**
 * @brief 羊驼唾液实体
 *
 * 羊驼发射的唾液，对狼造成伤害。
 *
 * 参考 MC 1.16.5 LlamaSpitEntity
 */
class LlamaSpitEntity : public ThrowableEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    LlamaSpitEntity(LegacyEntityType type, EntityId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }

    [[nodiscard]] f32 getGravity() const override { return 0.06f; }  // 更高的重力

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onImpact(const RayTraceResult& result) override;
};

/**
 * @brief 钓鱼浮标实体
 *
 * 钓鱼竿的浮标，用于钓鱼机制。
 *
 * 参考 MC 1.16.5 FishingBobberEntity
 */
class FishingBobberEntity : public Entity {
public:
    /**
     * @brief 钓鱼状态
     */
    enum class State : u8 {
        Flying,     // 飞行中
        Hooked,     // 钩住实体
        Bobbing,    // 浮在水面
        Fishing     // 钓鱼中
    };

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    FishingBobberEntity(LegacyEntityType type, EntityId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }

    void tick() override;

    // ========== 钓鱼浮标方法 ==========

    /**
     * @brief 获取钓鱼者
     */
    [[nodiscard]] Player* getAngler() const;

    /**
     * @brief 获取当前状态
     */
    [[nodiscard]] State state() const { return m_state; }

    /**
     * @brief 收杆
     * @return 钓到的物品（如果有）
     */
    i32 reelIn();

private:
    /**
     * @brief 检查咬钩
     */
    void checkBite();

    /**
     * @brief 生成钓鱼粒子
     */
    void spawnFishingParticles();

    Player* m_angler = nullptr;      // 钓鱼者
    State m_state = State::Flying;   // 当前状态
    i32 m_hookCountdown = 0;         // 咬钩倒计时
    i32 m_timeUntilBite = 0;         // 下次咬钩时间
    f32 m_fishAngle = 0.0f;          // 鱼的角度（用于动画）
    bool m_inOpenWater = true;       // 是否在开放水域
};

/**
 * @brief 潜影贝子弹实体
 *
 * 潜影贝发射的跟踪子弹，造成漂浮效果。
 *
 * 参考 MC 1.16.5 ShulkerBulletEntity
 */
class ShulkerBulletEntity : public ProjectileEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    ShulkerBulletEntity(LegacyEntityType type, EntityId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.3125f; }
    [[nodiscard]] f32 height() const override { return 0.3125f; }

    void tick() override;

    // ========== 潜影贝子弹方法 ==========

    /**
     * @brief 设置目标
     */
    void setTarget(Entity* target) { m_target = target; }

    /**
     * @brief 获取目标
     */
    [[nodiscard]] Entity* target() const { return m_target; }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;

private:
    /**
     * @brief 更新飞行方向
     */
    void updateDirection();

    Entity* m_target = nullptr;       // 目标实体
    Vector3 m_direction;              // 飞行方向
    i32 m_flightSteps = 0;            // 飞行步数
};

/**
 * @brief 唤魔者尖牙实体
 *
 * 唤魔者召唤的尖牙攻击，从地下冒出造成伤害。
 *
 * 参考 MC 1.16.5 EvokerFangsEntity
 */
class EvokerFangsEntity : public Entity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    EvokerFangsEntity(LegacyEntityType type, EntityId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.5f; }
    [[nodiscard]] f32 height() const override { return 0.8f; }

    void tick() override;

    // ========== 尖牙方法 ==========

    /**
     * @brief 设置所有者
     */
    void setOwner(LivingEntity* owner) { m_owner = owner; }

    /**
     * @brief 获取所有者
     */
    [[nodiscard]] LivingEntity* owner() const { return m_owner; }

private:
    LivingEntity* m_owner = nullptr;   // 所有者
    i32 m_warmupDelay = 0;              // 预热延迟
    bool m_sentAttackEvent = false;     // 是否已发送攻击事件
    i32 m_ticksExisted = 0;             // 存在时间
};

/**
 * @brief 末影之眼实体
 *
 * 末影之眼会飞向要塞。
 *
 * 参考 MC 1.16.5 EyeOfEnderEntity
 */
class EyeOfEnderEntity : public Entity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    EyeOfEnderEntity(LegacyEntityType type, EntityId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }

    void tick() override;

    // ========== 末影之眼方法 ==========

    /**
     * @brief 设置目标位置（要塞方向）
     */
    void moveTo(BlockCoord targetX, BlockCoord targetZ);

    /**
     * @brief 获取目标X坐标
     */
    [[nodiscard]] BlockCoord targetX() const { return m_targetX; }

    /**
     * @brief 获取目标Z坐标
     */
    [[nodiscard]] BlockCoord targetZ() const { return m_targetZ; }

    /**
     * @brief 是否应该碎裂
     */
    [[nodiscard]] bool shouldBreak() const { return m_break; }

private:
    BlockCoord m_targetX = 0;     // 目标X
    BlockCoord m_targetZ = 0;     // 目标Z
    i32 m_lifetime = 0;           // 存在时间
    bool m_break = false;         // 是否碎裂
};

/**
 * @brief 烟花火箭实体
 *
 * 烟花火箭可以发射、爆炸并产生各种效果。
 *
 * 参考 MC 1.16.5 FireworkRocketEntity
 */
class FireworkRocketEntity : public ProjectileEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    FireworkRocketEntity(LegacyEntityType type, EntityId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }

    void tick() override;

    // ========== 烟花火箭方法 ==========

    /**
     * @brief 设置烟花数据
     * @param data 烟花爆炸数据（NBT格式）
     */
    void setFireworkData(/* const CompoundNBT& data */) { /* TODO */ }

    /**
     * @brief 是否从弩射出
     */
    [[nodiscard]] bool shotFromCrossbow() const { return m_shotFromCrossbow; }

    /**
     * @brief 设置是否从弩射出
     */
    void setShotFromCrossbow(bool value) { m_shotFromCrossbow = value; }

    /**
     * @brief 获取飞行时间
     */
    [[nodiscard]] i32 flightTime() const { return m_flightTime; }

    /**
     * @brief 设置飞行时间
     */
    void setFlightTime(i32 time) { m_flightTime = time; }

private:
    /**
     * @brief 爆炸
     */
    void explode();

    /**
     * @brief 处理弩发射的伤害
     */
    void dealExplosionDamage();

    i32 m_flightTime = 0;            // 飞行时间
    i32 m_lifetime = 0;              // 存在时间
    bool m_shotFromCrossbow = false; // 是否从弩射出
    // CompoundNBT m_fireworkData;   // 烟花数据
};

} // namespace entity
} // namespace mc
