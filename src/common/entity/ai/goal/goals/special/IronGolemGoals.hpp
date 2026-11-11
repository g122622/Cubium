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

#include "common/entity/core/EntityReference.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include <string>

namespace mc {

// Forward declarations
class IronGolemEntity;
class LivingEntity;
class MobEntity;
class Player;

namespace entity {
class VillagerEntity;
}

namespace test {
class OfferFlowerGoalTestAccessor; // 测试访问器，声明为 friend 以访问 private 成员
} // namespace test

namespace entity::ai::goal {

/**
 * @brief 铁傀儡赠花目标
 *
 * 铁傀儡偶尔会看向附近的候选实体并展示手中的罂粟花。
 * 只在室外明亮时执行，概率为 1/8000。
 *
 * 候选实体由 EntityTypeTags::CANDIDATE_FOR_IRON_GOLEM_GIFT 决定（村民 + 铜傀儡）。
 * 当赠花计时器自然结束（非被抢占中断）且目标属于
 * EntityTypeTags::ACCEPTS_IRON_GOLEM_GIFT（铜傀儡）时，
 * 若铜傀儡的天线槽（EquipmentSlot::Saddle）为空且与铁傀儡的碰撞盒相交，
 * 会将罂粟花装备到铜傀儡的天线槽并标记为保整掉落，
 * 后续铜傀儡转雕像时会自动掉落。
 */
class OfferFlowerGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param ironGolem 铁傀儡实体
     */
    explicit OfferFlowerGoal(IronGolemEntity* ironGolem);

    ~OfferFlowerGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "OfferFlowerGoal";
    }

private:
    /**
     * @brief 在铁傀儡附近的赠花候选实体中查找最近的目标
     *
     * 使用 EntityTypeTags::CANDIDATE_FOR_IRON_GOLEM_GIFT 标签进行过滤，
     * 搜索范围为铁傀儡碰撞盒向 X/Z 各扩展 6 格、Y 扩展 2 格的 AABB。
     *
     * @return 最近的候选实体指针，未找到返回 nullptr
     */
    [[nodiscard]] LivingEntity* _findNearestCandidate() const;

    /**
     * @brief 获取赠花搜索 AABB
     *
     * 对应 MC 1.21.11 OfferFlowerGoal.getGolemBoundingBox()：
     * golem.getBoundingBox().inflate(6.0, 2.0, 6.0)。
     *
     * @return 搜索用 AABB
     */
    [[nodiscard]] AxisAlignedBB _getGolemSearchBox() const;

    /**
     * @brief 若目标可接受赠花，将罂粟花装备到其天线槽
     *
     * 对应 MC 1.21.11 OfferFlowerGoal.stop() 中的赠花条件块：
     * - tick 自然结束（m_tick == 0）
     * - 目标属于 EntityTypeTags::ACCEPTS_IRON_GOLEM_GIFT
     * - 目标天线槽（EquipmentSlot::Saddle）为空
     * - 铁傀儡搜索 AABB 与目标碰撞盒相交
     *
     * 满足条件时装备罂粟花并标记保整掉落。
     */
    void _tryGiftFlowerToCopperGolem();

    IronGolemEntity* m_ironGolem;
    EntityReference<LivingEntity> m_target;
    i32 m_tick = 0;

    // 赠花持续时间（ticks，对应 MC 1.21.11 OfferFlowerGoal.OFFER_TICKS = 400）
    static constexpr i32 OFFER_TICKS = 400;
    // 执行概率倒数（1/8000）
    static constexpr i32 CHANCE = 8000;
    // 搜索 AABB 扩展量（对应 MC inflate(6.0, 2.0, 6.0)）
    static constexpr f32 SEARCH_EXPAND_XZ = 6.0f;
    static constexpr f32 SEARCH_EXPAND_Y = 2.0f;

    friend class test::OfferFlowerGoalTestAccessor;
};

/**
 * @brief 铁傀儡保护村庄目标选择器
 *
 * 当村民被攻击时，铁傀儡会攻击攻击者。
 * 铁傀儡会记住攻击村民的实体并追击。
 */
class DefendVillageTargetGoal : public TargetGoal {
public:
    /**
     * @brief 构造函数
     * @param golem 拥有此目标的铁傀儡
     */
    DefendVillageTargetGoal(IronGolemEntity* golem);

    ~DefendVillageTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "DefendVillageTargetGoal";
    }

private:
    IronGolemEntity* m_golem;
    EntityReference<LivingEntity> m_villageAggressor;
};

} // namespace entity::ai::goal
} // namespace mc
