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
 * LIABILITY, CONTRACT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "NautilusArmorItem.hpp"
#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/core/Item.hpp"

namespace mc {
namespace item::items {

NautilusArmorItem::NautilusArmorItem(const ItemProperties& properties, const armor::ArmorMaterial& material)
    : Item(properties)
    // 护甲值由材质的 Body 槽位防御值提供（与 MC 1.21.11 Item.Properties.nautilusArmor
    // 通过 ArmorMaterial.createAttributes(ArmorType.BODY) 取护甲值的语义一致）
    , m_armorValue(material.getDefense(armor::ArmorSlot::Body))
    , m_equipSound(material.getEquipSound())
    , m_material(material)
{}

} // namespace item::items
} // namespace mc
