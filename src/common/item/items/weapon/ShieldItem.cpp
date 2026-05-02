#include "ShieldItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../core/ActionResult.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../world/IWorld.hpp"

namespace mc {
namespace item {

// ========== 构造函数 ==========

ShieldItem::ShieldItem(const ItemProperties& properties)
    : Item(properties)
{
}

// ========== Item 接口重写 ==========

i32 ShieldItem::getUseDuration(const ItemStack& /*stack*/) const {
    return MAX_USE_DURATION;
}

UseAction ShieldItem::getUseAction(const ItemStack& /*stack*/) const {
    return UseAction::Block;
}

ItemActionResult ShieldItem::onItemRightClick(IWorld& /*world*/, Player& player, Hand hand) {
    ItemStack shieldStack = player.getHeldItem(hand);

    // 进入格挡状态
    player.setActiveHand(hand);
    return ItemActionResult::success(shieldStack);
}

// ========== 静态方法 ==========

bool ShieldItem::isShield(const ItemStack& stack) {
    if (stack.isEmpty()) {
        return false;
    }
    return dynamic_cast<const ShieldItem*>(stack.getItem()) != nullptr;
}

} // namespace item
} // namespace mc
