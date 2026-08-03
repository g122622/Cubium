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
 * The copyright notice and this permission notice shall be included in all
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

#include "NbtPathArgumentType.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/arguments/NbtPath.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <cctype>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace command {

// ========== NbtPathArgumentType 实现 ==========

NbtPath NbtPathArgumentType::parse(StringReader& reader)
{
    i32 start = reader.getCursor();
    std::vector<std::unique_ptr<NbtPathNode>> nodes;
    bool isFirst = true;

    while (reader.canRead() && reader.peek() != ' ') {
        auto node = _parseNode(reader, isFirst);
        nodes.push_back(std::move(node));
        isFirst = false;

        // 检查是否需要 '.' 分隔
        if (reader.canRead() && reader.peek() != ' ') {
            char c = reader.peek();
            if (c != '[' && c != '{') {
                // 需要使用 '.' 分隔键名
                if (c == '.') {
                    reader.skip();
                } else {
                    throw CommandException(CommandErrorType::InvalidNbtPath,
                        "Expected '.' at position " + std::to_string(reader.getCursor()),
                        reader.getCursor());
                }
            }
        }
    }

    if (nodes.empty()) {
        throw CommandException(CommandErrorType::InvalidNbtPath, "Empty NBT path", start);
    }

    std::string rawText(reader.getString().substr(start, reader.getCursor() - start));
    return NbtPath(std::move(rawText), std::move(nodes));
}

std::unique_ptr<NbtPathNode> NbtPathArgumentType::_parseNode(StringReader& reader, bool isFirst)
{
    char c = reader.peek();

    // 处理引号包围的键名
    // 参考 MC 源码 readObjectNode：只检查后面是否跟 {，不处理 [
    // [ 留给主循环的下一轮 parseNode 调用自然处理
    if (c == '"') {
        std::string name = reader.readString();
        // 检查后面是否跟复合过滤器：foo{bar:1} 组合为 KeyFilterNode
        if (reader.canRead() && reader.peek() == '{') {
            auto filter = parseCompoundFilter(reader);
            return std::make_unique<NbtPathKeyFilterNode>(name, std::move(filter));
        }
        return std::make_unique<NbtPathStringNode>(name);
    }

    // 处理数组索引或过滤器
    if (c == '[') {
        reader.skip();
        if (reader.peek() == ']') {
            reader.skip();
            return std::make_unique<NbtPathAllElementsNode>();
        }
        if (reader.peek() == '{') {
            auto filter = parseCompoundFilter(reader);
            reader.expect(']');
            return std::make_unique<NbtPathListFilterNode>(std::move(filter));
        }
        // 解析索引
        i32 index = reader.readInt();
        reader.expect(']');
        return std::make_unique<NbtPathIndexNode>(index);
    }

    // 处理复合过滤器（仅第一个节点）
    if (c == '{') {
        if (!isFirst) {
            throw CommandException(
                CommandErrorType::InvalidNbtPath, "Compound filter only allowed at path start", reader.getCursor());
        }
        auto filter = parseCompoundFilter(reader);
        return std::make_unique<NbtPathCompoundFilterNode>(std::move(filter));
    }

    // 解析普通键名
    std::string name = _parseKeyName(reader);

    // 检查后面是否跟过滤器或索引
    if (reader.canRead()) {
        char next = reader.peek();
        if (next == '[') {
            // 键名后跟索引：foo[0]，返回 StringNode，让主循环处理 IndexNode
            return std::make_unique<NbtPathStringNode>(name);
        }
        if (next == '{') {
            auto filter = parseCompoundFilter(reader);
            return std::make_unique<NbtPathKeyFilterNode>(name, std::move(filter));
        }
    }

    return std::make_unique<NbtPathStringNode>(name);
}

std::unique_ptr<nbt::tags::compound_tag> NbtPathArgumentType::parseCompoundFilter(StringReader& reader)
{
    reader.expect('{');

    auto compound = std::make_unique<nbt::tags::compound_tag>();

    // 跳过空白
    reader.skipWhitespace();

    if (reader.peek() == '}') {
        reader.skip();
        return compound;
    }

    while (true) {
        reader.skipWhitespace();

        // 解析键名 - 使用 _readNbtUnquotedKey 停止在冒号处
        std::string key;
        if (reader.peek() == '"') {
            key = reader.readString();
        } else {
            key = _readNbtUnquotedKey(reader);
        }

        if (key.empty()) {
            throw CommandException(
                CommandErrorType::InvalidNbtPath, "Empty key in compound filter", reader.getCursor());
        }

        reader.skipWhitespace();
        reader.expect(':');
        reader.skipWhitespace();

        // 解析值
        auto value = parseNbtValue(reader);
        compound->value[key] = std::move(value);

        reader.skipWhitespace();

        if (reader.peek() == '}') {
            reader.skip();
            break;
        }

        reader.expect(',');
    }

    return compound;
}

std::string NbtPathArgumentType::_readNbtUnquotedKey(StringReader& reader)
{
    // 读取 NBT 未引用键名，遇到特殊字符时停止
    // 特殊字符包括: :, 空白
    std::string result;

    while (reader.canRead()) {
        char c = reader.peek();
        // 在 NBT 键名中，冒号表示键值分隔符
        if (c == ':' || reader.isWhitespace(c)) {
            break;
        }
        result += c;
        reader.skip();
    }

    return result;
}

std::string NbtPathArgumentType::_parseKeyName(StringReader& reader)
{
    // 键名可以包含字母、数字、下划线、点等
    // 但不能以数字开头
    i32 start = reader.getCursor();

    // 支持的字符：a-z, A-Z, 0-9, _, ., -, +
    // 但在路径中，点是分隔符，所以不应该出现在键名中
    std::string result;

    while (reader.canRead()) {
        char c = reader.peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '+') {
            result += c;
            reader.skip();
        } else {
            break;
        }
    }

    if (result.empty()) {
        throw CommandException(
            CommandErrorType::InvalidNbtPath, "Expected key name at position " + std::to_string(start), start);
    }

    return result;
}

std::unique_ptr<nbt::tags::tag> NbtPathArgumentType::parseNbtValue(StringReader& reader)
{
    char c = reader.peek();

    // 字符串（引号包围）
    if (c == '"') {
        std::string value = reader.readString();
        return std::make_unique<nbt::tags::string_tag>(value);
    }

    // 复合标签
    if (c == '{') {
        return parseCompoundFilter(reader);
    }

    // 列表
    if (c == '[') {
        return _parseListContent(reader);
    }

    // 布尔值
    if (c == 't' || c == 'T') {
        std::string word = _readNbtUnquotedValue(reader);
        if (word == "true" || word == "TRUE") {
            return std::make_unique<nbt::tags::byte_tag>(1);
        }
        // 不是布尔值，可能是以 t 开头的未引用字符串
        return std::make_unique<nbt::tags::string_tag>(word);
    }
    if (c == 'f' || c == 'F') {
        std::string word = _readNbtUnquotedValue(reader);
        if (word == "false" || word == "FALSE") {
            return std::make_unique<nbt::tags::byte_tag>(0);
        }
        // 不是布尔值，可能是以 f 开头的未引用字符串
        return std::make_unique<nbt::tags::string_tag>(word);
    }

    // 数字或未引用字符串
    // 如果以数字、- 或 + 开头，尝试解析为数字
    if (std::isdigit(static_cast<unsigned char>(c)) || c == '-' || c == '+') {
        i32 start = reader.getCursor();
        std::string numStr;

        // 读取数字字符串
        bool hasDot = false;
        if (c == '-' || c == '+') {
            numStr += c;
            reader.skip();
            c = reader.peek();
        }

        while (reader.canRead()) {
            c = reader.peek();
            if (std::isdigit(static_cast<unsigned char>(c))) {
                numStr += c;
                reader.skip();
            } else if (c == '.' && !hasDot) {
                hasDot = true;
                numStr += c;
                reader.skip();
            } else {
                break;
            }
        }

        // 检查后面是否跟类型后缀
        if (reader.canRead()) {
            c = reader.peek();
            if (c == 'b' || c == 'B' || c == 's' || c == 'S' || c == 'l' || c == 'L' || c == 'f' || c == 'F' ||
                c == 'd' || c == 'D') {
                reader.skip();
                try {
                    switch (c) {
                        case 'b':
                        case 'B':
                            return std::make_unique<nbt::tags::byte_tag>(static_cast<i8>(std::stoi(numStr)));
                        case 's':
                        case 'S':
                            return std::make_unique<nbt::tags::short_tag>(static_cast<i16>(std::stoi(numStr)));
                        case 'l':
                        case 'L':
                            return std::make_unique<nbt::tags::long_tag>(std::stoll(numStr));
                        case 'f':
                        case 'F':
                            return std::make_unique<nbt::tags::float_tag>(std::stof(numStr));
                        case 'd':
                        case 'D':
                            return std::make_unique<nbt::tags::double_tag>(std::stod(numStr));
                    }
                }
                catch (const std::exception& e) {
                    throw CommandException(CommandErrorType::InvalidNbtPath, "Invalid number: " + numStr, start);
                }
            }
        }

        // 没有后缀，根据是否有小数点决定类型
        try {
            if (hasDot) {
                return std::make_unique<nbt::tags::double_tag>(std::stod(numStr));
            } else {
                return std::make_unique<nbt::tags::int_tag>(std::stoi(numStr));
            }
        }
        catch (const std::exception& e) {
            throw CommandException(CommandErrorType::InvalidNbtPath, "Invalid number: " + numStr, start);
        }
    }

    // 未引用字符串（如 foo:bar 中的 bar）
    std::string value = _readNbtUnquotedValue(reader);
    if (!value.empty()) {
        return std::make_unique<nbt::tags::string_tag>(value);
    }

    throw CommandException(CommandErrorType::InvalidNbtPath,
        "Expected value at position " + std::to_string(reader.getCursor()),
        reader.getCursor());
}

std::string NbtPathArgumentType::_readNbtUnquotedValue(StringReader& reader)
{
    // 读取 NBT 未引用字符串值，遇到特殊字符时停止
    // 特殊字符包括: :, ,, }, ], 空白
    i32 start = reader.getCursor();
    std::string result;

    while (reader.canRead()) {
        char c = reader.peek();
        // 在 NBT 值中，以下字符表示值的结束
        if (c == ':' || c == ',' || c == '}' || c == ']' || reader.isWhitespace(c)) {
            break;
        }
        result += c;
        reader.skip();
    }

    return result;
}

std::unique_ptr<nbt::tags::tag> NbtPathArgumentType::_parseListContent(StringReader& reader)
{
    reader.expect('[');

    // 跳过空白
    reader.skipWhitespace();

    // 检查是否是类型化数组
    if (reader.canRead()) {
        char c = reader.peek();
        if (c == 'B' || c == 'I' || c == 'L') {
            char typeChar = c;
            reader.skip();
            reader.skipWhitespace();
            reader.expect(';');

            // 类型化数组
            std::vector<i8> bytes;
            std::vector<i32> ints;
            std::vector<i64> longs;

            while (reader.canRead() && reader.peek() != ']') {
                reader.skipWhitespace();
                auto value = parseNbtValue(reader);
                reader.skipWhitespace();

                if (typeChar == 'B') {
                    if (value->id() == nbt::TagId::Byte) {
                        bytes.push_back(dynamic_cast<nbt::tags::byte_tag&>(*value).value);
                    } else if (value->id() == nbt::TagId::Int) {
                        bytes.push_back(static_cast<i8>(dynamic_cast<nbt::tags::int_tag&>(*value).value));
                    }
                } else if (typeChar == 'I') {
                    if (value->id() == nbt::TagId::Int) {
                        ints.push_back(dynamic_cast<nbt::tags::int_tag&>(*value).value);
                    } else if (value->id() == nbt::TagId::Byte) {
                        ints.push_back(dynamic_cast<nbt::tags::byte_tag&>(*value).value);
                    }
                } else if (typeChar == 'L') {
                    if (value->id() == nbt::TagId::Long) {
                        longs.push_back(dynamic_cast<nbt::tags::long_tag&>(*value).value);
                    } else if (value->id() == nbt::TagId::Int) {
                        longs.push_back(dynamic_cast<nbt::tags::int_tag&>(*value).value);
                    }
                }

                reader.skipWhitespace();
                if (reader.peek() == ',') {
                    reader.skip();
                }
            }

            reader.expect(']');

            if (typeChar == 'B') {
                return std::make_unique<nbt::tags::bytearray_tag>(bytes);
            } else if (typeChar == 'I') {
                return std::make_unique<nbt::tags::intarray_tag>(ints);
            } else {
                return std::make_unique<nbt::tags::longarray_tag>(longs);
            }
        }
    }

    // 普通列表
    auto list = std::make_unique<nbt::tags::tag_list_tag>();
    std::vector<std::unique_ptr<nbt::tags::tag>> elements;

    while (reader.canRead() && reader.peek() != ']') {
        reader.skipWhitespace();
        auto value = parseNbtValue(reader);
        elements.push_back(std::move(value));
        reader.skipWhitespace();

        if (reader.peek() == ',') {
            reader.skip();
            reader.skipWhitespace();
        }
    }

    reader.expect(']');

    if (!elements.empty()) {
        nbt::TagId elemType = elements[0]->id();
        for (const auto& elem : elements) {
            if (elem->id() != elemType) {
                elemType = nbt::TagId::End; // 混合类型
                break;
            }
        }

        if (elemType != nbt::TagId::End) {
            list->eid = elemType;
        }
        list->value = std::move(elements);
    }

    return list;
}

// ========== NbtCompoundArgumentType 实现 ==========

std::shared_ptr<nbt::tags::compound_tag> NbtCompoundArgumentType::parse(StringReader& reader)
{
    if (reader.peek() != '{') {
        throw CommandException(CommandErrorType::InvalidNbtPath,
            "Expected '{' at position " + std::to_string(reader.getCursor()),
            reader.getCursor());
    }

    // 复用 NbtPathArgumentType 的解析器
    NbtPathArgumentType pathParser;
    return pathParser.parseCompoundFilter(reader);
}

// ========== NbtTagArgumentType 实现 ==========

std::shared_ptr<nbt::tags::tag> NbtTagArgumentType::parse(StringReader& reader)
{
    NbtPathArgumentType pathParser;
    return pathParser.parseNbtValue(reader);
}

} // namespace command
} // namespace mc
