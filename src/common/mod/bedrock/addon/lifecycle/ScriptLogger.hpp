#pragma once

#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"

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
    ScriptLogger() = default;
    ~ScriptLogger() override = default;

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
