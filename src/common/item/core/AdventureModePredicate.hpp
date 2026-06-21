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

#include <string>
#include <vector>

namespace mc {

// Forward declarations
class BlockState;
class IWorld;

/**
 * @brief 冒险模式谓词
 *
 * 对应 MC Java 的 AdventureModePredicate，用于冒险模式下物品的
 * CanPlaceOn / CanDestroy 标签匹配检查。
 *
 * 每个谓词条目可以是：
 * - 精确方块ID，如 "minecraft:stone"
 * - 方块标签引用，如 "#minecraft:logs"（以 # 开头）
 *
 * 匹配规则：
 * - 空谓词列表表示"无限制"（不匹配任何方块）
 * - 任一条目匹配即视为通过（OR 逻辑）
 * - 标签引用通过 BlockTags 系统解析
 *
 * TODO: 当前实现仅支持字符串列表匹配（方块ID和#标签引用），不支持
 * MC Java 原版 BlockPredicate 的方块状态属性匹配（如 "minecraft:oak_log[axis=y]"）
 * 和方块实体NBT匹配（如 "minecraft:chest{Items:[...]}"）。这些高级匹配功能
 * 需要在 StatePropertiesPredicate 和 NbtPredicate 实现后添加。
 * 参考: net/minecraft/world/item/AdventureModePredicate.java
 *       net/minecraft/advancements/criterion/BlockPredicate.java
 */
class AdventureModePredicate {
public:
    /**
     * @brief 默认构造（空谓词，不匹配任何方块）
     */
    AdventureModePredicate() = default;

    /**
     * @brief 从方块ID/标签列表构造
     * @param predicates 方块ID或标签引用列表（如 "minecraft:stone", "#minecraft:logs"）
     */
    explicit AdventureModePredicate(std::vector<std::string> predicates);

    /**
     * @brief 是否为空谓词（无任何条目）
     */
    [[nodiscard]] bool isEmpty() const { return m_predicates.empty(); }

    /**
     * @brief 获取谓词条目列表
     */
    [[nodiscard]] const std::vector<std::string>& getPredicates() const { return m_predicates; }

    /**
     * @brief 检查给定方块状态是否匹配此谓词
     *
     * 只要任一谓词条目匹配目标方块，即返回 true。
     * 精确方块ID匹配时，比较方块资源位置。
     * 标签引用（#前缀）通过 BlockTags 系统解析。
     *
     * @param state 目标方块状态
     * @return 如果匹配返回 true，不匹配或谓词为空返回 false
     */
    [[nodiscard]] bool test(const BlockState& state) const;

    /**
     * @brief 检查给定方块状态是否匹配此谓词（带世界参数）
     *
     * 与 test(const BlockState&) 相同逻辑，但预留了未来
     * 基于世界上下文检查的可能性。
     *
     * @param world 世界引用
     * @param state 目标方块状态
     * @return 如果匹配返回 true
     */
    [[nodiscard]] bool test(IWorld& world, const BlockState& state) const;

    /**
     * @brief 检查两个谓词是否相等
     */
    bool operator==(const AdventureModePredicate& other) const;
    bool operator!=(const AdventureModePredicate& other) const { return !(*this == other); }

private:
    /**
     * @brief 检查单个谓词条目是否匹配方块状态
     * @param predicate 单个谓词条目（方块ID或#标签引用）
     * @param state 目标方块状态
     * @return 是否匹配
     */
    [[nodiscard]] bool matchesPredicate(const std::string& predicate, const BlockState& state) const;

    /// 谓词条目列表（方块ID或#标签引用）
    std::vector<std::string> m_predicates;
};

} // namespace mc
