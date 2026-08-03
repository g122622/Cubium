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

#include "FunctionArgument.hpp"

#include "common/command/StringReader.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstddef>
#include <string>
#include <utility>

namespace mc {
namespace command {

/**
 * @brief 判断字符是否为资源位置标识符中的合法字符
 *
 * MC Java 的 Identifier.read() 使用贪婪读取，允许的字符为：
 * a-z, 0-9, _, -, ., :, /
 * 这比 readUnquotedString()（允许 +）更严格，但不允许空白和引号。
 */
static bool isAllowedInIdentifier(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == ':' || c == '/';
}

/**
 * @brief 从 StringReader 中贪婪读取资源位置标识符
 *
 * 与 readString() 不同，此方法严格只读取合法的标识符字符，
 * 在遇到非法字符时停止读取（不报错），与 MC Java 的 Identifier.read() 行为一致。
 */
static std::string readIdentifierGreedy(StringReader& reader)
{
    i32 start = reader.getCursor();
    while (reader.canRead() && isAllowedInIdentifier(reader.peek())) {
        reader.skip();
    }
    const auto startIdx = static_cast<size_t>(start);
    const auto endIdx = static_cast<size_t>(reader.getCursor());
    return std::string(reader.getString().substr(startIdx, endIdx - startIdx));
}

FunctionArgumentResult FunctionArgumentType::parse(StringReader& reader)
{
    i32 start = reader.getCursor();

    // 检查是否为标签引用（# 前缀）
    if (reader.canRead() && reader.peek() == '#') {
        reader.skip(); // 跳过 '#'
        std::string str = readIdentifierGreedy(reader);
        if (str.empty()) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::StringExpected, "Expected function tag name after '#'", start);
        }
        ResourceLocation id = ResourceLocation::parse(str);
        return FunctionArgumentResult(std::move(id), true);
    }

    // 普通函数引用
    std::string str = readIdentifierGreedy(reader);
    if (str.empty()) {
        reader.setCursor(start);
        throw CommandException(CommandErrorType::StringExpected, "Expected function name", start);
    }

    ResourceLocation id = ResourceLocation::parse(str);
    return FunctionArgumentResult(std::move(id), false);
}

} // namespace command
} // namespace mc
