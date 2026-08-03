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

#include "CopyNameFunction.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 填充玩家头颅函数
 *
 * 用玩家信息填充玩家头颅物品。
 * 参考: net.minecraft.loot.functions.FillPlayerHead
 *
 * 用于生成玩家头颅时填充皮肤信息。
 */
class FillPlayerHeadFunction : public LootFunction {
public:
    /**
     * @brief 构造填充玩家头颅函数
     * @param source 玩家来源（This/Killer/KillerPlayer）
     */
    explicit FillPlayerHeadFunction(CopyNameFunction::Source source);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "fill_player_head"; }

    [[nodiscard]] CopyNameFunction::Source getSource() const { return m_source; }

private:
    CopyNameFunction::Source m_source;
};

} // namespace loot
} // namespace mc
