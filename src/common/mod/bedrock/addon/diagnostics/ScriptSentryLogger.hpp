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
    ~ScriptSentryLogger() = default;

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
