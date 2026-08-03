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

#include "common/mod/bedrock/addon/lifecycle/ScriptLogger.hpp"
#include "common/mod/bedrock/addon/core/ScriptException.hpp"

#include <string>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

void ScriptLogger::onInfo(const std::string& message)
{
    // 仅当日志级别 <= info 时输出
    if (m_logLevel <= static_cast<int>(spdlog::level::info)) {
        spdlog::info("[Script] {}", message);
    }
}

void ScriptLogger::onWarn(const std::string& message)
{
    // 仅当日志级别 <= warn 时输出
    if (m_logLevel <= static_cast<int>(spdlog::level::warn)) {
        spdlog::warn("[Script] {}", message);
    }
}

void ScriptLogger::onError(const std::string& message)
{
    // 仅当日志级别 <= error 时输出
    if (m_logLevel <= static_cast<int>(spdlog::level::err)) {
        spdlog::error("[Script] {}", message);
    }
}

void ScriptLogger::onException(const ScriptException& exception)
{
    // 异常日志始终使用error级别
    if (m_logLevel <= static_cast<int>(spdlog::level::err)) {
        spdlog::error("[Script] {} at {}:{}: {}",
            ScriptException::errorTypeName(exception.type()),
            exception.filename(),
            exception.line(),
            exception.message());
    }
}

void ScriptLogger::setLogLevel(int level)
{
    m_logLevel = level;
}

int ScriptLogger::logLevel() const
{
    return m_logLevel;
}

} // namespace mc::mod::bedrock::addon
