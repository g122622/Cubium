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

#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace command {

/**
 * @brief NBT 路径节点接口
 *
 * 表示 NBT 路径中的单个节点，如键名、数组索引或过滤器。
 * 参考 MC 1.16.5: net.minecraft.command.arguments.NBTPathArgument.INode
 */
class NbtPathNode {
public:
    virtual ~NbtPathNode() = default;

    /**
     * @brief 克隆节点
     * @return 节点的深拷贝
     */
    [[nodiscard]] virtual std::unique_ptr<NbtPathNode> clone() const = 0;

    /**
     * @brief 从 NBT 标签中获取匹配的所有值
     * @param tag 输入的 NBT 标签
     * @return 匹配的标签列表
     * @throws CommandException 如果路径无效
     */
    [[nodiscard]] virtual std::vector<nbt::tags::tag*> get(nbt::tags::tag* tag) const = 0;

    /**
     * @brief 从 NBT 标签中获取匹配的所有值（const 版本）
     */
    [[nodiscard]] virtual std::vector<const nbt::tags::tag*> get(const nbt::tags::tag* tag) const = 0;

    /**
     * @brief 设置路径指向的值
     * @param tag 目标 NBT 标签
     * @param valueSupplier 值提供函数
     * @return 修改的数量
     */
    virtual i32 set(nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const = 0;

    /**
     * @brief 删除路径指向的值
     * @param tag 目标 NBT 标签
     * @return 删除的数量
     */
    virtual i32 remove(nbt::tags::tag* tag) const = 0;

    /**
     * @brief 获取或创建目标标签
     * @param tag 输入标签
     * @param creator 创建函数
     * @return 目标标签集合
     */
    [[nodiscard]] virtual std::vector<nbt::tags::tag*> getOrCreate(
        nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator) const = 0;

    /**
     * @brief 获取节点描述字符串
     */
    [[nodiscard]] virtual std::string toString() const = 0;
};

/**
 * @brief NBT 路径
 *
 * 表示完整的 NBT 路径，由多个节点组成。
 * 支持路径解析、获取、设置、删除操作。
 *
 * 参考 MC 1.16.5: net.minecraft.command.arguments.NBTPathArgument.NBTPath
 *
 * @example
 * @code
 * // 路径示例:
 * // "foo" - 访问复合标签的键 "foo"
 * // "foo.bar" - 访问嵌套键
 * // "foo[0]" - 访问列表的第一个元素
 * // "foo[]" - 访问列表的所有元素
 * // "foo{bar:1}" - 过滤复合标签
 * // "foo[{id:"diamond"}]" - 过滤列表中的元素
 * @endcode
 */
class NbtPath {
public:
    /**
     * @brief 构造空路径
     */
    NbtPath() = default;

    /**
     * @brief 从原始字符串和节点列表构造路径
     * @param rawText 原始路径字符串
     * @param nodes 路径节点
     */
    NbtPath(std::string rawText, std::vector<std::unique_ptr<NbtPathNode>> nodes);

    /**
     * @brief 拷贝构造
     */
    NbtPath(const NbtPath& other);

    /**
     * @brief 移动构造
     */
    NbtPath(NbtPath&& other) noexcept = default;

    /**
     * @brief 拷贝赋值
     */
    NbtPath& operator=(const NbtPath& other);

    /**
     * @brief 移动赋值
     */
    NbtPath& operator=(NbtPath&& other) noexcept = default;

    /**
     * @brief 从复合标签获取路径指向的所有值
     * @param tag 复合标签
     * @return 匹配的标签列表
     * @throws CommandException 如果路径无效或未找到
     */
    [[nodiscard]] std::vector<const nbt::tags::tag*> get(const nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从复合标签获取单个值
     * @param tag 复合标签
     * @return 匹配的单个标签，如果有多个匹配则抛出异常
     * @throws CommandException 如果路径无效、未找到或找到多个
     */
    [[nodiscard]] const nbt::tags::tag* getSingle(const nbt::tags::compound_tag& tag) const;

    /**
     * @brief 计算路径匹配的数量
     * @param tag 复合标签
     * @return 匹配的数量
     */
    [[nodiscard]] i32 count(const nbt::tags::compound_tag& tag) const;

    /**
     * @brief 检查路径是否存在
     * @param tag 复合标签
     * @return 如果路径存在则返回 true
     */
    [[nodiscard]] bool exists(const nbt::tags::compound_tag& tag) const;

    /**
     * @brief 设置路径指向的值
     * @param tag 复合标签（会被修改）
     * @param valueSupplier 值提供函数
     * @return 修改的数量
     * @throws CommandException 如果路径无效
     */
    i32 set(nbt::tags::compound_tag& tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const;

    /**
     * @brief 删除路径指向的值
     * @param tag 复合标签（会被修改）
     * @return 删除的数量
     * @throws CommandException 如果路径无效
     */
    i32 remove(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 将值插入到列表的指定位置
     * @param tag 复合标签（会被修改）
     * @param index 插入位置（负数从末尾计算）
     * @param values 要插入的值
     * @return 插入的数量
     */
    i32 insert(
        nbt::tags::compound_tag& tag, i32 index, const std::vector<std::unique_ptr<nbt::tags::tag>>& values) const;

    /**
     * @brief 将值追加到列表末尾
     * @param tag 复合标签（会被修改）
     * @param values 要追加的值
     * @return 追加的数量
     */
    i32 append(nbt::tags::compound_tag& tag, const std::vector<std::unique_ptr<nbt::tags::tag>>& values) const;

    /**
     * @brief 将值预置到列表开头
     * @param tag 复合标签（会被修改）
     * @param values 要预置的值
     * @return 预置的数量
     */
    i32 prepend(nbt::tags::compound_tag& tag, const std::vector<std::unique_ptr<nbt::tags::tag>>& values) const;

    /**
     * @brief 合并复合标签到路径指向的位置
     * @param tag 复合标签（会被修改）
     * @param value 要合并的复合标签
     * @return 修改的数量
     */
    i32 merge(nbt::tags::compound_tag& tag, const nbt::tags::compound_tag& value) const;

    /**
     * @brief 获取原始路径字符串
     */
    [[nodiscard]] const std::string& toString() const noexcept { return m_rawText; }

    /**
     * @brief 检查路径是否为空
     */
    [[nodiscard]] bool empty() const noexcept { return m_nodes.empty(); }

    /**
     * @brief 获取路径节点数量
     */
    [[nodiscard]] size_t size() const noexcept { return m_nodes.size(); }

private:
    std::string m_rawText;
    std::vector<std::unique_ptr<NbtPathNode>> m_nodes;
};

// ========== 路径节点实现 ==========

/**
 * @brief 字符串节点 - 访问复合标签的键
 *
 * 示例: "foo" 访问 compound.foo
 */
class NbtPathStringNode : public NbtPathNode {
public:
    explicit NbtPathStringNode(std::string name)
        : m_name(std::move(name))
    {}

    [[nodiscard]] std::unique_ptr<NbtPathNode> clone() const override
    {
        return std::make_unique<NbtPathStringNode>(m_name);
    }

    [[nodiscard]] std::vector<nbt::tags::tag*> get(nbt::tags::tag* tag) const override;
    [[nodiscard]] std::vector<const nbt::tags::tag*> get(const nbt::tags::tag* tag) const override;
    i32 set(nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const override;
    i32 remove(nbt::tags::tag* tag) const override;
    [[nodiscard]] std::vector<nbt::tags::tag*> getOrCreate(
        nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator) const override;
    [[nodiscard]] std::string toString() const override { return m_name; }

    [[nodiscard]] const std::string& name() const noexcept { return m_name; }

private:
    std::string m_name;
};

/**
 * @brief 索引节点 - 访问列表的指定索引
 *
 * 示例: "foo[0]" 访问 foo 列表的第一个元素
 */
class NbtPathIndexNode : public NbtPathNode {
public:
    explicit NbtPathIndexNode(i32 index)
        : m_index(index)
    {}

    [[nodiscard]] std::unique_ptr<NbtPathNode> clone() const override
    {
        return std::make_unique<NbtPathIndexNode>(m_index);
    }

    [[nodiscard]] std::vector<nbt::tags::tag*> get(nbt::tags::tag* tag) const override;
    [[nodiscard]] std::vector<const nbt::tags::tag*> get(const nbt::tags::tag* tag) const override;
    i32 set(nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const override;
    i32 remove(nbt::tags::tag* tag) const override;
    [[nodiscard]] std::vector<nbt::tags::tag*> getOrCreate(
        nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator) const override;
    [[nodiscard]] std::string toString() const override { return "[" + std::to_string(m_index) + "]"; }

    [[nodiscard]] i32 index() const noexcept { return m_index; }

private:
    i32 m_index;
};

/**
 * @brief 空列表节点 - 匹配列表中的所有元素
 *
 * 示例: "foo[]" 访问 foo 列表的所有元素
 */
class NbtPathAllElementsNode : public NbtPathNode {
public:
    NbtPathAllElementsNode() = default;

    [[nodiscard]] std::unique_ptr<NbtPathNode> clone() const override
    {
        return std::make_unique<NbtPathAllElementsNode>();
    }

    [[nodiscard]] std::vector<nbt::tags::tag*> get(nbt::tags::tag* tag) const override;
    [[nodiscard]] std::vector<const nbt::tags::tag*> get(const nbt::tags::tag* tag) const override;
    i32 set(nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const override;
    i32 remove(nbt::tags::tag* tag) const override;
    [[nodiscard]] std::vector<nbt::tags::tag*> getOrCreate(
        nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator) const override;
    [[nodiscard]] std::string toString() const override { return "[]"; }
};

/**
 * @brief 复合过滤器节点 - 过滤复合标签
 *
 * 示例: "{foo:bar}" 匹配包含 foo=bar 的复合标签
 */
class NbtPathCompoundFilterNode : public NbtPathNode {
public:
    explicit NbtPathCompoundFilterNode(std::unique_ptr<nbt::tags::compound_tag> filter);

    [[nodiscard]] std::unique_ptr<NbtPathNode> clone() const override;

    [[nodiscard]] std::vector<nbt::tags::tag*> get(nbt::tags::tag* tag) const override;
    [[nodiscard]] std::vector<const nbt::tags::tag*> get(const nbt::tags::tag* tag) const override;
    i32 set(nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const override;
    i32 remove(nbt::tags::tag* tag) const override;
    [[nodiscard]] std::vector<nbt::tags::tag*> getOrCreate(
        nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator) const override;
    [[nodiscard]] std::string toString() const override;

private:
    /**
     * @brief 检查复合标签是否匹配过滤器
     */
    [[nodiscard]] bool _matches(const nbt::tags::compound_tag& tag) const;

    std::unique_ptr<nbt::tags::compound_tag> m_filter;
};

/**
 * @brief 列表过滤节点 - 根据复合过滤器过滤列表元素
 *
 * 示例: "foo[{id:"diamond"}]" 匹配 foo 列表中包含 id="diamond" 的元素
 */
class NbtPathListFilterNode : public NbtPathNode {
public:
    explicit NbtPathListFilterNode(std::unique_ptr<nbt::tags::compound_tag> filter);

    [[nodiscard]] std::unique_ptr<NbtPathNode> clone() const override;

    [[nodiscard]] std::vector<nbt::tags::tag*> get(nbt::tags::tag* tag) const override;
    [[nodiscard]] std::vector<const nbt::tags::tag*> get(const nbt::tags::tag* tag) const override;
    i32 set(nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const override;
    i32 remove(nbt::tags::tag* tag) const override;
    [[nodiscard]] std::vector<nbt::tags::tag*> getOrCreate(
        nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator) const override;
    [[nodiscard]] std::string toString() const override;

private:
    /**
     * @brief 检查复合标签是否匹配过滤器
     */
    [[nodiscard]] bool _matches(const nbt::tags::compound_tag& tag) const;

    std::unique_ptr<nbt::tags::compound_tag> m_filter;
};

/**
 * @brief 复合键过滤器节点 - 键名后跟复合过滤器
 *
 * 示例: "foo{bar:1}" 访问 foo 键，并检查其是否包含 bar=1
 */
class NbtPathKeyFilterNode : public NbtPathNode {
public:
    NbtPathKeyFilterNode(std::string name, std::unique_ptr<nbt::tags::compound_tag> filter);

    [[nodiscard]] std::unique_ptr<NbtPathNode> clone() const override;

    [[nodiscard]] std::vector<nbt::tags::tag*> get(nbt::tags::tag* tag) const override;
    [[nodiscard]] std::vector<const nbt::tags::tag*> get(const nbt::tags::tag* tag) const override;
    i32 set(nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const override;
    i32 remove(nbt::tags::tag* tag) const override;
    [[nodiscard]] std::vector<nbt::tags::tag*> getOrCreate(
        nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator) const override;
    [[nodiscard]] std::string toString() const override;

private:
    std::string m_name;
    std::unique_ptr<nbt::tags::compound_tag> m_filter;
};

} // namespace command
} // namespace mc
