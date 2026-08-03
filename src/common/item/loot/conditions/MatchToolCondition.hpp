/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
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

#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <optional>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 工具匹配条件（minecraft:match_tool）
 *
 * 对应 Minecraft 原版的 MatchTool 战利品条件。
 * 使用 ItemPredicate 对工具物品进行完整匹配，支持：
 * - 物品ID匹配
 * - 物品标签匹配
 * - 数量范围匹配
 * - 耐久范围匹配
 * - 附魔匹配
 * - 存储附魔匹配（附魔书）
 * - NBT数据匹配
 * - 药水类型匹配
 *
 * 当 predicate 为空（nullopt）时，仅检查上下文中是否存在工具（非空 ItemStack）。
 * 对应 MC Java: MatchTool.test() 中 predicate.isEmpty() 时只检查 TOOL 参数是否存在。
 *
 * 参考: net.minecraft.world.level.storage.loot.predicates.MatchTool
 */
class MatchToolCondition : public LootCondition {
public:
    /**
     * @brief 构造匹配任意工具的条件（无谓词）
     *
     * 仅检查上下文中是否存在非空工具。
     */
    MatchToolCondition();

    /**
     * @brief 构造带物品谓词的工具匹配条件
     * @param predicate 物品谓词
     */
    explicit MatchToolCondition(advancement::ItemPredicate predicate);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override;
    [[nodiscard]] std::string getType() const noexcept override { return "match_tool"; }

    /**
     * @brief 获取物品谓词
     * @return 物品谓词的可选引用，nullopt 表示匹配任意工具
     */
    [[nodiscard]] const std::optional<advancement::ItemPredicate>& getPredicate() const noexcept { return m_predicate; }

private:
    std::optional<advancement::ItemPredicate> m_predicate;
};

} // namespace loot
} // namespace mc
