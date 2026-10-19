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

#include "common/world/timeline/Timeline.hpp"

namespace mc {
namespace world {
namespace timeline {

/**
 * @brief 预定义时间线
 *
 * MC 1.21.11 class Timelines。
 * 列举顺序与 RegistryDataBuilder 中 minecraft:timeline 注册表的条目顺序保持一致：
 * DAY(0), MOON(1), VILLAGER_SCHEDULE(2), EARLY_GAME(3)。
 */
class Timelines {
public:
    /**
     * @brief 日夜周期时间线
     *
     * 周期 24000 ticks（一整天），驱动太阳/月亮/星空角度、雾色、天空色、
     * 天空光照、日出日落色、星空亮度、眼眸花开合、嘎枝激活、海龟蛋孵化概率、
     * 猫送礼概率、蜜蜂归巢与怪物日燃等属性。
     */
    [[nodiscard]] static const Timeline& DAY();

    /**
     * @brief 月相时间线
     *
     * 周期 8 天（8 * 24000 ticks），驱动月相属性与地表史莱姆生成概率。
     */
    [[nodiscard]] static const Timeline& MOON();

    /**
     * @brief 村民日程
     *
     * 周期 24000 ticks（一整天），含两条轨道：
     * 成年村民活动（VILLAGER_ACTIVITY）与幼年村民活动（BABY_VILLAGER_ACTIVITY）。
     */
    [[nodiscard]] static const Timeline& VILLAGER_SCHEDULE();

    /**
     * @brief 早期游戏阶段时间线
     *
     * 无周期（以世界累积时间驱动）：在前 120000 ticks 内禁止生成劫掠巡逻队。
     */
    [[nodiscard]] static const Timeline& EARLY_GAME();

    /**
     * @brief 预热全部预定义时间线
     *
     * 在服务端初始化阶段调用，确保各时间线在首次使用前完成构建。
     */
    static void bootstrap();
};

} // namespace timeline
} // namespace world
} // namespace mc
