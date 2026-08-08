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

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/experience/ExperienceConstants.hpp"
#include <memory>
#include <string>

namespace mc {

// 前向声明
class Player;

/**
 * @brief 经验球实体
 *
 * 玩家拾取后获得经验值。经验球会在世界中漂浮，
 * 并被附近的玩家吸引。
 *
 * 特性：
 * - 受重力影响
 * - 被附近玩家吸引（8格范围）
 * - 可合并成更大的经验球
 * - 5分钟后消失
 */
class ExperienceOrbEntity : public Entity {
public:
    // ========== 常量 ==========

    /**
     * @brief 最大经验球值
     * 单个经验球可以包含的最大经验值
     */
    static constexpr i32 MAX_ORB_SIZE = entity::experience::constants::MAX_ORB_VALUE;

    /**
     * @brief 最大存活时间 (ticks)
     * 6000 ticks = 5 分钟
     */
    static constexpr i32 MAX_AGE = entity::experience::constants::MAX_ORB_AGE;

    /**
     * @brief 默认拾取延迟 (ticks)
     * 原版 MC 构造函数中不设置 pickupDelay，默认为 0
     */
    static constexpr i32 DEFAULT_PICKUP_DELAY = entity::experience::constants::DEFAULT_PICKUP_DELAY;

    /**
     * @brief 追踪玩家范围 (方块)
     */
    static constexpr f32 TRACKING_RANGE = entity::experience::constants::ORB_TRACKING_RANGE;

    /**
     * @brief 拾取检测距离 (方块)
     */
    static constexpr f32 PICKUP_DISTANCE = entity::experience::constants::ORB_PICKUP_DISTANCE;

    // ========== 构造函数 ==========

    /**
     * @brief 默认构造函数
     * @param xpValue 经验值
     * @param registry ECS 实体注册表
     */
    explicit ExperienceOrbEntity(i32 xpValue, ecs::EntityRegistry& registry);

    /**
     * @brief 完整构造函数
     * @param world 世界指针
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param xpValue 经验值
     * @param registry ECS 实体注册表
     */
    ExperienceOrbEntity(IWorld* world, f64 x, f64 y, f64 z, i32 xpValue, ecs::EntityRegistry& registry);

    /**
     * @brief 工厂方法（用于实体注册）
     * @param world 世界指针
     * @param registry ECS 实体注册表
     * @return 新创建的经验球实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    ~ExperienceOrbEntity() override = default;

    // 禁止拷贝
    ExperienceOrbEntity(const ExperienceOrbEntity&) = delete;
    ExperienceOrbEntity& operator=(const ExperienceOrbEntity&) = delete;

    // 允许移动
    ExperienceOrbEntity(ExperienceOrbEntity&&) = delete;
    ExperienceOrbEntity& operator=(ExperienceOrbEntity&&) = delete;

    // ========== Entity 接口 ==========

    void tick() override;

    [[nodiscard]] f32 width() const override { return 0.5f; }
    [[nodiscard]] f32 height() const override { return 0.5f; }

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

    /**
     * @brief 处理经验球实体受到伤害
     *
     * 经验球有 5 点生命值，受到伤害时减少生命值。
     * 当生命值降至 0 或以下时，经验球被销毁（调用 discard()）。
     * 对应 MC Java 的 ExperienceOrb.hurtServer()。
     */
    bool hurt(DamageSource& source, f32 amount) override;

    // ========== 经验相关 ==========

    /**
     * @brief 获取经验值
     */
    [[nodiscard]] i32 getXpValue() const { return m_xpValue; }

    /**
     * @brief 设置经验值
     * @param value 新的经验值
     */
    void setXpValue(i32 value);

    /**
     * @brief 获取经验球大小等级
     *
     * 根据经验值返回大小等级 (0-10)，用于渲染纹理选择。
     *
     * @return 大小等级
     */
    [[nodiscard]] i32 getOrbSize() const;

    /**
     * @brief 获取存活时间
     */
    [[nodiscard]] i32 getAge() const { return m_age; }

    /**
     * @brief 设置存活时间
     */
    void setAge(i32 age) { m_age = age; }

    /**
     * @brief 获取拾取延迟
     */
    [[nodiscard]] i32 getPickupDelay() const { return m_pickupDelay; }

    /**
     * @brief 设置拾取延迟
     */
    void setPickupDelay(i32 delay) { m_pickupDelay = delay; }

    /**
     * @brief 检查是否可以被拾取
     */
    [[nodiscard]] bool canBePickedUp() const { return m_pickupDelay <= 0; }

    // ========== 玩家追踪 ==========

    /**
     * @brief 检查是否正在追踪玩家
     */
    [[nodiscard]] bool isBeingTracked() const { return m_trackingPlayer != nullptr; }

    /**
     * @brief 获取追踪的玩家
     */
    [[nodiscard]] Player* getTrackingPlayer() const { return m_trackingPlayer; }

    // ========== 合并 ==========

    /**
     * @brief 尝试与另一个经验球合并
     *
     * 如果两个经验球距离足够近，会合并成一个。
     *
     * @param other 另一个经验球
     * @return 是否成功合并
     */
    bool tryMergeWith(ExperienceOrbEntity& other);

    /**
     * @brief 检查是否可以与另一个经验球合并
     */
    [[nodiscard]] bool canMergeWith(const ExperienceOrbEntity& other) const;

    // ========== 拾取 ==========

    /**
     * @brief 处理与玩家的碰撞
     *
     * 检查拾取条件并给予玩家经验。
     *
     * @param player 碰撞的玩家
     */
    void onCollideWithPlayer(Player& player) override;

    // ========== 静态工具方法 ==========

    /**
     * @brief 获取经验分割值
     *
     * 将大量经验分割成适当大小的经验球。
     *
     * @param totalXp 总经验值
     * @return 单个经验球应有的经验值
     */
    static i32 getXPSplit(i32 totalXp);

private:
    /**
     * @brief 更新物理运动
     */
    void _updateMovement();

    /**
     * @brief 追踪最近的玩家
     */
    void _followNearestPlayer();

    /**
     * @brief 查找最近的玩家
     * @return 最近的玩家指针，如果没有则返回 nullptr
     */
    [[nodiscard]] Player* _findNearestPlayer() const;

    /**
     * @brief 处理经验修补附魔
     *
     * 如果玩家有损坏的经修补附魔装备，用经验修复它。
     *
     * @param player 玩家
     * @return 是否使用了经验修补
     */
    bool _handleMending(Player& player);

    /**
     * @brief 给予玩家经验
     *
     * @param player 玩家
     * @return 实际给予的经验值
     */
    i32 _giveExperienceToPlayer(Player& player);

    /**
     * @brief 初始化数据参数
     */
    void _initData();

    i32 m_xpValue = 1;                  // 经验值
    i32 m_age = 0;                      // 存活时间 (ticks)
    i32 m_pickupDelay = 0;              // 拾取延迟 (MC原版默认为0)
    i32 m_health = 5;                   // 生命值（可被攻击摧毁）
    Player* m_trackingPlayer = nullptr; // 追踪的玩家

    // 玩家搜索缓存：每 20 + entityId % 100 ticks 搜索一次
    i32 m_tickCounter = 0;    // 每tick递增的计数器
    i32 m_lastSearchTick = 0; // 上次搜索玩家时的 tick 值
};

} // namespace mc
