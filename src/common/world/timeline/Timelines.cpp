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

#include "common/world/timeline/Timelines.hpp"

#include "common/entity/ai/brain/schedule/Activity.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/attribute/EnvironmentAttributes.hpp"
#include "common/world/timeline/KeyframeTrack.hpp"

namespace mc {
namespace world {
namespace timeline {

const Timeline& Timelines::DAY()
{
    // TODO: 尚未实现，见头文件说明
    static const Timeline instance = Timeline::Builder().build();
    return instance;
}

const Timeline& Timelines::MOON()
{
    // TODO: 尚未实现，见头文件说明
    static const Timeline instance = Timeline::Builder().build();
    return instance;
}

const Timeline& Timelines::VILLAGER_SCHEDULE()
{
    using Activity = entity::ai::brain::schedule::Activity;

    static const Timeline instance = Timeline::Builder()
                                         .setPeriodTicks(24000)
                                         .addTrack(attribute::EnvironmentAttributes::VILLAGER_ACTIVITY(),
                                             [](KeyframeTrack<Activity>::Builder& builder) {
                                                 builder.addKeyframe(10, Activity::IDLE)
                                                     .addKeyframe(2000, Activity::WORK)
                                                     .addKeyframe(9000, Activity::MEET)
                                                     .addKeyframe(11000, Activity::IDLE)
                                                     .addKeyframe(12000, Activity::REST);
                                             })
                                         .addTrack(attribute::EnvironmentAttributes::BABY_VILLAGER_ACTIVITY(),
                                             [](KeyframeTrack<Activity>::Builder& builder) {
                                                 builder.addKeyframe(10, Activity::IDLE)
                                                     .addKeyframe(3000, Activity::PLAY)
                                                     .addKeyframe(6000, Activity::IDLE)
                                                     .addKeyframe(10000, Activity::PLAY)
                                                     .addKeyframe(12000, Activity::REST);
                                             })
                                         .build();
    return instance;
}

const Timeline& Timelines::EARLY_GAME()
{
    // TODO: 尚未实现，见头文件说明
    static const Timeline instance = Timeline::Builder().build();
    return instance;
}

void Timelines::bootstrap()
{
    // 触发各预定义时间线的构建（Meyers singleton，首次访问时完成初始化）。
    // 列举顺序与 RegistryDataBuilder 中 minecraft:timeline 注册表的条目顺序一致。
    static const Timeline* const ALL_TIMELINES[] = {&DAY(), &MOON(), &VILLAGER_SCHEDULE(), &EARLY_GAME()};
    MC_UNUSED(ALL_TIMELINES);
}

} // namespace timeline
} // namespace world
} // namespace mc
