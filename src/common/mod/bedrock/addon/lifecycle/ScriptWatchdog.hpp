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

#include <chrono>

namespace mc::mod::bedrock::addon {

class ScriptManager;

/**
 * @brief 脚本看门狗
 *
 * 监控脚本执行，检测超时和内存越限。
 * 可以终止运行时间过长的脚本以防止服务器卡死。
 */
class ScriptWatchdog {
public:
    /**
     * @brief 看门狗配置
     */
    struct Config {
        /// 单次tick中脚本最大执行时间（毫秒），默认50ms
        u32 tickTimeLimitMs = 50;
        /// 脚本最大内存使用量（字节），默认64MB
        u64 memoryLimitBytes = 64 * 1024 * 1024;
        /// 是否启用看门狗
        bool enabled = true;
    };

    explicit ScriptWatchdog(Config config);
    ~ScriptWatchdog() = default;

    /**
     * @brief 每tick检查一次
     * @param manager 脚本管理器
     */
    void tick(ScriptManager& manager);

    /**
     * @brief 检查内存限制
     * @return 是否超过内存限制
     */
    [[nodiscard]] bool checkMemoryLimit(ScriptManager& manager) const;

    /**
     * @brief 检查执行时间
     * @return 是否超过执行时间限制
     */
    [[nodiscard]] bool checkExecutionTime() const noexcept;

    /**
     * @brief 记录tick开始时间
     */
    void beginTick() noexcept;

    /**
     * @brief 记录tick结束时间
     */
    void endTick() noexcept;

    /**
     * @brief 报告统计信息
     */
    void reportStats() const;

    /**
     * @brief 获取配置
     */
    [[nodiscard]] const Config& config() const;

    /**
     * @brief 更新配置
     */
    void setConfig(const Config& config);

    /**
     * @brief 获取上次tick耗时（毫秒）
     */
    [[nodiscard]] u64 lastTickDurationMs() const;

private:
    Config m_config;
    std::chrono::steady_clock::time_point m_tickStartTime;
    u64 m_lastTickDurationMs = 0;
    u64 m_totalTickCount = 0;
    u64 m_timeoutCount = 0;
    u64 m_oomCount = 0;
};

} // namespace mc::mod::bedrock::addon
