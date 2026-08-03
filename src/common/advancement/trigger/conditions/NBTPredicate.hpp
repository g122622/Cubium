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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF ANY KIND, WHETHER
 * EXPRESS OR IMPLIED, INCLUDING STATUTORY OR OTHERWISE, IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/core/Result.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

// 前向声明
namespace mc {
class Entity;
class ItemStack;
} // namespace mc

namespace mc::advancement {

/**
 * @brief NBT谓词
 *
 * 用于匹配NBT数据的条件谓词，检查实体或物品的NBT标签。
 * 匹配规则：期望标签中的所有字段必须在实际标签中存在且值相等（子集匹配），
 * 实际标签可以包含期望标签中没有的额外字段。
 */
class NBTPredicate {
public:
    /**
     * @brief 默认构造（匹配任意NBT）
     */
    NBTPredicate() = default;

    /**
     * @brief 从compound_tag构造
     * @param tag 期望的NBT数据
     */
    explicit NBTPredicate(std::unique_ptr<nbt::tags::compound_tag> tag);

    /**
     * @brief 拷贝构造
     */
    NBTPredicate(const NBTPredicate& other);

    /**
     * @brief 移动构造
     */
    NBTPredicate(NBTPredicate&& other) noexcept = default;

    /**
     * @brief 拷贝赋值
     */
    NBTPredicate& operator=(const NBTPredicate& other);

    /**
     * @brief 移动赋值
     */
    NBTPredicate& operator=(NBTPredicate&& other) noexcept = default;

    /**
     * @brief 检查实体NBT是否匹配
     *
     * 将实体序列化为NBT后进行子集匹配。
     * 对于玩家实体，还会包含 SelectedItem 字段。
     *
     * @param entity 实体
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const Entity& entity) const;

    /**
     * @brief 检查物品NBT是否匹配
     *
     * 将物品序列化为NBT后检查其tag字段进行子集匹配。
     *
     * @param stack 物品堆
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const ItemStack& stack) const;

    /**
     * @brief 检查NBT标签是否匹配
     * @param tag NBT标签（可以为nullptr）
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const nbt::tags::compound_tag* tag) const;

    /**
     * @brief 检查是否匹配任意NBT
     */
    [[nodiscard]] bool isAny() const noexcept { return m_tag == nullptr; }

    /**
     * @brief 从JSON解析
     *
     * 支持两种格式：
     * 1. 字符串格式：Mojangson 格式的NBT字符串，如 "{CustomName:'Hello'}"
     * 2. 对象格式：JSON对象直接作为NBT compound tag
     *
     * @param json JSON值（字符串或对象）
     * @return 解析结果
     */
    static Result<NBTPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     *
     * 将NBT数据序列化为Mojangson字符串格式。
     *
     * @return JSON值（字符串或null）
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief 获取期望的NBT标签
     */
    [[nodiscard]] const nbt::tags::compound_tag* getTag() const noexcept { return m_tag.get(); }

    /**
     * @brief 递归比较NBT标签是否匹配（子集匹配）
     *
     * 如果期望标签中的字段在实际标签中存在且值相等，则匹配。
     * 实际标签可以包含期望标签中没有的额外字段。
     * 对于列表类型，使用无序子集匹配：期望列表中的每个元素
     * 必须在实际列表中存在一个匹配的元素。
     *
     * 此方法与 MC Java 的 NbtUtils.compareNbt(listCompare=true) 行为一致。
     *
     * @param expected 期望的NBT数据
     * @param actual 实际的NBT数据
     * @return 是否匹配
     */
    static bool matchNBT(const nbt::tags::compound_tag& expected, const nbt::tags::compound_tag& actual) noexcept;

    /**
     * @brief 比较两个NBT标签是否相等
     *
     * 对于列表标签，使用无序子集匹配。
     *
     * @param expected 期望的NBT标签
     * @param actual 实际的NBT标签
     * @return 是否匹配
     */
    static bool matchTag(const nbt::tags::tag& expected, const nbt::tags::tag& actual) noexcept;

private:
    /**
     * @brief 将物品堆序列化为NBT并获取tag字段
     *
     * @param stack 物品堆
     * @return tag字段的compound_tag，如果无tag则返回nullptr
     */
    static std::unique_ptr<nbt::tags::compound_tag> _serializeItemStackTag(const ItemStack& stack);

    std::unique_ptr<nbt::tags::compound_tag> m_tag;
};

} // namespace mc::advancement
