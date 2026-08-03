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
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include <string>

namespace mc {
namespace entity {
class VillagerEntity;

namespace ai {
namespace goal {
namespace villager {

/**
 * @brief 村民繁殖目标
 *
 * 村民繁殖行为，需要足够的食物和床位。
 * 村民会寻找愿意繁殖的配偶，靠近后生成幼年村民。
 */
class VillagerBreedGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param villager 村民实体
     */
    explicit VillagerBreedGoal(VillagerEntity* villager);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;
    [[nodiscard]] std::string getTypeName() const override { return "VillagerBreedGoal"; }

private:
    /**
     * @brief 检查是否有足够的床位
     */
    [[nodiscard]] bool _hasEnoughBeds() const;

    /**
     * @brief 检查是否愿意繁殖
     */
    [[nodiscard]] bool _isWillingToBreed() const;

    /**
     * @brief 寻找繁殖伙伴
     */
    void _findPartner();

    /**
     * @brief 移动到伙伴
     */
    void _moveToPartner();

    /**
     * @brief 生成幼年村民
     */
    void _spawnChild();

private:
    VillagerEntity* m_villager;
    EntityInstanceId m_partnerId;
    i32 m_breedTicks = 0;
    static constexpr i32 BREED_TICKS = 60; // 繁殖动画时长
    static constexpr f32 BREED_DISTANCE = 2.0f;
};

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
