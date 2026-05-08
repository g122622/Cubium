#pragma once

#include "../../Types.hpp"

namespace mc::client::ui::kagero::tpl::core {

// 引入基础类型
using mc::i32;
using mc::u8;
using mc::u64;
using mc::f32;
using mc::Size;

/**
 * @brief 模板系统配置
 *
 * 控制模板解析和编译的行为
 */
struct TemplateConfig {
    /// 是否启用严格模式（禁止所有动态特性）
    bool strictMode = true;

    /// 是否允许内联脚本（严格模式下强制禁用）
    bool allowInlineScript = false;

    /// 是否允许内联表达式（严格模式下强制禁用）
    bool allowInlineExpression = false;

    /// 是否允许动态标签名（严格模式下强制禁用）
    bool allowDynamicTagName = false;

    /// 是否启用模板缓存
    bool enableCache = true;

    /// 模板缓存最大条目数
    size_t maxCacheSize = 100;

    /// 是否在编译时验证绑定路径
    bool validateBindingPaths = true;

    /// 是否在编译时验证回调名称
    bool validateCallbackNames = true;

    /// 是否启用调试输出
    bool debugOutput = false;

    /**
     * @brief 获取默认配置
     */
    static TemplateConfig defaults() {
        return TemplateConfig{};
    }

    /**
     * @brief 获取开发模式配置（启用调试）
     */
    static TemplateConfig development() {
        TemplateConfig config;
        config.debugOutput = true;
        return config;
    }

    /**
     * @brief 获取生产模式配置（启用缓存，禁用调试）
     */
    static TemplateConfig production() {
        TemplateConfig config;
        config.enableCache = true;
        config.debugOutput = false;
        return config;
    }
};

/**
 * @brief 模板版本枚举
 */
enum class TemplateVersion : u8 {
    V1_0 = 1,   ///< 初始版本
    LATEST = V1_0
};

/**
 * @brief 模板源码位置信息
 */
struct SourceLocation {
    size_t line = 1;        ///< 行号（从1开始）
    size_t column = 1;      ///< 列号（从1开始）
    size_t offset = 0;      ///< 文件偏移量

    SourceLocation() = default;
    SourceLocation(size_t line_, size_t column_, size_t offset_ = 0)
        : line(line_), column(column_), offset(offset_) {}

    /**
     * @brief 创建无效位置
     */
    static SourceLocation invalid() {
        return SourceLocation(0, 0, 0);
    }

    /**
     * @brief 检查是否有效
     */
    [[nodiscard]] bool isValid() const {
        return line > 0 && column > 0;
    }

    /**
     * @brief 转换为字符串
     */
    [[nodiscard]] std::string toString() const {
        return "line " + std::to_string(line) + ", column " + std::to_string(column);
    }
};

/**
 * @brief 模板源码范围
 */
struct SourceRange {
    SourceLocation start;
    SourceLocation end;

    SourceRange() = default;
    SourceRange(SourceLocation start_, SourceLocation end_)
        : start(start_), end(end_) {}

    /**
     * @brief 从单个位置创建范围
     */
    static SourceRange at(const SourceLocation& loc) {
        return SourceRange(loc, loc);
    }

    /**
     * @brief 合并两个范围
     */
    [[nodiscard]] SourceRange merge(const SourceRange& other) const {
        SourceRange result;
        result.start = start.offset < other.start.offset ? start : other.start;
        result.end = end.offset > other.end.offset ? end : other.end;
        return result;
    }
};

} // namespace mc::client::ui::kagero::tpl::core
