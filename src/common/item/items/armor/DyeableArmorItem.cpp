#include "DyeableArmorItem.hpp"

namespace mc {
namespace item::items {

DyeableArmorItem::DyeableArmorItem(const armor::ArmorMaterial& material, armor::ArmorSlot slot,
                                    ItemProperties properties)
    : ArmorItem(material, slot, std::move(properties)) {
}

u32 DyeableArmorItem::getColor(const ItemStack& stack) const {
    // TODO: 从NBT标签读取颜色
    // 当前实现返回默认颜色
    if (hasColor(stack)) {
        // 从NBT读取: stack.getTag()->getCompound("display")->getInt("color")
        return DEFAULT_COLOR;
    }
    return DEFAULT_COLOR;
}

void DyeableArmorItem::setColor(ItemStack& stack, u32 color) {
    // TODO: 设置NBT标签
    // stack.getOrCreateTag()->getCompound("display")->putInt("color", color);
    (void)stack;
    (void)color;
}

void DyeableArmorItem::clearColor(ItemStack& stack) {
    // TODO: 从NBT标签移除颜色
    (void)stack;
}

bool DyeableArmorItem::hasColor(const ItemStack& stack) {
    // TODO: 检查NBT标签
    (void)stack;
    return false;
}

} // namespace item::items
} // namespace mc
