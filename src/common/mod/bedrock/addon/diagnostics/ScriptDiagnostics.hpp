#pragma once

#include "common/core/Types.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本诊断级别
 */
enum class DiagnosticLevel : u8 {
    Info = 0,
    Warning = 1,
    Error = 2,
};

/**
 * @brief 脚本诊断条目
 */
struct DiagnosticEntry {
    DiagnosticLevel level = DiagnosticLevel::Info;
    std::string source;   ///< 来源插件名称
    std::string message;  ///< 诊断消息
    std::string filename; ///< 相关脚本文件
    i32 line = -1;        ///< 行号
    i32 column = -1;      ///< 列号
};

/**
 * @brief 脚本诊断收集器
 *
 * 收集和报告脚本执行过程中的错误、警告和诊断信息。
 * 用于开发调试和运行时错误追踪。
 */
class ScriptDiagnostics {
public:
    ScriptDiagnostics() = default;
    ~ScriptDiagnostics() = default;

    // 拷贝操作
    ScriptDiagnostics(const ScriptDiagnostics&) = default;
    ScriptDiagnostics& operator=(const ScriptDiagnostics&) = default;

    // 移动操作
    ScriptDiagnostics(ScriptDiagnostics&&) noexcept = default;
    ScriptDiagnostics& operator=(ScriptDiagnostics&&) noexcept = default;

    /**
     * @brief 记录诊断信息
     */
    void add(DiagnosticLevel level,
        std::string source,
        std::string message,
        std::string filename = "",
        i32 line = -1,
        i32 column = -1);

    /**
     * @brief 获取所有诊断条目
     */
    [[nodiscard]] const std::vector<DiagnosticEntry>& entries() const;

    /**
     * @brief 获取指定级别的诊断条目数量
     */
    [[nodiscard]] size_t count(DiagnosticLevel level) const;

    /**
     * @brief 清除所有诊断条目
     */
    void clear();

    /**
     * @brief 生成诊断报告
     */
    [[nodiscard]] std::string generateReport() const;

private:
    std::vector<DiagnosticEntry> m_entries;
};

} // namespace mc::mod::bedrock::addon
