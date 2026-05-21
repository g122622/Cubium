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

#include "LootFunction.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace loot {

/**
 * @brief 探险地图函数
 *
 * 生成探险地图。
 * 参考: net.minecraft.loot.functions.ExplorationMap
 *
 * 用于生成藏宝图、林地府邸地图等。
 */
class ExplorationMapFunction : public LootFunction {
public:
    /**
     * @brief 地图目的地类型
     */
    enum class Destination : u8 {
        BuriedTreasure, // 埋藏的宝藏
        Mansion,        // 林地府邸
        Monument,       // 海底神殿
        Shipwreck,      // 沉船
        RuinedPortal    // 废弃传送门
    };

    /**
     * @brief 构造探险地图函数
     * @param destination 目的地类型
     */
    explicit ExplorationMapFunction(Destination destination = Destination::BuriedTreasure);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] std::string getType() const override { return "exploration_map"; }

    [[nodiscard]] Destination getDestination() const { return m_destination; }

private:
    Destination m_destination;
};

} // namespace loot
} // namespace mc
