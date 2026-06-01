#pragma once

#include "common/core/Types.hpp"

#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本异常类型
 */
enum class ScriptErrorType : u8 {
    SyntaxError,       // 语法错误
    TypeError,         // 类型错误
    RangeError,        // 范围错误
    ReferenceError,    // 引用错误
    RuntimeError,      // 运行时错误
    ModuleError,       // 模块加载错误
    InternalError,     // 内部错误
    TimeoutError,      // 超时错误
};

/**
 * @brief 脚本异常
 *
 * 从脚本引擎抛出的异常信息
 */
class ScriptException {
public:
    ScriptException(ScriptErrorType type, std::string message, std::string filename = "", i32 line = -1,
                    i32 column = -1)
        : m_type(type), m_message(std::move(message)), m_filename(std::move(filename)), m_line(line),
          m_column(column) {}

    [[nodiscard]] ScriptErrorType type() const { return m_type; }
    [[nodiscard]] const std::string& message() const { return m_message; }
    [[nodiscard]] const std::string& filename() const { return m_filename; }
    [[nodiscard]] i32 line() const { return m_line; }
    [[nodiscard]] i32 column() const { return m_column; }

    [[nodiscard]] std::string toString() const {
        std::string result = errorTypeName(m_type);
        if (!m_message.empty()) {
            result += ": " + m_message;
        }
        if (!m_filename.empty()) {
            result += " at " + m_filename;
            if (m_line >= 0) {
                result += ":" + std::to_string(m_line);
                if (m_column >= 0) {
                    result += ":" + std::to_string(m_column);
                }
            }
        }
        return result;
    }

    static const char* errorTypeName(ScriptErrorType type) {
        switch (type) {
        case ScriptErrorType::SyntaxError: return "SyntaxError";
        case ScriptErrorType::TypeError: return "TypeError";
        case ScriptErrorType::RangeError: return "RangeError";
        case ScriptErrorType::ReferenceError: return "ReferenceError";
        case ScriptErrorType::RuntimeError: return "RuntimeError";
        case ScriptErrorType::ModuleError: return "ModuleError";
        case ScriptErrorType::InternalError: return "InternalError";
        case ScriptErrorType::TimeoutError: return "TimeoutError";
        }
        return "UnknownError";
    }

private:
    ScriptErrorType m_type;
    std::string m_message;
    std::string m_filename;
    i32 m_line;
    i32 m_column;
};

} // namespace mc::mod::bedrock::addon
