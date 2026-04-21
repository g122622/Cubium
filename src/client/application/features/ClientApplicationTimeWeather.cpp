#include "../ClientApplication.hpp"

#include "common/perfetto/TraceEvents.hpp"

namespace mc::client {

void ClientApplication::updateTimeAndWeather(f32 deltaTime)
{
    if (!m_renderer) {
        return;
    }

    MC_TRACE_EVENT("rendering.frame", "UpdateTime");

    constexpr i64 DAY_LENGTH_TICKS = 24000;

    // 每帧推进时间（无论是否有服务端同步）
    // 这确保天空、太阳、月亮在每帧平滑变化
    m_renderTickAccumulator += deltaTime * 20.0f;
    while (m_renderTickAccumulator >= 1.0f) {
        m_renderTickAccumulator -= 1.0f;
        ++m_renderGameTime;
        m_renderDayTime = (m_renderDayTime + 1) % DAY_LENGTH_TICKS;
    }

    // 当有服务端同步时，逐渐纠正到服务端时间（避免跳变）
    if (m_hasServerTimeSync) {
        const i64 serverDayTime = m_world.dayTime();
        const i64 serverGameTime = m_world.gameTime();

        // 计算时间差（处理 dayTime 循环）
        i64 dayTimeDiff = serverDayTime - m_renderDayTime;
        if (dayTimeDiff > DAY_LENGTH_TICKS / 2) {
            dayTimeDiff -= DAY_LENGTH_TICKS;
        } else if (dayTimeDiff < -DAY_LENGTH_TICKS / 2) {
            dayTimeDiff += DAY_LENGTH_TICKS;
        }

        // 平滑纠正：每帧纠正差值的 1%，避免跳变（TODO:根据用户设定的帧率调整纠正率。CORRECTION_RATE = 1 / 用户设定FPS）
        constexpr f32 CORRECTION_RATE = 0.01f;
        const i64 correction = static_cast<i64>(dayTimeDiff * CORRECTION_RATE);
        if (correction != 0) {
            m_renderDayTime = (m_renderDayTime + correction + DAY_LENGTH_TICKS) % DAY_LENGTH_TICKS;
        }

        // gameTime 同步纠正
        i64 gameTimeDiff = serverGameTime - m_renderGameTime;
        m_renderGameTime += static_cast<i64>(gameTimeDiff * CORRECTION_RATE);
    }

    m_renderer->updateTime(m_renderDayTime, m_renderGameTime, m_renderTickAccumulator);

    // 更新天气状态到渲染器
    m_renderer->updateWeather(
        m_world.weather().rainStrength(m_renderTickAccumulator),
        m_world.weather().thunderStrength(m_renderTickAccumulator)
    );
}

} // namespace mc::client