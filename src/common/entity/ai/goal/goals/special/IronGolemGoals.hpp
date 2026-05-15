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

#include "core/Types.hpp"
#include "core/EnumSet.hpp"
#include "../../Goal.hpp"
#include <memory>

namespace mc {

// Forward declarations
class IronGolemEntity;

namespace entity {
class VillagerEntity;
}

namespace entity::ai::goal {

/**
 * @brief 铁傀儡给村民展示花朵目标
 *
 * 铁傀儡偶尔会看向附近的村民并展示手中的罂粟花。
 * 只在白天执行，概率为 1/8000。
 *
 * 参考 MC 1.16.5 ShowVillagerFlowerGoal
 */
class ShowVillagerFlowerGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param ironGolem 铁傀儡实体
     */
    explicit ShowVillagerFlowerGoal(IronGolemEntity* ironGolem);

    ~ShowVillagerFlowerGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;
    [[nodiscard]] std::string getTypeName() const override { return "ShowVillagerFlowerGoal"; }

private:
    IronGolemEntity* m_ironGolem;
    entity::VillagerEntity* m_villager = nullptr;
    i32 m_lookTime = 0;

    // MC 1.16.5 常量
    static constexpr f32 SEARCH_RANGE = 6.0f;      // 搜索村民范围
    static constexpr f32 SEARCH_HEIGHT = 2.0f;     // 搜索村民高度
    static constexpr i32 LOOK_DURATION = 400;      // 看向持续时间（ticks = 20秒）
    static constexpr i32 CHANCE = 8000;            // 执行概率倒数（1/8000）
};

} // namespace entity::ai::goal
} // namespace mc
