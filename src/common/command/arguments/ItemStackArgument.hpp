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
 */

#pragma once

#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/ItemArgument.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <memory>
#include <string>

namespace mc::command {

/**
 * @brief 物品栈参数（原版 {@code ItemArgument}，命令树里叫 {@code item_stack}）
 *
 * 语法是「物品 id + 可选的 `[组件]` + 可选的 `{...}` 数据」。类型名与网络 id 按原版
 * `item_stack` 上报；具体物品/组件由 {@link ItemArgumentType} 的解析与调用方处理。
 */
class ItemStackArgumentType : public ArgumentType<ItemInput> {
public:
    [[nodiscard]] ItemInput parse(StringReader& reader) override
    {
        // TODO: 物品栈的组件语法（`stone[damage=5]`）与 NBT 数据尚未完整实现，
        //   这里先按 ItemArgument 的语法读物品 id，并吞掉可选的 `[...]` / `{...}`。
        ItemInput result = ItemArgumentType::item()->parse(reader);
        skipBracketGroup(reader, '[', ']');
        skipBracketGroup(reader, '{', '}');
        return result;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "item_stack";
    }

    static std::shared_ptr<ItemStackArgumentType> itemStack()
    {
        return std::make_shared<ItemStackArgumentType>();
    }

private:
    static void skipBracketGroup(StringReader& reader, char open, char close)
    {
        if (!reader.canRead() || reader.peek() != open) {
            return;
        }
        i32 depth = 0;
        while (reader.canRead()) {
            const char c = reader.read();
            if (c == open) {
                ++depth;
            } else if (c == close) {
                --depth;
                if (depth == 0) {
                    return;
                }
            }
        }
    }
};

} // namespace mc::command
