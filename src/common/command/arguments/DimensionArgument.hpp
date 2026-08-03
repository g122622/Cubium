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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND OF EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/command/CommandContext.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/command/suggestions/Suggestions.hpp"
#include "common/core/Types.hpp"
#include "common/world/dimension/MapDimensionId.hpp"

#include <future>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::command {

/**
 * @brief 维度参数类型
 *
 * 解析 Minecraft 维度标识符，支持以下格式：
 * - 命名空间格式："minecraft:overworld"、"minecraft:the_nether"、"minecraft:the_end"
 * - 简短格式："overworld"、"the_nether"、"the_end"
 * - 数字格式："0"、"-1"、"1"
 *
 * 解析结果为 DimensionId（i32），0=主世界, -1=下界, 1=末地。
 * 支持自动补全建议，列出所有可用维度名称。
 *
 * 对齐 MC Java 版 net.minecraft.commands.arguments.DimensionArgument
 */
class DimensionArgumentType : public ArgumentType<DimensionId> {
public:
    [[nodiscard]] DimensionId parse(StringReader& reader) override
    {
        i32 start = reader.getCursor();

        // 读取维度名称字符串
        std::string name = reader.readUnquotedString();

        // 尝试解析为维度 ID
        DimensionId dimId = dimensionNameToId(name);

        // dimensionNameToId 对未知名称默认返回 0（主世界），
        // 这里需要验证输入确实是有效的维度名称
        if (!_isValidDimensionName(name, dimId)) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::Unknown, "Unknown dimension: '" + name + "'", start);
        }

        return dimId;
    }

    [[nodiscard]] std::string getTypeName() const override { return "dimension"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"overworld", "the_nether", "the_end"};
    }

    [[nodiscard]] nlohmann::json serializeMetadata() const override { return nlohmann::json::object(); }

    // ========== 静态工厂方法 ==========

    /**
     * @brief 创建维度参数类型实例
     */
    static std::shared_ptr<DimensionArgumentType> dimension() { return std::make_shared<DimensionArgumentType>(); }

    // ========== 静态获取方法 ==========

    /**
     * @brief 从命令上下文中获取维度 ID
     * @tparam S 命令源类型
     * @param context 命令上下文
     * @param name 参数名
     * @return 维度 ID
     */
    template <typename S>
    static DimensionId getDimension(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<DimensionId>(name);
    }

private:
    /**
     * @brief 验证维度名称是否有效
     *
     * 检查解析后的维度 ID 是否对应有效的维度名称输入。
     * dimensionNameToId 对未知输入默认返回 0，需要区分"overworld"和无效输入。
     *
     * @param name 原始输入字符串
     * @param dimId 解析后的维度 ID
     * @return 是否为有效的维度名称
     */
    static bool _isValidDimensionName(const std::string& name, DimensionId dimId)
    {
        // 检查所有已知的维度名称格式
        if (name == "minecraft:overworld" || name == "overworld" || name == "0") {
            return dimId == 0;
        }
        if (name == "minecraft:the_nether" || name == "the_nether" || name == "-1") {
            return dimId == -1;
        }
        if (name == "minecraft:the_end" || name == "the_end" || name == "1") {
            return dimId == 1;
        }
        return false;
    }
};

/**
 * @brief 维度自动补全建议提供者
 *
 * 为维度参数提供 Tab 补全建议，列出所有可用维度名称。
 */
template <typename S>
class DimensionSuggestionProvider : public ISuggestionProvider<S> {
public:
    std::future<Suggestions> getSuggestions(CommandContext<S>& /*context*/, SuggestionsBuilder& builder) override
    {
        builder.suggest("overworld");
        builder.suggest("the_nether");
        builder.suggest("the_end");
        return builder.buildFuture();
    }
};

} // namespace mc::command
