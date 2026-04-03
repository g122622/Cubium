#include "GlassBottleItem.hpp"

namespace mc {
namespace item {

// ========== GlassBottleItem 实现 ==========

GlassBottleItem::GlassBottleItem(const ItemProperties& properties)
    : Item(properties) {
}

ItemActionResult GlassBottleItem::onItemRightClick(IWorld& /*world*/, Player& /*player*/, Hand /*hand*/) {
    // TODO: 射线检测水源
    // 对水源使用：装水变为水瓶
    // 对炼药锅使用：装水变为水瓶

    return ItemActionResult(ActionResultType::Pass, ItemStack());
}

} // namespace item
} // namespace mc
