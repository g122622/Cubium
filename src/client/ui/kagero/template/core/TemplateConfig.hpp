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

#include "client/ui/kagero/Types.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc::client::ui::kagero::tpl::core {

// 引入基础类型
using mc::f32;
using mc::i32;
using mc::Size;
using mc::u64;
using mc::u8;

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
    Size maxCacheSize = 100;

    /// 是否在编译时验证绑定路径
    bool validateBindingPaths = true;

    /// 是否在编译时验证回调名称
    bool validateCallbackNames = true;

    /// 是否启用调试输出
    bool debugOutput = false;

    /**
     * @brief 获取默认配置
     */
    static TemplateConfig defaults() { return TemplateConfig{}; }

    /**
     * @brief 获取开发模式配置（启用调试）
     */
    static TemplateConfig development()
    {
        TemplateConfig config;
        config.debugOutput = true;
        return config;
    }

    /**
     * @brief 获取生产模式配置（启用缓存，禁用调试）
     */
    static TemplateConfig production()
    {
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
    V1_0 = 1, ///< 初始版本
    LATEST = V1_0
};

/**
 * @brief 模板源码位置信息
 */
struct SourceLocation {
    Size line = 1;   ///< 行号（从1开始）
    Size column = 1; ///< 列号（从1开始）
    Size offset = 0; ///< 文件偏移量

    SourceLocation() = default;
    SourceLocation(Size line_, Size column_, Size offset_ = 0)
        : line(line_)
        , column(column_)
        , offset(offset_)
    {}

    /**
     * @brief 创建无效位置
     */
    static SourceLocation invalid() { return SourceLocation(0, 0, 0); }

    /**
     * @brief 检查是否有效
     */
    [[nodiscard]] bool isValid() const { return line > 0 && column > 0; }

    /**
     * @brief 转换为字符串
     */
    [[nodiscard]] std::string toString() const
    {
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
        : start(start_)
        , end(end_)
    {}

    /**
     * @brief 从单个位置创建范围
     */
    static SourceRange at(const SourceLocation& loc) { return SourceRange(loc, loc); }

    /**
     * @brief 合并两个范围
     */
    [[nodiscard]] SourceRange merge(const SourceRange& other) const
    {
        SourceRange result;
        result.start = start.offset < other.start.offset ? start : other.start;
        result.end = end.offset > other.end.offset ? end : other.end;
        return result;
    }
};

} // namespace mc::client::ui::kagero::tpl::core
