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
#include "common/item/component/Fireworks.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/ListOperation.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 设置烟花火箭函数（minecraft:set_fireworks）
 *
 * 对应原版 SetFireworksFunction：修改 fireworks 组件的飞行时长与爆炸列表。
 * explosions 为列表操作对象 { "mode": ..., "explosions": [...] }，flight_duration 为可选整数。
 * 未设置的字段保留物品当前值；两者都未设置时写入空 fireworks（等同于 vanilla DEFAULT_VALUE）。
 *
 * JSON：{ "flight_duration": 2, "explosions": { "mode": "append", "explosions": [...] } }
 */
class SetFireworksFunction : public LootFunction {
public:
    struct ExplosionListOperation {
        ListOperationKind kind = ListOperationKind::ReplaceAll;
        std::vector<item::component::FireworkExplosion> explosions;
        i32 offset = 0; ///< insert 模式的插入位置
    };

    SetFireworksFunction(std::optional<ExplosionListOperation> explosions, std::optional<i32> flightDuration);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;

    [[nodiscard]] std::string getType() const noexcept override
    {
        return "set_fireworks";
    }

    [[nodiscard]] const std::optional<ExplosionListOperation>& getExplosions() const
    {
        return m_explosions;
    }

    [[nodiscard]] const std::optional<i32>& getFlightDuration() const
    {
        return m_flightDuration;
    }

private:
    std::optional<ExplosionListOperation> m_explosions;
    std::optional<i32> m_flightDuration;
};

} // namespace loot
} // namespace mc
