/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
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

#include "WolfArmorItem.hpp"
#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/armor/DyeableArmorItem.hpp"
#include <utility>

namespace mc {
namespace item::items {

WolfArmorItem::WolfArmorItem(const armor::ArmorMaterial& material, armor::ArmorSlot slot, ItemProperties properties)
    : DyeableArmorItem(material, slot, std::move(properties))
{
    // ArmorItem 基类构造函数中 _buildAttributeModifiers(getDefense()) 已正确使用
    // ArmadilloScuteArmorMaterial::getDefense(ArmorSlot::Body) 返回 11（MC 1.21.11 wolf_armor defense = 11）
    // 无需在此重建属性修饰符
}

} // namespace item::items
} // namespace mc
