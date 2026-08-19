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

#include "../../../../core/Types.hpp"
#include "../MonsterEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include <memory>

namespace mc {

/**
 * @brief 蜘蛛实体
 *
 * 可以爬墙的敌对生物。蜘蛛具有独特的光照敏感攻击特性：
 * - 只在黑暗中攻击玩家和铁傀儡（光照等级 < 7）
 * - 白天或明亮环境中保持中立
 *
 * 特性：
 * - 爬墙：可以垂直爬上墙壁
 * - 夜间攻击：仅在黑暗中攻击
 * - 白天中立：在明亮环境中不主动攻击
 * - 不燃烧：不在阳光下燃烧
 * - 节肢生物：对节肢杀手附魔免疫效果敏感
 *
 * AI 目标:
 * | 优先级 | Goal | 说明 |
 * |--------|------|------|
 * | 1 | SwimGoal | 游泳 |
 * | 3 | LeapAtTargetGoal | 跳向目标（力度0.4F） |
 * | 4 | SpiderAttackGoal | 近战攻击（带光照条件） |
 * | 5 | WaterAvoidingRandomWalkingGoal | 避水随机行走 |
 * | 6 | LookAtGoal | 看向玩家 |
 * | 6 | LookRandomlyGoal | 随机看向 |
 *
 * 目标选择:
 * | 优先级 | Goal | 说明 |
 * |--------|------|------|
 * | 1 | HurtByTargetGoal | 被攻击后反击 |
 * | 2 | SpiderTargetGoal\<Player\> | 攻击玩家（黑暗条件） |
 * | 3 | SpiderTargetGoal\<IronGolem\> | 攻击铁傀儡（黑暗条件） |
 */
class SpiderEntity : public MonsterEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    SpiderEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~SpiderEntity() override = default;

    // 节肢生物归类（对齐 Java Spider.getMobType()==ARTHROPOD）。基类默认 Undefined，未覆写会导致
    // 节肢杀手(Bane of Arthropods)附魔无加成、节肢类瞬间伤害药水无额外效果等失效。
    // 子类 CaveSpiderEntity 继承此覆写自动归类 Arthropod。
    [[nodiscard]] CreatureAttribute getCreatureAttribute() const override { return CreatureAttribute::Arthropod; }

    // 禁止拷贝
    SpiderEntity(const SpiderEntity&) = delete;
    SpiderEntity& operator=(const SpiderEntity&) = delete;

    // 允许移动
    SpiderEntity(SpiderEntity&&) = delete;
    SpiderEntity& operator=(SpiderEntity&&) = delete;

    /**
     * @brief 创建蜘蛛实体
     * @param world 世界实例
     * @return 新的蜘蛛实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 攀爬系统 ==========

    /**
     * @brief 是否正在攀爬
     */
    [[nodiscard]] bool isClimbing() const { return m_climbing; }

    /**
     * @brief 设置攀爬状态
     */
    void setClimbing(bool climbing) { m_climbing = climbing; }

    /**
     * @brief 是否可以攀爬
     */
    [[nodiscard]] bool canClimb() const { return true; }

    // ========== 攻击状态 ==========

    /**
     * @brief 是否应该攻击
     * 蜘蛛只在黑暗中攻击
     */
    [[nodiscard]] bool shouldAttack(LivingEntity* target) const override;

    // ========== 阳光燃烧 ==========

    /**
     * @brief 蜘蛛不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.65f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 1.4f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 0.9f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    bool m_climbing = false;
    bool m_wasOnGround = false;
};

} // namespace mc
