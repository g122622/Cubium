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

#include "common/mod/bedrock/addon/modules/types/ScriptWorldAccessor.hpp"
#include "common/core/Types.hpp"

#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

ScriptWorldAccessor& ScriptWorldAccessor::instance()
{
    static ScriptWorldAccessor instance;
    return instance;
}

void ScriptWorldAccessor::sendMessage(const std::string& message)
{
    if (m_messageCallback) {
        m_messageCallback(message);
    } else {
        spdlog::warn("[BedrockAddon] ScriptWorldAccessor::sendMessage: no callback registered");
    }
}

std::vector<std::string> ScriptWorldAccessor::getAllPlayerNames()
{
    if (m_getPlayerNamesCallback) {
        return m_getPlayerNamesCallback();
    }
    return {};
}

u64 ScriptWorldAccessor::currentTick() const
{
    if (m_currentTickCallback) {
        return m_currentTickCallback();
    }
    return 0;
}

mc::IWorld* ScriptWorldAccessor::getDimension(const std::string& dimensionId)
{
    if (m_getDimensionCallback) {
        return m_getDimensionCallback(dimensionId);
    }
    return nullptr;
}

mc::scoreboard::Scoreboard* ScriptWorldAccessor::getScoreboard()
{
    if (m_getScoreboardCallback) {
        return m_getScoreboardCallback();
    }
    return nullptr;
}

void ScriptWorldAccessor::setMessageCallback(std::function<void(const std::string&)> callback)
{
    m_messageCallback = std::move(callback);
}

void ScriptWorldAccessor::setGetPlayerNamesCallback(std::function<std::vector<std::string>()> callback)
{
    m_getPlayerNamesCallback = std::move(callback);
}

void ScriptWorldAccessor::setCurrentTickCallback(std::function<u64()> callback)
{
    m_currentTickCallback = std::move(callback);
}

void ScriptWorldAccessor::setGetDimensionCallback(std::function<mc::IWorld*(const std::string&)> callback)
{
    m_getDimensionCallback = std::move(callback);
}

void ScriptWorldAccessor::setGetScoreboardCallback(std::function<mc::scoreboard::Scoreboard*()> callback)
{
    m_getScoreboardCallback = std::move(callback);
}

} // namespace mc::mod::bedrock::addon
