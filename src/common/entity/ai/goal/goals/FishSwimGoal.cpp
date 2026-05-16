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

#include "FishSwimGoal.hpp"
#include "../../../entities/passive/fish/AbstractFishEntity.hpp"

namespace mc::entity::ai::goal {

FishSwimGoal::FishSwimGoal(AbstractFishEntity* fish)
    : RandomSwimmingGoal(fish, 1.0, 40)
    , m_fish(fish)
{
    // MC 1.16.5 AbstractFishEntity.SwimGoal:
    // super(fish, 1.0D, 40)
}

FishSwimGoal::FishSwimGoal(AbstractFishEntity* fish, f64 speed, i32 chance)
    : RandomSwimmingGoal(fish, speed, chance)
    , m_fish(fish)
{
}

bool FishSwimGoal::shouldExecute()
{
    // MC 1.16.5 AbstractFishEntity.SwimGoal.shouldExecute():
    // return this.fish.func_212800_dy() && super.shouldExecute();
    //
    // func_212800_dy() 对应 canRandomSwim()：
    // - AbstractFishEntity: 返回 true
    // - AbstractGroupFishEntity: 返回 !hasGroupLeader()
    //
    // 群游鱼类只有在没有群首时才会自主游泳

    if (m_fish == nullptr) {
        return false;
    }

    // 检查是否可以随机游泳
    if (!m_fish->canRandomSwim()) {
        return false;
    }

    // 调用父类的 shouldExecute
    return RandomSwimmingGoal::shouldExecute();
}

} // namespace mc::entity::ai::goal
