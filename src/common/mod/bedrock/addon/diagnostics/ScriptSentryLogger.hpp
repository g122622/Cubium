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

#include <string>

namespace mc::mod::bedrock::addon {

/**
 * @brief Sentry风格脚本日志器
 *
 * 以结构化格式记录脚本执行事件，便于调试和监控。
 * 输出通过spdlog进行。
 */
class ScriptSentryLogger {
public:
    ScriptSentryLogger() = default;
    ~ScriptSentryLogger() noexcept = default;

    /**
     * @brief 记录脚本加载事件
     */
    void logScriptLoaded(const std::string& pluginName, const std::string& entryPoint);

    /**
     * @brief 记录脚本错误事件
     */
    void logScriptError(
        const std::string& pluginName, const std::string& error, const std::string& filename = "", i32 line = -1);

    /**
     * @brief 记录脚本警告事件
     */
    void logScriptWarning(const std::string& pluginName, const std::string& warning);

    /**
     * @brief 记录模块注册事件
     */
    void logModuleRegistered(const std::string& moduleName, const std::string& version);

    /**
     * @brief 记录看门狗事件
     */
    void logWatchdogEvent(const std::string& eventType, const std::string& detail);
};

} // namespace mc::mod::bedrock::addon
