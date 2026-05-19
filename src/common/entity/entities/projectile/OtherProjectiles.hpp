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

#include "ProjectileEntity.hpp"
#include "ProjectileHelper.hpp"
#include "ThrowableEntity.hpp"
#include "common/item/core/ItemStack.hpp"
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
    explicit LlamaSpitEntity(EntityId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }

    [[nodiscard]] f32 getGravity() const override { return 0.06f; } // 更高的重力

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
        Flying,  // 飞行中
        Hooked,  // 钩住实体
        Bobbing, // 浮在水面
        Fishing  // 钓鱼中（咬钩状态）
    };

    /**
     * @brief 水类型（用于开放水域检测）
     */
    enum class WaterType : u8 {
        AboveWater,  // 水上方块（空气或睡莲）
        InsideWater, // 水内部（完整水源方块）
        Invalid      // 无效
    };

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit FishingBobberEntity(EntityId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }

    void tick() override;

    // ========== 钓鱼浮标方法 ==========

    /**
     * @brief 获取钓鱼者
     */
    [[nodiscard]] Player* getAngler() const { return m_angler; }

    /**
     * @brief 设置发射者（钓鱼者）
     * @param shooter 发射者实体
     */
    void setShooter(Entity* shooter);

    /**
     * @brief 获取当前状态
     */
    [[nodiscard]] State state() const { return m_state; }

    /**
     * @brief 发射浮标
     * @param shooter 发射者
     * @param pitch 俯仰角（度）
     * @param yaw 偏航角（度）
     * @param pitchOffset 俯仰角偏移
     * @param velocity 速度
     * @param inaccuracy 不准确度
     */
    void shootFrom(Entity& shooter, f32 pitch, f32 yaw, f32 pitchOffset, f32 velocity, f32 inaccuracy);

    /**
     * @brief 收杆
     * @return 钓到的物品数量（用于耐久消耗）
     */
    i32 reelIn();

    /**
     * @brief 获取被钩住的实体
     * @return 被钩住的实体指针，如果没有则返回 nullptr
     */
    [[nodiscard]] Entity* getCaughtEntity() const { return m_caughtEntity; }

    /**
     * @brief 获取被钩住的实体ID（用于网络同步）
     * @return 实体ID，如果没有则返回 0
     */
    [[nodiscard]] EntityId getCaughtEntityId() const { return m_caughtEntityId; }

    /**
     * @brief 设置钓鱼附魔加成
     * @param luckBonus 海之眷顾附魔等级
     * @param speedBonus 饵钓附魔等级
     */
    void setFishingBonus(i32 luckBonus, i32 speedBonus)
    {
        m_luckBonus = luckBonus;
        m_speedBonus = speedBonus;
    }

    /**
     * @brief 是否在开放水域
     */
    [[nodiscard]] bool isInOpenWater() const { return m_inOpenWater; }

private:
    /**
     * @brief 检测水面并更新状态
     */
    void updateWaterState();

    /**
     * @brief 检测是否在水中
     */
    [[nodiscard]] bool isInWater() const override;

    /**
     * @brief 检测开放水域
     * @return 是否满足开放水域条件
     */
    [[nodiscard]] bool checkOpenWater();

    /**
     * @brief 钓鱼逻辑tick
     */
    void catchingFish();

    /**
     * @brief 生成钓鱼粒子
     */
    void spawnFishingParticles();

    /**
     * @brief 生成收杆物品
     * @return 消耗的耐久度
     */
    i32 spawnCatchItems();

    /**
     * @brief 生成经验球
     * @param totalXp 总经验值
     */
    void spawnExperienceOrbs(i32 totalXp);

    /**
     * @brief 设置咬钩等待时间
     */
    void setWaitTime();

    /**
     * @brief 执行射线检测
     * @return 射线检测结果
     *
     * 参考 MC 1.16.5 FishingBobberEntity.checkCollision()
     */
    [[nodiscard]] RayTraceResult performRayTrace();

    /**
     * @brief 检查是否可以命中指定实体
     * @param target 目标实体
     * @return 是否可以命中
     *
     * 钓鱼浮标可以命中：普通可命中实体 + 物品实体
     */
    [[nodiscard]] bool canHitEntity(const Entity& target) const;

    /**
     * @brief 命中实体时的回调
     * @param result 射线检测结果
     *
     * 参考 MC 1.16.5 FishingBobberEntity.onEntityHit()
     */
    void onEntityHit(const RayTraceResult& result);

    /**
     * @brief 命中方块时的回调
     * @param result 射线检测结果
     *
     * 参考 MC 1.16.5: 命中方块后停止移动
     */
    void onBlockHit(const RayTraceResult& result);

    /**
     * @brief 拉动被钩住的实体
     *
     * 参考 MC 1.16.5 FishingBobberEntity.bringInHookedEntity()
     */
    void bringInHookedEntity();

    /**
     * @brief 同步被钩住实体ID（用于客户端）
     */
    void syncCaughtEntityId();

    Player* m_angler = nullptr;       // 钓鱼者
    Entity* m_caughtEntity = nullptr; // 被钩住的实体
    EntityId m_caughtEntityId = 0;    // 被钩住实体ID（用于网络同步，存储时+1，0表示无）
    State m_state = State::Flying;    // 当前状态
    i32 m_ticksCaughtDelay = 0;       // 咬钩等待计时器
    i32 m_ticksCatchableDelay = 0;    // 鱼接近计时器
    i32 m_ticksCatchable = 0;         // 可捕获窗口期
    f32 m_fishAngle = 0.0f;           // 鱼的角度（用于动画）
    bool m_inOpenWater = false;       // 是否在开放水域
    i32 m_luckBonus = 0;              // 海之眷顾附魔等级
    i32 m_speedBonus = 0;             // 饵钓附魔等级
    i32 m_outOfWaterTime = 0;         // 离开水的时间计数器
    i32 m_lifetime = 0;               // 存在时间
};

/**
 * @brief 潜影贝子弹实体
 *
 * 潜影贝发射的跟踪子弹，造成漂浮效果。
 *
 * 特性：
 * - 沿轴向移动，追踪目标
 * - 命中后造成4点伤害和10秒漂浮效果
 * - 被击中时会消失并产生爆炸粒子
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
     * @brief 默认构造函数
     */
    explicit ShulkerBulletEntity(EntityId id);

    /**
     * @brief 带目标的构造函数
     * @param world 世界
     * @param shooter 发射者（潜影贝）
     * @param target 目标实体
     * @param axis 初始移动轴
     */
    ShulkerBulletEntity(IWorld* world, LivingEntity* shooter, Entity* target, Axis axis);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.3125f; }
    [[nodiscard]] f32 height() const override { return 0.3125f; }

    void tick() override;

    // ========== 投掷物属性 ==========

    [[nodiscard]] bool isBurning() const { return false; }
    [[nodiscard]] f32 getBrightness() const { return 1.0f; }
    [[nodiscard]] bool canBeCollidedWith() const override { return true; }

    // ========== 潜影贝子弹方法 ==========

    /**
     * @brief 设置目标
     */
    void setTarget(Entity* target);

    /**
     * @brief 获取目标
     */
    [[nodiscard]] Entity* target() const { return m_target; }

    /**
     * @brief 获取当前移动方向
     */
    [[nodiscard]] Direction direction() const { return m_direction; }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;
    void onImpact(const RayTraceResult& result) override;

    /**
     * @brief 检查是否可以命中指定实体
     */
    [[nodiscard]] bool canHitEntity(const Entity& target) const override;

private:
    /**
     * @brief 选择下一个移动方向
     * @param excludedAxis 排除的轴（避免反向移动）
     */
    void selectNextMoveDirection(Axis excludedAxis);

    /**
     * @brief 设置移动方向
     */
    void setDirection(Direction dir);

    /**
     * @brief 更新飞行逻辑
     */
    void updateFlight();

    Entity* m_target = nullptr;            ///< 目标实体
    std::string m_targetUuid;              ///< 目标UUID（用于重新查找）
    Direction m_direction = Direction::Up; ///< 当前移动方向
    i32 m_flightSteps = 0;                 ///< 剩余飞行步数
    Vector3d m_targetDelta;                ///< 目标速度增量

    // 常量
    static constexpr f32 BULLET_SPEED = 0.15;          ///< 子弹速度
    static constexpr f32 ACCELERATION = 1.025;         ///< 加速度因子
    static constexpr i32 MIN_STEPS = 10;               ///< 最小飞行步数
    static constexpr i32 MAX_STEPS_EXTRA = 5;          ///< 额外飞行步数范围
    static constexpr f32 LEVITATION_DURATION = 200.0f; ///< 漂浮效果持续时间（ticks）
    static constexpr f32 DAMAGE = 4.0f;                ///< 伤害值
};

/**
 * @brief 唤魔者尖牙实体
 *
 * 唤魔者召唤的尖牙攻击，从地下冒出造成伤害。
 *
 * 特性：
 * - 预热延迟：尖牙出现前有预热时间
 * - 范围伤害：对碰撞箱内的生物造成魔法伤害
 * - 队伍判断：不伤害唤魔者及其队友
 * - 有限生命：攻击后自动消失
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
    explicit EvokerFangsEntity(EntityId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.5f; }
    [[nodiscard]] f32 height() const override { return 0.8f; }

    void tick() override;

    // ========== 尖牙方法 ==========

    /**
     * @brief 设置所有者
     * @param owner 所有者实体（唤魔者）
     */
    void setOwner(LivingEntity* owner) { m_owner = owner; }

    /**
     * @brief 获取所有者
     * @return 所有者实体
     */
    [[nodiscard]] LivingEntity* owner() const { return m_owner; }

    /**
     * @brief 设置预热延迟
     * @param delay 预热延迟（ticks）
     */
    void setWarmupDelay(i32 delay) { m_warmupDelay = delay; }

    /**
     * @brief 获取预热延迟
     */
    [[nodiscard]] i32 warmupDelay() const { return m_warmupDelay; }

    /**
     * @brief 获取动画进度
     * @param partialTicks 部分tick时间
     * @return 动画进度（0.0-1.0）
     *
     * 参考 MC 1.16.5 EvokerFangsEntity.getAnimationProgress()
     */
    [[nodiscard]] f32 getAnimationProgress(f32 partialTicks) const;

private:
    /**
     * @brief 对范围内实体造成伤害
     *
     * 参考 MC 1.16.5 EvokerFangsEntity.damage()
     */
    void damageEntities();

    LivingEntity* m_owner = nullptr;        ///< 所有者（唤魔者）
    i32 m_warmupDelay = 0;                  ///< 预热延迟（ticks）
    bool m_sentAttackEvent = false;         ///< 是否已发送攻击事件
    i32 m_lifeTicks = 22;                   ///< 生命时长（ticks），MC 1.16.5 默认22
    bool m_clientSideAttackStarted = false; ///< 客户端攻击开始标志
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
    explicit EyeOfEnderEntity(EntityId id);

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
    BlockCoord m_targetX = 0; // 目标X
    BlockCoord m_targetZ = 0; // 目标Z
    i32 m_lifetime = 0;       // 存在时间
    bool m_break = false;     // 是否碎裂
};

/**
 * @brief 烟花火箭实体
 *
 * 烟花火箭可以发射、爆炸并产生各种效果。
 * 从弩发射的烟花火箭会对周围实体造成伤害。
 *
 * 伤害机制（MC 1.16.5）：
 * - 爆炸半径：5 格
 * - 基础伤害：5 点
 * - 每个爆炸效果增加：+2 点伤害
 * - 距离衰减：damage * sqrt((5 - distance) / 5)
 * - 视线检测：两条射线（脚部和腰部），任一未被方块阻挡即可造成伤害
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
    explicit FireworkRocketEntity(EntityId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }

    void tick() override;

    // ========== 烟花火箭方法 ==========

    /**
     * @brief 设置烟花物品
     * @param item 烟花火箭物品堆
     *
     * 从物品中读取飞行时间和爆炸效果数据。
     */
    void setFireworkItem(const ItemStack& item);

    /**
     * @brief 获取烟花物品
     * @return 烟花火箭物品堆（可能为空）
     */
    [[nodiscard]] const ItemStack& fireworkItem() const { return m_fireworkItem; }

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

    /**
     * @brief 获取爆炸效果数量
     * @return 爆炸效果数量（0 表示无爆炸效果）
     */
    [[nodiscard]] i32 getExplosionCount() const;

    /**
     * @brief 检查视线是否被方块阻挡
     * @param target 目标实体
     * @return 如果视线未被阻挡返回 true
     */
    [[nodiscard]] bool canSeeEntity(const Entity& target) const;

    /**
     * @brief 处理弩发射的伤害
     *
     * 对爆炸半径 5 格内的 LivingEntity 造成伤害。
     * 伤害计算：5 + 爆炸效果数量 * 2，根据距离衰减。
     */
    void dealExplosionDamage();

private:
    /**
     * @brief 爆炸
     */
    void explode();

    ItemStack m_fireworkItem;        // 烟花火箭物品
    i32 m_flightTime = 1;            // 飞行时间（ticks = flightTime * 10 + random）
    i32 m_lifetime = 0;              // 已存在时间
    bool m_shotFromCrossbow = false; // 是否从弩射出
};

} // namespace entity
} // namespace mc
