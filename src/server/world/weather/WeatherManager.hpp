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
#include "common/util/math/random/Random.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/weather/WeatherState.hpp"
#include "common/world/weather/WeatherUtils.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <utility>

// 前向声明
namespace mc {
class BlockPos;
class IWorld;

namespace world::chunk {
class ChunkData;
} // namespace world::chunk
} // namespace mc

namespace mc::server {

// 从 weather 命名空间导入类型
using mc::weather::WeatherState;
using mc::weather::WeatherType;

/**
 * @brief 天气变化回调类型
 *
 * 参数：旧天气类型，新天气类型
 */
using WeatherChangeCallback = std::function<void(WeatherType, WeatherType)>;

/**
 * @brief 服务端天气管理器
 *
 * 负责管理世界天气状态，包括：
 * - 天气周期 tick 更新
 * - 天气命令处理
 * - 闪电生成
 * - 天气同步通知
 *
 * 使用示例：
 * @code
 * WeatherManager weather;
 * weather.initialize(seed, true);
 *
 * // 主循环
 * while (running) {
 *     weather.tick();
 *
 *     // 检查天气变化
 *     if (weather.hasWeatherChanged()) {
 *         broadcastWeatherUpdate();
 *     }
 * }
 *
 * // 命令处理
 * weather.setClear(6000);  // /weather clear 300
 * @endcode
 */
class WeatherManager {
public:
    /**
     * @brief 构造函数
     */
    WeatherManager();

    /**
     * @brief 析构函数
     */
    ~WeatherManager();

    // 禁止拷贝
    WeatherManager(const WeatherManager&) = delete;
    WeatherManager& operator=(const WeatherManager&) = delete;

    // 允许移动
    WeatherManager(WeatherManager&&) noexcept = default;
    WeatherManager& operator=(WeatherManager&&) noexcept = default;

    // ========== 初始化 ==========

    /**
     * @brief 初始化天气管理器
     *
     * 新世界的计时器与原版一致从 0 开始：首个 tick 会按 RAIN_DELAY /
     * THUNDER_DELAY 采样第一次降雨/雷暴的等待时间。
     *
     * @param seed 随机种子
     * @param canHaveWeather 该维度是否允许天气演算
     *        （有天空光照、无天花板、且不是末地）
     */
    void initialize(u64 seed, bool canHaveWeather);

    // ========== 天气状态查询 ==========

    /**
     * @brief 该维度是否允许天气演算
     *
     * 对应原版 Level.canHaveWeather：有天空光照、无天花板、且不是末地。
     */
    [[nodiscard]] bool canHaveWeather() const
    {
        return m_canHaveWeather;
    }

    /**
     * @brief 获取当前天气状态
     */
    [[nodiscard]] const WeatherState& state() const
    {
        return m_state;
    }

    /**
     * @brief 是否正在降雨（强度检查）
     *
     * 无天气演算能力的维度恒为 false（对应原版 Level.isRaining 的 canHaveWeather 门控）。
     */
    [[nodiscard]] bool isRaining() const
    {
        return m_canHaveWeather && m_state.isRaining();
    }

    /**
     * @brief 是否正在雷暴（强度检查）
     *
     * 无天气演算能力的维度恒为 false（对应原版 Level.isThundering 的 canHaveWeather 门控）。
     */
    [[nodiscard]] bool isThundering() const
    {
        return m_canHaveWeather && m_state.isThundering();
    }

    /**
     * @brief 获取降雨强度
     *
     * @param partialTick 部分 tick (0.0 - 1.0)
     * @return 插值后的强度值
     */
    [[nodiscard]] f32 rainStrength(f32 partialTick = 0.0f) const
    {
        return m_state.getRainStrength(partialTick);
    }

    /**
     * @brief 获取雷暴强度（已乘降雨强度，对应原版 Level.getThunderLevel）
     *
     * @param partialTick 部分 tick (0.0 - 1.0)
     * @return 插值后的有效强度值
     */
    [[nodiscard]] f32 thunderStrength(f32 partialTick = 0.0f) const
    {
        return m_state.getThunderStrength(partialTick);
    }

    /**
     * @brief 获取原始雷暴强度（未乘降雨强度）
     *
     * 网络广播 ThunderStrengthChange 时使用，对应原版发送 level.thunderLevel 字段。
     *
     * @param partialTick 部分 tick (0.0 - 1.0)
     * @return 插值后的原始强度值
     */
    [[nodiscard]] f32 rawThunderStrength(f32 partialTick = 0.0f) const
    {
        return m_state.getRawThunderStrength(partialTick);
    }

    /**
     * @brief 获取当前天气类型
     *
     * 无天气演算能力的维度恒为晴天。
     */
    [[nodiscard]] WeatherType weatherType() const
    {
        return m_canHaveWeather ? m_state.weatherType() : WeatherType::Clear;
    }

    /**
     * @brief 检查天气是否发生变化
     *
     * 在 tick() 后检查，仅当降雨/雷暴状态切换时返回 true
     */
    [[nodiscard]] bool hasWeatherChanged() const
    {
        return m_weatherChanged;
    }

    /**
     * @brief 检查天气强度是否发生变化
     *
     * 在 tick() 后检查，强度变化时返回 true
     */
    [[nodiscard]] bool hasStrengthChanged() const
    {
        return m_strengthChanged;
    }

    // ========== 天气命令 ==========

    /**
     * @brief 设置晴天
     *
     * 对应 /weather clear [duration]。未提供 duration 时按 RAIN_DELAY 采样，
     * 与 /weather clear 的缺省行为一致。
     *
     * @param duration 持续时间 (ticks)，<= 0 表示未提供
     * @return 实际应用持续时间 (ticks)
     */
    i32 setClear(i32 duration);

    /**
     * @brief 设置降雨
     *
     * 对应 /weather rain [duration]。未提供 duration 时按 RAIN_DURATION 采样。
     * rainTime 与 thunderTime 都被写成同一 duration（雷暴计时器用于随后结束降雨）。
     *
     * @param duration 持续时间 (ticks)，<= 0 表示未提供
     * @return 实际应用持续时间 (ticks)
     */
    i32 setRain(i32 duration);

    /**
     * @brief 设置雷暴
     *
     * 对应 /weather thunder [duration]。未提供 duration 时按 THUNDER_DURATION 采样。
     * 雷暴会自动启用降雨，rainTime 与 thunderTime 都被写成同一 duration。
     *
     * @param duration 持续时间 (ticks)，<= 0 表示未提供
     * @return 实际应用持续时间 (ticks)
     */
    i32 setThunder(i32 duration);

    /**
     * @brief 从存档恢复天气状态
     *
     * 直接写入 level.dat 中保存的四个天气字段，并按原版 prepareWeather 的规则
     * 恢复强度：正在下雨时降雨强度置 1，正在雷暴时雷暴强度置 1。
     *
     * @param clearWeatherTime 强制晴天剩余时间
     * @param rainTime 降雨剩余时间
     * @param raining 是否正在下雨
     * @param thunderTime 雷暴剩余时间
     * @param thundering 是否正在雷暴
     */
    void restoreState(i32 clearWeatherTime, i32 rainTime, bool raining, i32 thunderTime, bool thundering);

    /**
     * @brief 重置天气周期（玩家睡醒后调用）
     *
     * 对应原版 ServerLevel.resetWeatherCycle：清空降雨/雷暴计时器与状态标志，
     * 保留 clearWeatherTime，强度随后自然渐变到 0。
     */
    void resetWeather();

    // ========== 游戏规则 ==========

    /**
     * @brief 设置天气周期是否启用
     *
     * 对应游戏规则 doWeatherCycle
     *
     * @param enabled 是否启用
     */
    void setWeatherCycleEnabled(bool enabled)
    {
        m_state.weatherCycleEnabled = enabled;
    }

    /**
     * @brief 获取天气周期是否启用
     */
    [[nodiscard]] bool weatherCycleEnabled() const
    {
        return m_state.weatherCycleEnabled;
    }

    // ========== 主循环 ==========

    /**
     * @brief 执行一个 tick
     *
     * 更新天气计时器和强度渐变。
     * 如果天气周期启用，会自动处理天气变化。
     */
    void tick();

    // ========== 闪电落点 ==========

    /**
     * @brief 计算一次闪电的落点
     *
     * 对应原版 ServerLevel.findLightningTargetAround：先按 MOTION_BLOCKING 高度图
     * 取随机列的最高方块，再优先寻找 128 格内位于世界表面的避雷针，其次在膨胀 3 格的
     * 柱状范围内随机挑一个可见天空的生物。最终落点必须允许降水，否则返回 nullopt。
     *
     * 概率判定（1/100000）与随机列生成由调用方（ServerWorld）完成，本方法只负责落点。
     *
     * @param world 世界
     * @param chunk 随机列所在区块
     * @param randomPos 随机列（X/Z 位于 chunk 内，Y 无意义）
     * @return 闪电落点；无有效落点时为 nullopt
     */
    [[nodiscard]] std::optional<BlockPos> findLightningTarget(
        mc::IWorld& world, const mc::world::chunk::ChunkData& chunk, const BlockPos& randomPos);

    /**
     * @brief 设置天气变化回调
     *
     * @param callback 天气变化时调用的回调函数
     */
    void setWeatherChangeCallback(WeatherChangeCallback callback)
    {
        m_weatherChangeCallback = std::move(callback);
    }

private:
    /**
     * @brief 在 128 格内查找位于世界表面的避雷针
     *
     * 对应原版 ServerLevel.findLightningRod：要求避雷针所在 Y 等于该列
     * WORLD_SURFACE 高度图的最高方块 Y，并在多个候选中取距离最近者。
     *
     * @param world 世界
     * @param pos 搜索中心（闪电随机列的最高方块位置）
     * @return 避雷针上方一格（闪电落点）；未找到时为 nullopt
     */
    [[nodiscard]] std::optional<BlockPos> _findLightningRod(mc::IWorld& world, const BlockPos& pos) const;

    // ========== 内部方法 ==========
    /**
     * @brief 更新天气强度渐变
     */
    void _updateStrength();

    /**
     * @brief 处理天气周期逻辑
     */
    void _tickWeatherCycle();

    /**
     * @brief 检查天气变化并触发回调
     */
    void _checkWeatherChange();

private:
    WeatherState m_state;                        ///< 天气状态
    std::unique_ptr<mc::math::IRandom> m_random; ///< 随机数生成器
    bool m_canHaveWeather = false;               ///< 维度是否允许天气演算

    // 变化标志（每tick后重置）
    bool m_weatherChanged = false;  ///< 降雨/雷暴状态是否变化
    bool m_strengthChanged = false; ///< 强度是否变化

    // 回调
    WeatherChangeCallback m_weatherChangeCallback;

    // 天气变化检测状态
    bool m_lastRaining = false; ///< 上一帧的降雨状态
};

} // namespace mc::server
