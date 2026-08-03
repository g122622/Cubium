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

#include "common/mod/bedrock/addon/plugin/ScriptPlugin.hpp"
#include "common/core/Result.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/ModuleDescriptor.hpp"
#include "common/mod/bedrock/addon/plugin/PluginExecutionGroup.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPackConfiguration.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPluginSource.hpp"

#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

ScriptPlugin::ScriptPlugin(std::string uuid, std::string name, std::string version, PluginExecutionGroup executionGroup)
    : m_uuid(std::move(uuid))
    , m_name(std::move(name))
    , m_version(std::move(version))
    , m_executionGroup(executionGroup)
{}

ScriptPlugin::~ScriptPlugin()
{
    unload();
}

ScriptPlugin::ScriptPlugin(ScriptPlugin&& other) noexcept
    : m_uuid(std::move(other.m_uuid))
    , m_name(std::move(other.m_name))
    , m_version(std::move(other.m_version))
    , m_executionGroup(other.m_executionGroup)
    , m_state(other.m_state)
    , m_context(std::move(other.m_context))
    , m_configuration(std::move(other.m_configuration))
    , m_errorMessage(std::move(other.m_errorMessage))
{
    other.m_state = State::Unloaded;
}

ScriptPlugin& ScriptPlugin::operator=(ScriptPlugin&& other) noexcept
{
    if (this != &other) {
        unload();
        m_uuid = std::move(other.m_uuid);
        m_name = std::move(other.m_name);
        m_version = std::move(other.m_version);
        m_executionGroup = other.m_executionGroup;
        m_state = other.m_state;
        m_context = std::move(other.m_context);
        m_configuration = std::move(other.m_configuration);
        m_errorMessage = std::move(other.m_errorMessage);
        other.m_state = State::Unloaded;
    }
    return *this;
}

Result<void> ScriptPlugin::load(IScriptEngine& engine, IDependencyLoader& source, IScriptPrinter& printer)
{
    if (m_state != State::Unloaded) {
        return Error(ErrorCode::InvalidState, "Cannot load plugin '" + m_name + "' in state: " + stateName(m_state));
    }

    m_state = State::Loading;
    spdlog::info("[BedrockAddon] Loading plugin: {} ({})", m_name, m_uuid);

    // 创建模块描述符
    ModuleDescriptor descriptor;
    descriptor.name = m_name;
    descriptor.uuid = m_uuid;
    descriptor.isRuntimeModule = false;

    // 创建脚本上下文
    auto context = engine.createContext(descriptor, {}, source, printer);
    if (!context) {
        m_state = State::Error;
        m_errorMessage = "Failed to create script context for plugin: " + m_name;
        spdlog::error("[BedrockAddon] {}", m_errorMessage);
        return Error(ErrorCode::OperationFailed, m_errorMessage);
    }

    m_context = std::move(context);

    // 加载入口脚本
    auto* pluginSource = dynamic_cast<ScriptPluginSource*>(&source);
    if (pluginSource) {
        auto entryData = pluginSource->loadEntryPoint();
        if (entryData.has_value()) {
            auto evalResult = m_context->evaluateModule(entryData->source, entryData->filePath);
            if (!evalResult.success()) {
                m_state = State::Error;
                m_errorMessage = "Failed to execute entry point: " + evalResult.errorMessage();
                spdlog::error("[BedrockAddon] {}", m_errorMessage);
                m_context.reset();
                return Error(ErrorCode::OperationFailed, m_errorMessage);
            }
            spdlog::info("[BedrockAddon] Entry point executed: {}", entryData->filePath);
        } else {
            spdlog::warn("[BedrockAddon] No entry point found for plugin: {}", m_name);
        }
    }

    m_state = State::Loaded;
    spdlog::info("[BedrockAddon] Plugin loaded: {} ({})", m_name, m_uuid);
    return Result<void>::ok();
}

Result<void> ScriptPlugin::start()
{
    if (m_state != State::Loaded) {
        return Error(ErrorCode::InvalidState, "Cannot start plugin '" + m_name + "' in state: " + stateName(m_state));
    }

    m_state = State::Running;
    spdlog::info("[BedrockAddon] Plugin started: {} ({})", m_name, m_uuid);
    return Result<void>::ok();
}

void ScriptPlugin::stop()
{
    if (m_state != State::Running) {
        return;
    }

    m_state = State::Loaded;
    spdlog::info("[BedrockAddon] Plugin stopped: {} ({})", m_name, m_uuid);
}

void ScriptPlugin::unload()
{
    if (m_state == State::Unloaded) {
        return;
    }

    m_state = State::Unloading;

    if (m_context) {
        m_context.reset();
    }

    m_errorMessage.clear();
    m_state = State::Unloaded;
    spdlog::info("[BedrockAddon] Plugin unloaded: {} ({})", m_name, m_uuid);
}

void ScriptPlugin::tick()
{
    if (m_state != State::Running || !m_context) {
        return;
    }

    // 处理JS引擎的待处理任务（Promise回调、setTimeout等）
    if (!m_context->isValid()) {
        m_state = State::Error;
        m_errorMessage = "Script context became invalid during tick";
        spdlog::error("[BedrockAddon] {}", m_errorMessage);
        return;
    }
}

const std::string& ScriptPlugin::uuid() const
{
    return m_uuid;
}

const std::string& ScriptPlugin::name() const
{
    return m_name;
}

const std::string& ScriptPlugin::version() const
{
    return m_version;
}

ScriptPlugin::State ScriptPlugin::state() const
{
    return m_state;
}

PluginExecutionGroup ScriptPlugin::executionGroup() const
{
    return m_executionGroup;
}

const std::string& ScriptPlugin::errorMessage() const
{
    return m_errorMessage;
}

IScriptContext* ScriptPlugin::context()
{
    return m_context.get();
}

const IScriptContext* ScriptPlugin::context() const
{
    return m_context.get();
}

ScriptPackConfiguration& ScriptPlugin::configuration()
{
    return m_configuration;
}

const ScriptPackConfiguration& ScriptPlugin::configuration() const
{
    return m_configuration;
}

const char* ScriptPlugin::stateName(State state)
{
    switch (state) {
        case State::Unloaded:
            return "Unloaded";
        case State::Loading:
            return "Loading";
        case State::Loaded:
            return "Loaded";
        case State::Running:
            return "Running";
        case State::Error:
            return "Error";
        case State::Unloading:
            return "Unloading";
    }
    return "Unknown";
}

} // namespace mc::mod::bedrock::addon
