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

#include "LootFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 随机附魔函数
 *
 * 随机选择附魔类型和等级。
 * 参考: net.minecraft.loot.functions.EnchantRandomly
 */
class EnchantRandomlyFunction : public LootFunction {
public:
    /**
     * @brief 构造随机附魔函数
     * @param enchantments 可选的附魔ID列表（空表示随机选择所有适用附魔）
     * @param treasure 是否包含宝藏附魔
     */
    explicit EnchantRandomlyFunction(const std::vector<std::string>& enchantments = {}, bool treasure = false);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "enchant_randomly"; }

    [[nodiscard]] const std::vector<std::string>& getEnchantments() const { return m_enchantments; }
    [[nodiscard]] bool isTreasure() const { return m_treasure; }

private:
    std::vector<std::string> m_enchantments;
    bool m_treasure;
};

} // namespace loot
} // namespace mc
