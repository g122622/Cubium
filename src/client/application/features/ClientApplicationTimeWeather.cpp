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

#include "client/application/ClientApplication.hpp"

#include "common/core/Constants.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/dimension/DimensionRenderSettings.hpp"
#include "common/world/dimension/MapDimensionId.hpp"

using namespace mc::trace;

namespace mc::client {

void ClientApplication::updateTimeAndWeather(f32 deltaTime)
{
    MC_ASSERT_RELEASE(m_renderer);

    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "UpdateTime");

    constexpr i64 DAY_LENGTH_TICKS = game::DAY_LENGTH_TICKS;

    // 每帧推进时间（无论是否有服务端同步）
    // 这确保天空、太阳、月亮在每帧平滑变化
    m_renderTickAccumulator += deltaTime * 20.0f;
    while (m_renderTickAccumulator >= 1.0f) {
        m_renderTickAccumulator -= 1.0f;
        ++m_renderGameTime;
        m_renderDayTime = (m_renderDayTime + 1) % DAY_LENGTH_TICKS;

        m_world.weather().tickLightningFlash();
    }

    // 当有服务端同步时，逐渐纠正到服务端时间（避免跳变）
    if (m_hasServerTimeSync) {
        const i64 serverDayTime = m_world.dayTimeOfDay();
        const i64 serverGameTime = m_world.gameTime();

        // 计算时间差（处理 dayTime 循环）
        i64 dayTimeDiff = serverDayTime - m_renderDayTime;
        if (dayTimeDiff > DAY_LENGTH_TICKS / 2) {
            dayTimeDiff -= DAY_LENGTH_TICKS;
        } else if (dayTimeDiff < -DAY_LENGTH_TICKS / 2) {
            dayTimeDiff += DAY_LENGTH_TICKS;
        }

        // 使用帧率无关的指数衰减公式进行平滑纠正
        // 每秒纠正约 50% 的差值，在平滑性和响应性之间取得平衡
        constexpr f32 CORRECTION_PER_SECOND = 0.5f;
        const f32 correctionFactor = math::exponentialDecayFactor(CORRECTION_PER_SECOND, deltaTime);
        const i64 correction = static_cast<i64>(dayTimeDiff * correctionFactor);
        if (correction != 0) {
            m_renderDayTime = (m_renderDayTime + correction + DAY_LENGTH_TICKS) % DAY_LENGTH_TICKS;
        }

        // gameTime 同步纠正（使用相同的帧率无关纠正因子）
        i64 gameTimeDiff = serverGameTime - m_renderGameTime;
        m_renderGameTime += static_cast<i64>(gameTimeDiff * correctionFactor);
    }

    m_renderer->updateTime(m_renderDayTime, m_renderGameTime, m_renderTickAccumulator);

    // 更新天气状态到渲染器
    m_renderer->updateWeather(m_world.weather().rainStrength(m_renderTickAccumulator),
        m_world.weather().thunderStrength(m_renderTickAccumulator));

    // 更新闪电闪烁亮度到渲染器
    m_renderer->setLightningFlashBrightness(m_world.weather().lightningFlashBrightness());

    // 更新云高度（根据当前维度）
    updateCloudHeight();
}

void ClientApplication::updateCloudHeight()
{
    MC_ASSERT_RELEASE(m_renderer);

    // 获取当前维度的渲染设置
    const DimensionId currentDim = m_dimensionManager.currentDimension();
    const world::DimensionRenderSettings settings = getDimensionRenderSettings(currentDim);

    // 传递云高度和是否有云到渲染器
    // 注意：由于项目使用 -ffast-math，NaN 检测不可靠，
    // 因此使用显式的 hasClouds 布尔字段
    m_renderer->setCloudHeight(static_cast<f64>(settings.cloudHeight), settings.hasClouds);
}

world::DimensionRenderSettings ClientApplication::getDimensionRenderSettings(DimensionId dimensionId) const
{
    // 根据 DimensionId 返回对应的渲染设置
    if (dimensionId == static_cast<DimensionId>(MapDimensionId::Overworld)) {
        return world::DimensionRenderSettings::overworld();
    } else if (dimensionId == static_cast<DimensionId>(MapDimensionId::Nether)) {
        return world::DimensionRenderSettings::nether();
    } else if (dimensionId == static_cast<DimensionId>(MapDimensionId::End)) {
        return world::DimensionRenderSettings::end();
    }

    // 默认使用主世界设置
    return world::DimensionRenderSettings::overworld();
}

} // namespace mc::client