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
#include "common/item/component/FireworkExplosion.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 设置烟花爆炸函数（minecraft:set_firework_explosion）
 *
 * 对应原版 SetFireworkExplosionFunction：修改 firework_explosion 组件的形状/颜色/拖尾/闪烁。
 * 未给出的字段保留物品当前值（无组件时取默认值）。
 *
 * JSON：{ "shape": "star", "colors": [...], "fade_colors": [...], "trail": true, "twinkle": false }
 */
class SetFireworkExplosionFunction : public LootFunction {
public:
    SetFireworkExplosionFunction(std::optional<item::component::FireworkShape> shape,
        std::optional<std::vector<i32>> colors,
        std::optional<std::vector<i32>> fadeColors,
        std::optional<bool> trail,
        std::optional<bool> twinkle);

    [[nodiscard]] ItemStack run(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;

    [[nodiscard]] std::string getType() const noexcept override
    {
        return "set_firework_explosion";
    }

    [[nodiscard]] const std::optional<item::component::FireworkShape>& getShape() const
    {
        return m_shape;
    }

private:
    std::optional<item::component::FireworkShape> m_shape;
    std::optional<std::vector<i32>> m_colors;
    std::optional<std::vector<i32>> m_fadeColors;
    std::optional<bool> m_trail;
    std::optional<bool> m_twinkle;
};

} // namespace loot
} // namespace mc
