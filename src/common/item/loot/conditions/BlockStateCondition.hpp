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

#include "common/item/loot/StatePropertiesPredicate.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 方块状态条件
 *
 * 检查被破坏的方块是否为指定类型，并可选地检查方块属性值。
 *
 * 支持：
 * - 精确匹配：检查属性值是否等于指定值
 * - 范围匹配：检查属性值是否在指定范围内（适用于整数属性）
 *
 * 参考: net.minecraft.loot.conditions.BlockStateProperty
 *
 * JSON 格式示例:
 * @code
 * // 仅检查方块类型
 * {
 *   "condition": "minecraft:block_state_property",
 *   "block": "minecraft:beetroots"
 * }
 *
 * // 检查方块类型和精确属性
 * {
 *   "condition": "minecraft:block_state_property",
 *   "block": "minecraft:beetroots",
 *   "properties": { "age": "3" }
 * }
 *
 * // 检查方块类型和范围属性
 * {
 *   "condition": "minecraft:block_state_property",
 *   "block": "minecraft:wheat",
 *   "properties": { "age": { "min": "5", "max": "7" } }
 * }
 *
 * // 多属性检查
 * {
 *   "condition": "minecraft:block_state_property",
 *   "block": "minecraft:oak_door",
 *   "properties": { "open": "true", "facing": "north" }
 * }
 * @endcode
 */
class BlockStateCondition : public LootCondition {
public:
    /**
     * @brief 构造方块状态条件（仅检查方块ID）
     * @param blockId 方块ID（如 "minecraft:diamond_ore"）
     */
    explicit BlockStateCondition(const std::string& blockId);

    /**
     * @brief 构造方块状态条件（检查方块ID和属性）
     * @param blockId 方块ID（如 "minecraft:beetroots"）
     * @param properties 属性匹配谓词
     */
    BlockStateCondition(const std::string& blockId, StatePropertiesPredicate properties);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "block_state_property"; }

    [[nodiscard]] const std::string& getBlockId() const { return m_blockId; }
    [[nodiscard]] const StatePropertiesPredicate& getProperties() const { return m_properties; }

private:
    std::string m_blockId;
    StatePropertiesPredicate m_properties;
};

} // namespace loot
} // namespace mc
