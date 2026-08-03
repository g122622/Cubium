/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the the following conditions:
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

#include "common/util/nbt/Nbt.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mc {

// Forward declarations
class BlockState;
class BlockPos;
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
 * - 带NBT匹配的方块ID，如 "minecraft:chest{Items:[...]}"
 * - 同时带属性和NBT匹配，如 "minecraft:chest[waterlogged=false]{Items:[...]}"
 *
 * 匹配规则：
 * - 空谓词列表表示"无限制"（不匹配任何方块）
 * - 任一条目匹配即视为通过（OR 逻辑）
 * - 同一条目内的属性条件必须全部满足（AND 逻辑）
 * - 属性匹配使用精确值比较，如 axis=y 仅匹配 axis 属性值为 y 的方块状态
 * - NBT匹配使用子集比较（期望标签中的所有字段必须在实际标签中存在且值相等）
 * - NBT匹配需要世界上下文（IWorld + BlockPos）以获取方块实体数据
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
     * @brief 从方块ID/标签/属性/NBT列表构造
     *
     * 接受原始字符串列表，每个字符串可以是：
     * - "minecraft:stone" —— 精确方块ID
     * - "#minecraft:logs" —— 标签引用
     * - "minecraft:oak_log[axis=y]" —— 带属性匹配的方块ID
     * - "#minecraft:logs[axis=y]" —— 带属性匹配的标签引用
     * - "minecraft:chest{Items:[...]}" —— 带NBT匹配的方块ID
     * - "minecraft:chest[waterlogged=false]{Items:[...]}" —— 同时带属性和NBT匹配
     *
     * @param predicates 方块ID、标签引用或带属性/NBT匹配的字符串列表
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
     * 不检查方块实体的NBT数据，NBT条件会被忽略。
     * 如果谓词包含NBT条件，建议使用带世界上下文的重载版本。
     *
     * @param state 目标方块状态
     * @return 如果匹配返回 true，不匹配或谓词为空返回 false
     */
    [[nodiscard]] bool test(const BlockState& state) const;

    /**
     * @brief 检查给定方块状态是否匹配此谓词（带世界上下文，支持NBT匹配）
     *
     * 当谓词包含NBT条件时，会从世界获取对应位置的方块实体，
     * 将方块实体的NBT数据与谓词中的NBT条件进行子集匹配。
     * 如果没有方块实体但谓词要求NBT匹配，则该条目不匹配。
     *
     * @param world 世界引用
     * @param pos 方块位置（用于获取方块实体）
     * @param state 目标方块状态
     * @return 如果匹配返回 true
     */
    [[nodiscard]] bool test(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 检查给定方块状态是否匹配此谓词（带世界上下文但不带位置，不支持NBT匹配）
     *
     * 此重载提供向后兼容性。无法获取方块实体NBT数据，
     * NBT条件会被忽略。如果需要NBT匹配，请使用带 BlockPos 的版本。
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
     * @brief 解析后的单个谓词条目
     *
     * 存储从原始谓词字符串解析出的各组成部分：
     * - blockPart: 方块ID或标签引用（不含属性和NBT）
     * - properties: 属性匹配条件列表
     * - nbtTag: 解析后的NBT标签（可能为空）
     * - hasNbt: 是否包含NBT条件
     */
    struct ParsedPredicate {
        std::string blockPart;                                ///< 方块ID或标签引用
        std::optional<std::vector<PropertyMatch>> properties; ///< 属性匹配条件
        std::shared_ptr<nbt::tags::compound_tag> nbtTag;      ///< 解析后的NBT标签（可能为空）
        bool hasNbt = false;                                  ///< 是否包含NBT条件
    };

    /**
     * @brief 检查单个解析后的谓词条目是否匹配方块状态（不检查NBT）
     * @param predicate 解析后的谓词
     * @param state 目标方块状态
     * @return 是否匹配
     */
    [[nodiscard]] bool matchesParsedPredicate(const ParsedPredicate& predicate, const BlockState& state) const;

    /**
     * @brief 检查单个解析后的谓词条目是否匹配方块状态和方块实体NBT
     * @param predicate 解析后的谓词
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 目标方块状态
     * @return 是否匹配
     */
    [[nodiscard]] bool matchesParsedPredicateWithNbt(
        const ParsedPredicate& predicate, IWorld& world, const BlockPos& pos, const BlockState& state) const;

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

    /**
     * @brief 解析谓词字符串中的NBT部分
     *
     * 从谓词字符串中提取并解析 {Items:[...]} 等 NBT 数据。
     * 支持 Mojangson 格式的 NBT 字符串。
     *
     * @param predicate 完整谓词字符串
     * @param nbtStart NBT 部分的起始位置（'{' 的位置）
     * @return 解析后的 NBT compound tag，解析失败返回 nullptr
     */
    [[nodiscard]] static std::shared_ptr<nbt::tags::compound_tag> parseNbt(
        const std::string& predicate, size_t nbtStart);

    /**
     * @brief 解析原始谓词字符串为结构化的 ParsedPredicate
     *
     * 将谓词字符串分解为方块ID/标签、属性条件和NBT标签。
     * 支持格式：
     * - "minecraft:stone"
     * - "#minecraft:logs"
     * - "minecraft:oak_log[axis=y]"
     * - "minecraft:chest{Items:[...]}"
     * - "minecraft:chest[waterlogged=false]{Items:[...]}"
     *
     * @param predicate 原始谓词字符串
     * @return 解析后的 ParsedPredicate
     */
    [[nodiscard]] static ParsedPredicate parsePredicate(const std::string& predicate);

    /// 原始谓词条目列表（含可能的属性和NBT语法）
    std::vector<std::string> m_predicates;

    /// 解析后的谓词条目列表（延迟解析，首次使用时填充）
    mutable std::vector<ParsedPredicate> m_parsedPredicates;
    mutable bool m_parsed = false;

    /**
     * @brief 确保谓词已解析
     *
     * 延迟解析谓词字符串，首次调用时填充 m_parsedPredicates。
     */
    void ensureParsed() const;
};

} // namespace mc
