#include "DyeableArmorItem.hpp"

namespace mc {
namespace item::items {

DyeableArmorItem::DyeableArmorItem(
    const armor::ArmorMaterial& material, armor::ArmorSlot slot, ItemProperties properties)
    : ArmorItem(material, slot, std::move(properties))
{}

u32 DyeableArmorItem::getColor(const ItemStack& stack) const
{
    const auto* displayTag = stack.getChildTag(TAG_DISPLAY);
    if (displayTag != nullptr) {
        const auto colorIter = displayTag->find(TAG_COLOR);
        if (colorIter != displayTag->end() && colorIter->is_number()) {
            return colorIter->get<u32>() & 0x00FFFFFFu;
        }
    }

    return DEFAULT_COLOR;
}

void DyeableArmorItem::setColor(ItemStack& stack, u32 color)
{
    stack.getOrCreateChildTag(TAG_DISPLAY)[TAG_COLOR] = static_cast<u32>(color & 0x00FFFFFFu);
}

void DyeableArmorItem::clearColor(ItemStack& stack)
{
    auto* tag = stack.getTag();
    if (tag == nullptr || !tag->is_object()) {
        return;
    }

    auto displayIter = tag->find(TAG_DISPLAY);
    if (displayIter == tag->end() || !displayIter->is_object()) {
        return;
    }

    displayIter->erase(TAG_COLOR);
    if (displayIter->empty()) {
        stack.removeChildTag(TAG_DISPLAY);
    }
}

bool DyeableArmorItem::hasColor(const ItemStack& stack)
{
    const auto* displayTag = stack.getChildTag(TAG_DISPLAY);
    return displayTag != nullptr && displayTag->contains(TAG_COLOR) && (*displayTag)[TAG_COLOR].is_number();
}

} // namespace item::items
} // namespace mc
