/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include <optional>
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
 * 每个谓词条目可以是以下格式之一：
 * - 精确方块ID，如 "minecraft:stone"
 * - 方块标签引用，如 "#minecraft:logs"（以 # 开头）
 * - 带属性匹配的方块ID，如 "minecraft:oak_log[axis=y]"
 * - 带属性匹配的标签引用，如 "#minecraft:logs[axis=y]"
 * - 多属性匹配，如 "minecraft:oak_stairs[half=top,facing=east]"
 *
 * 匹配规则：
 * - 空谓词列表表示"无限制"（不匹配任何方块）
 * - 任一条目匹配即视为通过（OR 逻辑）
 * - 同一条目内的属性条件必须全部满足（AND 逻辑）
 * - 属性匹配使用精确值比较，如 axis=y 仅匹配 axis 属性值为 y 的方块状态
 *
 * 参考: net/minecraft/world/item/AdventureModePredicate.java
 */
class AdventureModePredicate {
public:
    /**
     * @brief 解析后的单个属性匹配条件
     */
    struct PropertyMatch {
        std::string name;  ///< 属性名，如 "axis"、"facing"
        std::string value; ///< 属性值，如 "y"、"east"
    };

    /**
     * @brief 默认构造（空谓词，不匹配任何方块）
     */
    AdventureModePredicate() = default;

    /**
     * @brief 从方块ID/标签/属性列表构造
     *
     * 接受原始字符串列表，每个字符串可以是：
     * - "minecraft:stone" —— 精确方块ID
     * - "#minecraft:logs" —— 标签引用
     * - "minecraft:oak_log[axis=y]" —— 带属性匹配的方块ID
     * - "#minecraft:logs[axis=y]" —— 带属性匹配的标签引用
     *
     * @param predicates 方块ID、标签引用或带属性匹配的字符串列表
     */
    explicit AdventureModePredicate(std::vector<std::string> predicates);

    /**
     * @brief 是否为空谓词（无任何条目）
     */
    [[nodiscard]] bool isEmpty() const { return m_predicates.empty(); }

    /**
     * @brief 获取原始谓词条目列表
     */
    [[nodiscard]] const std::vector<std::string>& getPredicates() const { return m_predicates; }

    /**
     * @brief 检查给定方块状态是否匹配此谓词（纯方块状态，无世界上下文）
     *
     * 只要任一谓词条目匹配目标方块，即返回 true。
     * 精确方块ID匹配时，比较方块资源位置。
     * 标签引用（#前缀）通过 BlockTags 系统解析。
     * 带属性匹配（[...]）时，同时检查方块状态属性值。
     *
     * @param state 目标方块状态
     * @return 如果匹配返回 true，不匹配或谓词为空返回 false
     */
    [[nodiscard]] bool test(const BlockState& state) const;

    /**
     * @brief 检查给定方块状态是否匹配此谓词（带世界参数）
     *
     * 当前与无世界参数版本行为一致。未来可扩展用于方块实体 NBT 匹配。
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
     * @param predicate 单个谓词条目（方块ID、#标签引用、或带[...]属性匹配）
     * @param state 目标方块状态
     * @return 是否匹配
     */
    [[nodiscard]] bool matchesPredicate(const std::string& predicate, const BlockState& state) const;

    /**
     * @brief 检查方块状态的属性是否匹配给定的属性条件列表
     *
     * 所有属性条件必须全部满足（AND 逻辑）。
     * 如果方块没有某个指定属性，该属性匹配失败。
     *
     * @param state 目标方块状态
     * @param properties 属性匹配条件列表
     * @return 所有属性是否都匹配
     */
    [[nodiscard]] bool matchesProperties(const BlockState& state, const std::vector<PropertyMatch>& properties) const;

    /**
     * @brief 解析谓词字符串中的属性部分
     *
     * 解析格式: "key1=value1,key2=value2"
     *
     * @param propsStr 属性字符串（不含方括号）
     * @return 解析后的属性匹配列表，解析失败返回空 optional
     */
    [[nodiscard]] static std::optional<std::vector<PropertyMatch>> parseProperties(std::string_view propsStr);

    /// 谓词条目列表（原始字符串，含可能的属性语法）
    std::vector<std::string> m_predicates;
};

} // namespace mc
