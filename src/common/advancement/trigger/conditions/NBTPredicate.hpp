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

#include "common/core/Result.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>

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
     * @param entity 实体
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const Entity& entity) const;

    /**
     * @brief 检查物品NBT是否匹配
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
     */
    static Result<NBTPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief 获取期望的NBT标签
     */
    [[nodiscard]] const nbt::tags::compound_tag* getTag() const noexcept { return m_tag.get(); }

private:
    /**
     * @brief 递归比较NBT标签是否匹配
     *
     * 如果期望标签中的字段在实际标签中存在且值相等，则匹配。
     * 实际标签可以包含期望标签中没有的额外字段。
     *
     * @param expected 期望的NBT数据
     * @param actual 实际的NBT数据
     * @return 是否匹配
     */
    static bool _matchNBT(const nbt::tags::compound_tag& expected, const nbt::tags::compound_tag& actual) noexcept;

    /**
     * @brief 比较两个NBT标签是否相等
     */
    static bool _matchTag(const nbt::tags::tag& expected, const nbt::tags::tag& actual) noexcept;

    std::unique_ptr<nbt::tags::compound_tag> m_tag;
};

} // namespace mc::advancement
