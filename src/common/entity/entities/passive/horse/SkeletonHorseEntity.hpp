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

#include "AbstractHorseEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include <memory>

namespace mc {

// Forward declaration
namespace entity::ai::goal {
class TriggerSkeletonTrapGoal;
}

/**
 * @brief 骷髅马实体
 *
 * 由骷髅骑乘的亡灵马，在雷暴天气中生成。
 *
 * 特性：
 * - 可骑乘：可直接骑乘，无需驯服
 * - 不死生物：免疫溺水、中毒
 * - 不在阳光下燃烧：MC 原版中骷髅马不在 BURN_IN_DAYLIGHT 标签中
 * - 不繁殖：无法繁殖
 * - 陷阱触发：玩家接近时触发陷阱，生成骷髅骑手
 * - 捕获：击败骷髅骑手后可以骑乘
 */
class SkeletonHorseEntity : public AbstractHorseEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    SkeletonHorseEntity(EntityInstanceId id);
    ~SkeletonHorseEntity() override = default;

    // 禁止拷贝
    SkeletonHorseEntity(const SkeletonHorseEntity&) = delete;
    SkeletonHorseEntity& operator=(const SkeletonHorseEntity&) = delete;

    // 允许移动
    SkeletonHorseEntity(SkeletonHorseEntity&&) = delete;
    SkeletonHorseEntity& operator=(SkeletonHorseEntity&&) = delete;

    /**
     * @brief 创建骷髅马实体
     * @param world 世界实例
     * @return 新的骷髅马实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 骑乘系统 ==========

    /**
     * @brief 检查玩家是否可以骑乘
     * 骷髅马不需要驯服即可骑乘
     */
    [[nodiscard]] bool canBeRiddenBy(Player* player) const;

    // ========== 繁殖系统 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * 骷髅马不能繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override
    {
        (void)itemStack;
        return false;
    }

    /**
     * @brief 生成幼体
     * 骷髅马不能繁殖
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override
    {
        (void)partner;
        return nullptr;
    }

    // ========== 不死生物特性 ==========

    /**
     * @brief 是否免疫溺水
     */
    [[nodiscard]] bool canBreatheUnderwater() const override { return true; }

    /**
     * @brief 骷髅马不能吃草
     */
    [[nodiscard]] bool canEatGrass() const override { return false; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.6f; }

    // ========== 骷髅马陷阱 ==========

    /**
     * @brief 是否为陷阱马
     *
     * 陷阱马在雷暴时生成，当玩家接近时会触发：
     * - 生成一只骷髅骑手
     * - 变成普通骷髅马
     */
    [[nodiscard]] bool isTrap() const { return m_trap; }

    /**
     * @brief 设置陷阱马状态
     *
     * 设置为陷阱马时，需要添加 TriggerSkeletonTrapGoal 到目标选择器
     * 取消陷阱马时，需要移除 TriggerSkeletonTrapGoal
     */
    void setTrap(bool trap);

    /**
     * @brief 触发陷阱
     *
     * 当玩家接近陷阱马时调用：
     * - 生成骷髅骑手
     * - 将陷阱马转换为普通骷髅马
     */
    void triggerTrap();

    /**
     * @brief 当被闪电击中时调用
     *
     * 陷阱马被闪电击中时触发陷阱
     */
    void onStruckByLightning() override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_trap = false;                                             // 是否为陷阱马
    i32 m_trapTime = 0;                                              // 陷阱存活时间 (ticks)
    static constexpr i32 TRAP_MAX_TIME = 18000;                      // 陷阱最大存活时间 (15分钟)
    entity::ai::goal::TriggerSkeletonTrapGoal* m_trapGoal = nullptr; // 陷阱触发目标（不拥有所有权）
};

} // namespace mc
