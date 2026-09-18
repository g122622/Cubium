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
     * TODO: 尚未实现。MC 的 DAY 时间线以 24000 ticks 为周期驱动天空颜色、雾效、
     * 环境光等属性，需先补齐对应的 EnvironmentAttribute 再填充关键帧。
     */
    [[nodiscard]] static const Timeline& DAY();

    /**
     * @brief 月相时间线
     *
     * TODO: 尚未实现。MC 的 MOON 时间线以 8 天为周期驱动月相属性，
     * 需先补齐 MOON_PHASE 类型的 EnvironmentAttribute。
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
     * TODO: 尚未实现。MC 的 EARLY_GAME 时间线驱动早期游戏阶段相关属性，
     * 需先补齐对应的 EnvironmentAttribute。
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
