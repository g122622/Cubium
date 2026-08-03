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

#include "common/mod/bedrock/addon/diagnostics/ScriptSentryLogger.hpp"
#include "common/core/Types.hpp"

#include <string>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

void ScriptSentryLogger::logScriptLoaded(const std::string& pluginName, const std::string& entryPoint)
{
    spdlog::info("[BedrockSentry] script.loaded plugin={} entry={}", pluginName, entryPoint);
}

void ScriptSentryLogger::logScriptError(
    const std::string& pluginName, const std::string& error, const std::string& filename, i32 line)
{
    if (filename.empty()) {
        spdlog::error("[BedrockSentry] script.error plugin={} error=\"{}\"", pluginName, error);
    } else if (line < 0) {
        spdlog::error("[BedrockSentry] script.error plugin={} file={} error=\"{}\"", pluginName, filename, error);
    } else {
        spdlog::error(
            "[BedrockSentry] script.error plugin={} file={}:{} error=\"{}\"", pluginName, filename, line, error);
    }
}

void ScriptSentryLogger::logScriptWarning(const std::string& pluginName, const std::string& warning)
{
    spdlog::warn("[BedrockSentry] script.warning plugin={} warning=\"{}\"", pluginName, warning);
}

void ScriptSentryLogger::logModuleRegistered(const std::string& moduleName, const std::string& version)
{
    spdlog::info("[BedrockSentry] module.registered name={} version={}", moduleName, version);
}

void ScriptSentryLogger::logWatchdogEvent(const std::string& eventType, const std::string& detail)
{
    spdlog::warn("[BedrockSentry] watchdog.{} detail=\"{}\"", eventType, detail);
}

} // namespace mc::mod::bedrock::addon
