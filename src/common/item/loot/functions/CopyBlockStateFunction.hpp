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
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {

class BlockState;

namespace loot {

/**
 * @brief 复制方块状态函数
 *
 * 从被破坏的方块复制状态属性到物品。
 * 用于保留方块的某些状态属性（如容器的朝向）。
 */
class CopyBlockStateFunction : public LootFunction {
public:
    /**
     * @brief 构造复制方块状态函数
     * @param blockId 方块ID
     * @param properties 要复制的属性名列表（空表示复制所有）
     */
    explicit CopyBlockStateFunction(const std::string& blockId, const std::vector<std::string>& properties = {});

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "copy_block_state"; }

    [[nodiscard]] const std::string& getBlockId() const { return m_blockId; }
    [[nodiscard]] const std::vector<std::string>& getProperties() const { return m_properties; }

private:
    std::string m_blockId;
    std::vector<std::string> m_properties;
};

} // namespace loot
} // namespace mc
