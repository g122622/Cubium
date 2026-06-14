/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software be
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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

// 此文件原先包含多个AI目标的存根声明。
// 以下目标已迁移到独立文件中完整实现：
//   - EatGrassGoal       -> EatGrassGoal.hpp
//   - HurtByTargetGoal   -> target/TargetGoals.hpp
//   - NearestAttackableTargetGoal -> target/TargetGoals.hpp
//   - FleeSunGoal        -> FleeSunGoal.hpp
//   - FindShelterGoal    -> FindShelterGoal.hpp
//   - RestrictSunGoal    -> RestrictSunGoal.hpp
//   - ReturnToHomeGoal   -> 已合并到 movement/MovementGoals.hpp (MoveTowardsRestrictionGoal)
//   - FlyGoal            -> FlyGoal.hpp
//   - SleepGoal          -> villager/SleepAtNightGoal.hpp
//   - WorkAtPoiGoal      -> villager/WorkAtJobSiteGoal.hpp
//   - TradeWithPlayerGoal -> 由VillagerEntity的交易系统处理
//   - ShowWaresGoal      -> 由VillagerEntity的交易系统处理
//
// 如需使用以上目标，请直接include对应的头文件。

// 此文件保留为空，以保持向后兼容性。
// 未来如果有新的通用目标需要临时存放，可以在此文件中声明。
