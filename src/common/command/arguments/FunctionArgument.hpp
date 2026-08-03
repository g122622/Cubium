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

#include "ArgumentType.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/suggestions/Suggestions.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace command {

/**
 * @brief 函数参数解析结果
 *
 * 存储 FunctionArgumentType 的解析结果，支持两种引用方式：
 * - 直接函数引用：解析 "namespace:path" 格式的函数 ID
 * - 标签引用：解析 "#namespace:path" 格式的函数标签 ID
 *
 * 延迟到实际执行时才从 FunctionManager 查找函数对象，
 * 因为解析阶段 CommandContext 中没有服务器实例。
 * 这与 MC Java 的 FunctionArgument.Result 设计一致。
 */
class FunctionArgumentResult {
public:
    /** @brief 默认构造函数（CommandNode 解析错误回退所需） */
    FunctionArgumentResult()
        : m_id(ResourceLocation("minecraft", "empty"))
        , m_isTag(false)
    {}

    /**
     * @brief 构造函数引用结果
     * @param id 函数的资源位置 ID
     */
    explicit FunctionArgumentResult(ResourceLocation id)
        : m_id(std::move(id))
        , m_isTag(false)
    {}

    /**
     * @brief 构造标签引用或函数引用结果
     * @param id 资源位置 ID
     * @param isTag true 表示标签引用，false 表示函数引用
     */
    FunctionArgumentResult(ResourceLocation id, bool isTag)
        : m_id(std::move(id))
        , m_isTag(isTag)
    {}

    /** @brief 获取资源位置 ID */
    [[nodiscard]] const ResourceLocation& id() const noexcept { return m_id; }

    /** @brief 是否为标签引用 */
    [[nodiscard]] bool isTag() const noexcept { return m_isTag; }

    /**
     * @brief 获取格式化的显示名称
     *
     * 标签引用返回 "#namespace:path"，函数引用返回 "namespace:path"
     */
    [[nodiscard]] std::string displayName() const { return m_isTag ? ("#" + m_id.toString()) : m_id.toString(); }

private:
    ResourceLocation m_id;
    bool m_isTag;
};

/**
 * @brief 函数参数类型
 *
 * 解析 /function 和 /schedule 命令中的函数名参数。
 * 支持两种格式：
 * - "namespace:path" — 直接函数引用
 * - "#namespace:path" — 函数标签引用（执行标签中所有函数）
 *
 * 参考 MC Java 的 FunctionArgument 实现。
 *
 * 用法示例：
 * @code
 * auto funcArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, FunctionArgumentResult>>(
 *     "name", FunctionArgumentType::functions());
 * funcArg->setCustomSuggestions(std::make_shared<FunctionSuggestionProvider>());
 * funcArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return runFunction(ctx); });
 * @endcode
 */
class FunctionArgumentType : public ArgumentType<FunctionArgumentResult> {
public:
    [[nodiscard]] FunctionArgumentResult parse(StringReader& reader) override;

    [[nodiscard]] std::string getTypeName() const override { return "function"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override { return {"foo", "foo:bar", "#foo"}; }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<FunctionArgumentType> functions() { return std::make_shared<FunctionArgumentType>(); }

    // ========== 静态获取方法 ==========

    /**
     * @brief 从命令上下文中获取解析后的函数参数结果
     * @tparam S 命令源类型
     * @param context 命令上下文
     * @param name 参数名
     * @return 函数参数解析结果
     */
    template <typename S>
    static FunctionArgumentResult getFunctionResult(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<FunctionArgumentResult>(name);
    }
};

} // namespace command
} // namespace mc
