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

#include "EquipmentSlotNames.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include <optional>
#include <string_view>

namespace mc {
namespace entity::serialization::EquipmentSlotNames {

const char* toName(EquipmentSlot slot) noexcept
{
    switch (slot) {
        case EquipmentSlot::MainHand:
            return "mainhand";
        case EquipmentSlot::OffHand:
            return "offhand";
        case EquipmentSlot::Feet:
            return "feet";
        case EquipmentSlot::Legs:
            return "legs";
        case EquipmentSlot::Chest:
            return "chest";
        case EquipmentSlot::Head:
            return "head";
        case EquipmentSlot::Body:
            return "body";
        case EquipmentSlot::Saddle:
            return "saddle";
        default:
            return "mainhand";
    }
}

std::optional<EquipmentSlot> fromName(const std::string_view name) noexcept
{
    if (name == "mainhand") return EquipmentSlot::MainHand;
    if (name == "offhand") return EquipmentSlot::OffHand;
    if (name == "feet") return EquipmentSlot::Feet;
    if (name == "legs") return EquipmentSlot::Legs;
    if (name == "chest") return EquipmentSlot::Chest;
    if (name == "head") return EquipmentSlot::Head;
    if (name == "body") return EquipmentSlot::Body;
    if (name == "saddle") return EquipmentSlot::Saddle;
    return std::nullopt;
}

} // namespace entity::serialization::EquipmentSlotNames
} // namespace mc
