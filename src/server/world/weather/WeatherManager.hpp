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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/weather/WeatherState.hpp"
#include "common/world/weather/WeatherUtils.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

// 前向声明
namespace mc {
class BlockPos;
class IWorld;
} // namespace mc

namespace mc::server {

// 从 weather 命名空间导入类型
using mc::weather::WeatherState;
using mc::weather::WeatherType;

/**
 * @brief 闪电生成回调类型
 *
 * 参数：闪电位置
 */
using LightningSpawnCallback = std::function<void(const mc::BlockPos&)>;

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
 * WeatherManager weather(world);
 * weather.initialize(seed);
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
     * @param seed 随机种子
     */
    void initialize(u64 seed);

    /**
     * @brief 设置关联的世界
     *
     * @param world 世界指针（不获取所有权）
     */
    void setWorld(mc::IWorld* world) { m_world = world; }

    // ========== 天气状态查询 ==========

    /**
     * @brief 获取当前天气状态
     */
    [[nodiscard]] const WeatherState& state() const { return m_state; }

    /**
     * @brief 是否正在降雨（强度检查）
     */
    [[nodiscard]] bool isRaining() const { return m_state.isRaining(); }

    /**
     * @brief 是否正在雷暴（强度检查）
     */
    [[nodiscard]] bool isThundering() const { return m_state.isThundering(); }

    /**
     * @brief 获取降雨强度
     *
     * @param partialTick 部分 tick (0.0 - 1.0)
     * @return 插值后的强度值
     */
    [[nodiscard]] f32 rainStrength(f32 partialTick = 0.0f) const { return m_state.getRainStrength(partialTick); }

    /**
     * @brief 获取雷暴强度
     *
     * @param partialTick 部分 tick (0.0 - 1.0)
     * @return 插值后的强度值
     */
    [[nodiscard]] f32 thunderStrength(f32 partialTick = 0.0f) const { return m_state.getThunderStrength(partialTick); }

    /**
     * @brief 获取当前天气类型
     */
    [[nodiscard]] WeatherType weatherType() const { return m_state.weatherType(); }

    /**
     * @brief 检查天气是否发生变化
     *
     * 在 tick() 后检查，仅当降雨/雷暴状态切换时返回 true
     */
    [[nodiscard]] bool hasWeatherChanged() const { return m_weatherChanged; }

    /**
     * @brief 检查天气强度是否发生变化
     *
     * 在 tick() 后检查，强度变化时返回 true
     */
    [[nodiscard]] bool hasStrengthChanged() const { return m_strengthChanged; }

    // ========== 天气命令 ==========

    /**
     * @brief 设置晴天
     *
     * 对应 /weather clear [duration]
     *
     * @param duration 持续时间 (ticks)，0 表示使用默认值
     */
    void setClear(i32 duration = 0);

    /**
     * @brief 设置降雨
     *
     * 对应 /weather rain [duration]
     *
     * @param duration 持续时间 (ticks)，0 表示使用默认值
     */
    void setRain(i32 duration = 0);

    /**
     * @brief 设置雷暴
     *
     * 对应 /weather thunder [duration]
     * 注意：雷暴会自动启用降雨
     *
     * @param duration 持续时间 (ticks)，0 表示使用默认值
     */
    void setThunder(i32 duration = 0);

    /**
     * @brief 重置天气（玩家睡觉后调用）
     *
     * 清除所有天气效果，恢复晴天
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
    void setWeatherCycleEnabled(bool enabled) { m_state.weatherCycleEnabled = enabled; }

    /**
     * @brief 获取天气周期是否启用
     */
    [[nodiscard]] bool weatherCycleEnabled() const { return m_state.weatherCycleEnabled; }

    // ========== 主循环 ==========

    /**
     * @brief 执行一个 tick
     *
     * 更新天气计时器和强度渐变。
     * 如果天气周期启用，会自动处理天气变化。
     */
    void tick();

    // ========== 闪电生成 ==========

    /**
     * @brief 尝试生成闪电
     *
     * 雷暴时每tick调用，有概率生成闪电。
     *
     * @return 闪电生成位置，如果没有生成则返回无效位置
     */
    [[nodiscard]] std::pair<bool, BlockPos> trySpawnLightning();

    /**
     * @brief 设置闪电生成回调
     *
     * @param callback 闪电生成时调用的回调函数
     */
    void setLightningCallback(LightningSpawnCallback callback) { m_lightningCallback = std::move(callback); }

    /**
     * @brief 设置天气变化回调
     *
     * @param callback 天气变化时调用的回调函数
     */
    void setWeatherChangeCallback(WeatherChangeCallback callback) { m_weatherChangeCallback = std::move(callback); }

    // ========== 序列化 ==========

    /**
     * @brief 序列化天气状态
     *
     * 用于世界存档
     *
     * @param data 输出数据缓冲区
     */
    void serialize(std::vector<u8>& data) const;

    /**
     * @brief 反序列化天气状态
     *
     * 用于加载世界存档
     *
     * @param data 输入数据缓冲区
     * @param offset 数据偏移
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> deserialize(const std::vector<u8>& data, size_t& offset);

private:
    /**
     * @brief 在指定位置周围寻找闪电目标
     *
     * 查找附近的生物实体，如果找到则返回其位置，否则返回原始位置。
     *
     * @param pos 原始位置
     * @return 调整后的位置
     */
    [[nodiscard]] BlockPos _findLightningTargetAround(const BlockPos& pos) const;

    /**
     * @brief 获取随机方块位置
     *
     * 在区块内生成随机位置。
     *
     * @param chunkX 区块 X 起点（方块坐标）
     * @param sectionY 区块段 Y 起点（方块坐标）
     * @param chunkZ 区块 Z 起点（方块坐标）
     * @return 随机方块位置
     */
    [[nodiscard]] BlockPos _getBlockRandomPos(i32 chunkX, i32 sectionY, i32 chunkZ);

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
    mc::IWorld* m_world = nullptr;               ///< 关联的世界（不拥有）

    // 变化标志（每tick后重置）
    bool m_weatherChanged = false;  ///< 降雨/雷暴状态是否变化
    bool m_strengthChanged = false; ///< 强度是否变化

    // 回调
    LightningSpawnCallback m_lightningCallback;
    WeatherChangeCallback m_weatherChangeCallback;

    // 天气变化检测状态
    bool m_lastRaining = false; ///< 上一帧的降雨状态

    // LCG 状态（用于随机位置生成）
    i64 m_updateLCG = 0; ///< 线性同余生成器状态
};

} // namespace mc::server
