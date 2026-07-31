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

#include "AbstractRaiderEntity.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 劫掠兽实体
 *
 * 大型敌对生物，与掠夺者一起参与掠夺事件。
 *
 * 特性：
 * - 强力攻击：高伤害近战攻击
 * - 咆哮：对周围实体造成伤害和击退
 * - 眩晕：攻击后有概率进入眩晕状态
 * - 破坏方块：可以破坏树叶方块
 * - 掠夺：参与掠夺事件
 * - 骑乘：可被掠夺者骑乘
 */
class RavagerEntity : public AbstractRaiderEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    RavagerEntity(EntityInstanceId id);
    ~RavagerEntity() override = default;

    // 禁止拷贝
    RavagerEntity(const RavagerEntity&) = delete;
    RavagerEntity& operator=(const RavagerEntity&) = delete;

    // 允许移动
    RavagerEntity(RavagerEntity&&) = delete;
    RavagerEntity& operator=(RavagerEntity&&) = delete;

    /// 本类继承链标识（parent = AbstractRaiderEntity::classInfo()）。见 Entity::classInfo()。
    // 透传层无自身同步字段，classInfo 仅作父链遍历节点。
    static const entity::EntityClassInfo& classInfo();

    /**
     * @brief 创建劫掠兽实体
     * @param world 世界实例
     * @return 新的劫掠兽实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 攻击系统 ==========

    /**
     * @brief 是否正在攻击
     */
    [[nodiscard]] bool isAttacking() const { return m_attackTick > 0; }

    /**
     * @brief 获取攻击动画 tick
     */
    [[nodiscard]] i32 getAttackTick() const { return m_attackTick; }

    /**
     * @brief 是否正在眩晕
     */
    [[nodiscard]] bool isStunned() const { return m_stunTick > 0; }

    /**
     * @brief 获取眩晕 tick
     */
    [[nodiscard]] i32 getStunTick() const { return m_stunTick; }

    /**
     * @brief 是否正在咆哮
     */
    [[nodiscard]] bool isRoaring() const { return m_roarTick > 0; }

    /**
     * @brief 获取咆哮 tick
     */
    [[nodiscard]] i32 getRoarTick() const { return m_roarTick; }

    /**
     * @brief 攻击目标实体
     *
     * 设置攻击动画，播放音效
     * @param target 目标实体
     * @return 是否攻击成功
     */
    bool attackEntityAsMob(LivingEntity& target) override;

    /**
     * @brief 构造击退向量
     *
     * 攻击目标后，有 50% 概率眩晕或发射目标
     * @param target 目标实体
     */
    void constructKnockBackVector(LivingEntity* target);

    // ========== 骑乘系统 ==========

    /**
     * @brief 是否正在被骑乘
     */
    [[nodiscard]] bool hasRider() const { return m_rider != nullptr; }

    /**
     * @brief 获取骑乘者
     */
    [[nodiscard]] Entity* getRider() const { return m_rider; }

    /**
     * @brief 设置骑乘者
     */
    void setRider(Entity* rider) { m_rider = rider; }

    // ========== 破坏系统 ==========

    /**
     * @brief 是否可以破坏方块
     */
    [[nodiscard]] bool canBreakBlocks() const { return m_canBreakBlocks; }

    /**
     * @brief 设置是否可以破坏方块
     */
    void setCanBreakBlocks(bool canBreak) { m_canBreakBlocks = canBreak; }

    // ========== 属性 ==========

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 1.95f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 2.2f; }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 2.05f; }

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 检查移动是否被阻塞
     *
     * 当攻击、眩晕或咆哮时不能移动
     */
    [[nodiscard]] bool isMovementBlocked() const;

    /**
     * @brief 检查是否能看见目标
     *
     * 眩晕或咆哮时不能看见目标
     */
    [[nodiscard]] bool canSee(const Entity& other) const override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    // ========== 私有方法 ==========

    /**
     * @brief 执行咆哮攻击
     *
     * 对周围 4 格内的实体造成 6 点伤害并击退
     * 掠夺者类实体免疫伤害
     */
    void _roar();

    /**
     * @brief 发射实体（击退效果）
     *
     * 将实体向远离劫掠兽的方向发射
     * @param entity 要发射的实体
     */
    void _launchEntity(Entity* entity);

    /**
     * @brief 眩晕粒子效果
     *
     * 眩晕时显示粒子效果
     */
    void _spawnStunParticles();

    /**
     * @brief 破坏碰撞到的树叶方块
     *
     * 当水平碰撞且 mobGriefing 为 true 时破坏树叶
     */
    void _breakLeavesOnCollision();

    // ========== 成员变量 ==========

    // 攻击状态
    i32 m_attackTick = 0; // 攻击动画 tick
    i32 m_stunTick = 0;   // 眩晕 tick
    i32 m_roarTick = 0;   // 咆哮 tick

    // 骑乘
    Entity* m_rider = nullptr;

    // 破坏
    bool m_canBreakBlocks = true;

    // 常量 - 公开用于测试
public:
    static constexpr i32 ATTACK_DURATION = 10;  // 攻击动画持续时间 (ticks)
    static constexpr i32 STUN_DURATION = 40;    // 眩晕持续时间 (ticks)
    static constexpr i32 ROAR_DURATION = 20;    // 咆哮持续时间 (ticks)
    static constexpr f32 ATTACK_DAMAGE = 12.0f; // 攻击伤害
    static constexpr f32 ROAR_DAMAGE = 6.0f;    // 咆哮伤害
    static constexpr f32 ROAR_RANGE = 4.0f;     // 咆哮范围
    static constexpr f32 LAUNCH_POWER = 4.0f;   // 发射力度
    static constexpr f32 LAUNCH_Y_POWER = 0.2f; // 发射 Y 轴力度
    static constexpr f32 STUN_CHANCE = 0.5;     // 眩晕概率
};

} // namespace mc
