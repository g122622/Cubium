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

namespace mc {

// Forward declarations
class Entity;
class Player;
class BlockEntity;

namespace loot {

/**
 * @brief 复制名称函数
 *
 * 从掉落源复制名称到物品。
 * 参考: net.minecraft.loot.functions.CopyName
 *
 * 用于命名实体掉落物品时保留名称（如命名生物掉落的物品）。
 */
class CopyNameFunction : public LootFunction {
public:
    /**
     * @brief 名称来源
     */
    enum class Source : u8 {
        This,         // 当前实体
        Killer,       // 击杀者
        KillerPlayer, // 击杀玩家
        BlockEntity   // 方块实体
    };

    /**
     * @brief 构造复制名称函数
     * @param source 名称来源
     */
    explicit CopyNameFunction(Source source);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "copy_name"; }

    [[nodiscard]] Source getSource() const { return m_source; }

private:
    Source m_source;
};

} // namespace loot
} // namespace mc
