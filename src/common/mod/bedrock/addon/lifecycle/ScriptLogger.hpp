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

#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/ScriptException.hpp"

#include <string>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本日志桥接
 *
 * 将JS脚本的console.log/warn/error输出桥接到spdlog。
 * 实现IScriptPrinter接口，由ScriptPluginManager使用。
 */
class ScriptLogger : public IScriptPrinter {
public:
    ScriptLogger() noexcept = default;
    ~ScriptLogger() noexcept override = default;

    // 禁止拷贝，允许移动
    ScriptLogger(const ScriptLogger&) = delete;
    ScriptLogger& operator=(const ScriptLogger&) = delete;
    ScriptLogger(ScriptLogger&&) noexcept = default;
    ScriptLogger& operator=(ScriptLogger&&) noexcept = default;

    void onInfo(const std::string& message) override;
    void onWarn(const std::string& message) override;
    void onError(const std::string& message) override;
    void onException(const ScriptException& exception) override;

    /**
     * @brief 设置日志级别过滤
     * @param level spdlog日志级别（0=trace, 1=debug, 2=info, 3=warn, 4=error, 5=critical, 6=off）
     */
    void setLogLevel(int level);

    /**
     * @brief 获取当前日志级别
     */
    [[nodiscard]] int logLevel() const;

private:
    int m_logLevel = 2; // spdlog::level::info
};

} // namespace mc::mod::bedrock::addon
