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

#pragma once

#include "ArgumentType.hpp"
#include "NbtPath.hpp"
#include "common/command/StringReader.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <any>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace mc {
namespace command {

/**
 * @brief NBT 路径参数类型
 *
 * 解析 NBT 路径字符串为 NbtPath 对象。
 *
 * 支持的路径语法：
 * - "foo" - 访问复合标签的键 "foo"
 * - "foo.bar" - 访问嵌套键
 * - "foo[0]" - 访问列表的第一个元素
 * - "foo[-1]" - 访问列表的最后一个元素
 * - "foo[]" - 访问列表的所有元素
 * - "{foo:bar}" - 复合过滤器
 * - "foo[{id:'diamond'}]" - 列表过滤器
 * - "foo{bar:1}" - 键名 + 复合过滤器
 *
 * @example
 * @code
 * // 解析路径
 * StringReader reader("Items[0].tag.display.Name");
 * NbtPath path = NbtPathArgumentType::nbtPath()->parse(reader);
 *
 * // 使用路径获取值
 * auto results = path.get(compoundTag);
 * @endcode
 */
class NbtPathArgumentType : public ArgumentType<NbtPath> {
public:
    [[nodiscard]] NbtPath parse(StringReader& reader) override;
    [[nodiscard]] std::string getTypeName() const override { return "nbt_path"; }
    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"foo", "foo.bar", "foo[0]", "foo[]", "{foo:bar}"};
    }

    /**
     * @brief 创建 NBT 路径参数类型
     */
    static std::shared_ptr<NbtPathArgumentType> nbtPath() { return std::make_shared<NbtPathArgumentType>(); }

    /**
     * @brief 从命令上下文获取 NBT 路径
     */
    template <typename S>
    static NbtPath getNbtPath(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<NbtPath>(name);
    }

    /**
     * @brief 解析复合标签过滤器
     * @param reader 字符串读取器
     * @return 复合标签
     */
    [[nodiscard]] static std::unique_ptr<nbt::tags::compound_tag> parseCompoundFilter(StringReader& reader);

    /**
     * @brief 解析 NBT 值
     * @param reader 字符串读取器
     * @return NBT 标签
     */
    [[nodiscard]] static std::unique_ptr<nbt::tags::tag> parseNbtValue(StringReader& reader);

private:
    /**
     * @brief 解析单个路径节点
     * @param reader 字符串读取器
     * @param isFirst 是否是第一个节点
     * @return 路径节点
     */
    [[nodiscard]] std::unique_ptr<NbtPathNode> _parseNode(StringReader& reader, bool isFirst);

    /**
     * @brief 解析键名
     * @param reader 字符串读取器
     * @return 键名
     */
    [[nodiscard]] static std::string _parseKeyName(StringReader& reader);

    /**
     * @brief 解析 NBT 列表内容
     * @param reader 字符串读取器
     * @return 列表标签
     */
    [[nodiscard]] static std::unique_ptr<nbt::tags::tag> _parseListContent(StringReader& reader);

    /**
     * @brief 读取 NBT 未引用键名
     *
     * 遇到特殊字符时停止: :, 空白
     * @param reader 字符串读取器
     * @return 读取的字符串
     */
    [[nodiscard]] static std::string _readNbtUnquotedKey(StringReader& reader);

    /**
     * @brief 读取 NBT 未引用字符串值
     *
     * 遇到特殊字符时停止: :, ,, }, ], 空白
     * @param reader 字符串读取器
     * @return 读取的字符串
     */
    [[nodiscard]] static std::string _readNbtUnquotedValue(StringReader& reader);
};

/**
 * @brief NBT 复合标签参数类型
 *
 * 解析 Mojangson 格式的 NBT 复合标签。
 * 使用 shared_ptr 以支持存储在 std::any 中。
 */
class NbtCompoundArgumentType : public ArgumentType<std::shared_ptr<nbt::tags::compound_tag>> {
public:
    [[nodiscard]] std::shared_ptr<nbt::tags::compound_tag> parse(StringReader& reader) override;
    [[nodiscard]] std::string getTypeName() const override { return "nbt_compound"; }
    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"{}", "{foo:bar}", "{foo:1,bar:\"hello\"}"};
    }

    /**
     * @brief 创建 NBT 复合标签参数类型
     */
    static std::shared_ptr<NbtCompoundArgumentType> nbtCompound()
    {
        return std::make_shared<NbtCompoundArgumentType>();
    }

    /**
     * @brief 从命令上下文获取 NBT 复合标签
     */
    template <typename S>
    static std::shared_ptr<nbt::tags::compound_tag> getNbt(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<std::shared_ptr<nbt::tags::compound_tag>>(name);
    }
};

/**
 * @brief NBT 标签参数类型
 *
 * 解析 Mojangson 格式的任意 NBT 标签。
 * 使用 shared_ptr 以支持存储在 std::any 中。
 */
class NbtTagArgumentType : public ArgumentType<std::shared_ptr<nbt::tags::tag>> {
public:
    [[nodiscard]] std::shared_ptr<nbt::tags::tag> parse(StringReader& reader) override;
    [[nodiscard]] std::string getTypeName() const override { return "nbt_tag"; }
    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"0", "true", "\"hello\"", "{foo:bar}", "[1,2,3]"};
    }

    /**
     * @brief 创建 NBT 标签参数类型
     */
    static std::shared_ptr<NbtTagArgumentType> nbtTag() { return std::make_shared<NbtTagArgumentType>(); }

    /**
     * @brief 从命令上下文获取 NBT 标签
     */
    template <typename S>
    static std::shared_ptr<nbt::tags::tag> getNbt(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<std::shared_ptr<nbt::tags::tag>>(name);
    }
};

} // namespace command
} // namespace mc
