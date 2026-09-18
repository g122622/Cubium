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

#include <gtest/gtest.h>

#include "common/entity/ai/brain/schedule/Activity.hpp"
#include "common/world/attribute/AttributeTypes.hpp"
#include "common/world/attribute/EnvironmentAttribute.hpp"
#include "common/world/attribute/EnvironmentAttributes.hpp"
#include "common/world/attribute/LerpFunction.hpp"
#include "common/world/timeline/AttributeTrackSampler.hpp"
#include "common/world/timeline/KeyframeTrack.hpp"
#include "common/world/timeline/KeyframeTrackSampler.hpp"
#include "common/world/timeline/Timeline.hpp"
#include "common/world/timeline/Timelines.hpp"

#include <memory>
#include <optional>
#include <ostream>
#include <string>

namespace mc::entity::ai::brain::schedule {

/// 供 gtest 在断言失败时打印活动名
void PrintTo(const Activity& activity, std::ostream* os)
{
    *os << activity.getKey();
}

} // namespace mc::entity::ai::brain::schedule

namespace mc::world::timeline {
namespace {

using Activity = entity::ai::brain::schedule::Activity;

/**
 * 活动采样器辅助类
 *
 * 包装 AttributeTrackSampler，以外部变量驱动 dayTime，
 * 并用自增 tick 使采样器缓存失效，从而可在测试中查询任意 dayTime。
 */
class ActivitySampler {
public:
    ActivitySampler(const Timeline& timeline, const attribute::EnvironmentAttribute<Activity>& envAttr)
        : m_sampler(timeline.createTrackSampler<Activity, Activity>(envAttr, [this]() { return m_dayTime; }))
    {}

    [[nodiscard]] Activity sample(i64 dayTime)
    {
        m_dayTime = dayTime;
        ++m_tick;
        return m_sampler->applyTimeBased(Activity::IDLE, m_tick);
    }

private:
    i64 m_dayTime = 0;
    i64 m_tick = 0;
    std::unique_ptr<AttributeTrackSampler<Activity, Activity>> m_sampler;
};

// ============================================================================
// KeyframeTrackSampler 基础语义
// ============================================================================

TEST(KeyframeTrackSamplerTest, SingleKeyframeYieldsConstantValue)
{
    const KeyframeTrack<Activity> track =
        KeyframeTrack<Activity>::Builder().addKeyframe(0, Activity::IDLE).build();

    const auto sampler = track.bakeSampler(std::nullopt, attribute::LerpFunction<Activity>::ofStep(1.0f));

    EXPECT_EQ(sampler.sample(0), Activity::IDLE);
    EXPECT_EQ(sampler.sample(1000), Activity::IDLE);
    EXPECT_EQ(sampler.sample(-500), Activity::IDLE);
}

TEST(KeyframeTrackSamplerTest, WithoutPeriodKeepsLastValueAfterEnd)
{
    const KeyframeTrack<Activity> track = KeyframeTrack<Activity>::Builder()
                                              .addKeyframe(0, Activity::IDLE)
                                              .addKeyframe(100, Activity::WORK)
                                              .build();

    const auto sampler = track.bakeSampler(std::nullopt, attribute::LerpFunction<Activity>::ofStep(1.0f));

    EXPECT_EQ(sampler.sample(0), Activity::IDLE);
    EXPECT_EQ(sampler.sample(50), Activity::IDLE);
    EXPECT_EQ(sampler.sample(100), Activity::WORK);
    // 无周期时不回绕，超出末尾保持末值
    EXPECT_EQ(sampler.sample(1000), Activity::WORK);
}

TEST(KeyframeTrackSamplerTest, StepLerpKeepsFromValueUntilNextKeyframe)
{
    // ofStep(1.0)：区间内保持前一关键帧的值，直到抵达下一关键帧
    const KeyframeTrack<Activity> track = KeyframeTrack<Activity>::Builder()
                                              .addKeyframe(10, Activity::IDLE)
                                              .addKeyframe(2000, Activity::WORK)
                                              .build();

    const auto sampler =
        track.bakeSampler(std::optional<i32>(24000), attribute::LerpFunction<Activity>::ofStep(1.0f));

    EXPECT_EQ(sampler.sample(10), Activity::IDLE);
    EXPECT_EQ(sampler.sample(1999), Activity::IDLE);
    EXPECT_EQ(sampler.sample(2000), Activity::WORK);
}

TEST(KeyframeTrackSamplerTest, PeriodWrapsBeforeFirstKeyframe)
{
    // 周期回绕：早于首个关键帧的查询回到上一周期末的关键帧
    const KeyframeTrack<Activity> track = KeyframeTrack<Activity>::Builder()
                                              .addKeyframe(10, Activity::IDLE)
                                              .addKeyframe(2000, Activity::WORK)
                                              .build();

    const auto sampler =
        track.bakeSampler(std::optional<i32>(24000), attribute::LerpFunction<Activity>::ofStep(1.0f));

    EXPECT_EQ(sampler.sample(9), Activity::WORK);
    EXPECT_EQ(sampler.sample(0), Activity::WORK);
    EXPECT_EQ(sampler.sample(24010), Activity::IDLE);
}

// ============================================================================
// 预定义时间线
// ============================================================================

TEST(TimelineTest, VillagerScheduleHasOneDayPeriod)
{
    EXPECT_EQ(Timelines::VILLAGER_SCHEDULE().periodTicks(), std::optional<i32>(24000));
}

TEST(TimelineTest, VillagerScheduleMatchesKeyframeBoundaries)
{
    ActivitySampler sampler(Timelines::VILLAGER_SCHEDULE(), attribute::EnvironmentAttributes::VILLAGER_ACTIVITY());

    // 早于首个关键帧(10)时，回绕到上一周期末尾的 REST
    EXPECT_EQ(sampler.sample(0), Activity::REST);
    EXPECT_EQ(sampler.sample(9), Activity::REST);

    EXPECT_EQ(sampler.sample(10), Activity::IDLE);
    EXPECT_EQ(sampler.sample(1999), Activity::IDLE);
    EXPECT_EQ(sampler.sample(2000), Activity::WORK);
    EXPECT_EQ(sampler.sample(8999), Activity::WORK);
    EXPECT_EQ(sampler.sample(9000), Activity::MEET);
    EXPECT_EQ(sampler.sample(10999), Activity::MEET);
    EXPECT_EQ(sampler.sample(11000), Activity::IDLE);
    EXPECT_EQ(sampler.sample(11999), Activity::IDLE);
    EXPECT_EQ(sampler.sample(12000), Activity::REST);
    EXPECT_EQ(sampler.sample(23999), Activity::REST);
}

TEST(TimelineTest, VillagerScheduleWrapsAcrossDayBoundary)
{
    ActivitySampler sampler(Timelines::VILLAGER_SCHEDULE(), attribute::EnvironmentAttributes::VILLAGER_ACTIVITY());

    // 第二周期应与第一周期一致
    EXPECT_EQ(sampler.sample(24000), Activity::REST);
    EXPECT_EQ(sampler.sample(24010), Activity::IDLE);
    EXPECT_EQ(sampler.sample(26000), Activity::WORK);
    EXPECT_EQ(sampler.sample(33000), Activity::MEET);
}

TEST(TimelineTest, BabyVillagerScheduleUsesPlayActivity)
{
    ActivitySampler sampler(
        Timelines::VILLAGER_SCHEDULE(), attribute::EnvironmentAttributes::BABY_VILLAGER_ACTIVITY());

    EXPECT_EQ(sampler.sample(0), Activity::REST);
    EXPECT_EQ(sampler.sample(10), Activity::IDLE);
    EXPECT_EQ(sampler.sample(3000), Activity::PLAY);
    EXPECT_EQ(sampler.sample(5999), Activity::PLAY);
    EXPECT_EQ(sampler.sample(6000), Activity::IDLE);
    EXPECT_EQ(sampler.sample(10000), Activity::PLAY);
    EXPECT_EQ(sampler.sample(12000), Activity::REST);
}

TEST(TimelineTest, ActivityTypeKeyframeLerpIsStepInterpolated)
{
    // 活动是离散值：使用 AttributeTypes::ACTIVITY() 的 keyframeLerp 时区间内保持前值
    const KeyframeTrack<Activity> track = KeyframeTrack<Activity>::Builder()
                                              .addKeyframe(10, Activity::IDLE)
                                              .addKeyframe(2000, Activity::WORK)
                                              .build();

    const auto sampler = track.bakeSampler(
        std::optional<i32>(24000), attribute::AttributeTypes::ACTIVITY().keyframeLerp());

    EXPECT_EQ(sampler.sample(500), Activity::IDLE);
    EXPECT_EQ(sampler.sample(1999), Activity::IDLE);
    EXPECT_EQ(sampler.sample(2000), Activity::WORK);
}

} // namespace
} // namespace mc::world::timeline
