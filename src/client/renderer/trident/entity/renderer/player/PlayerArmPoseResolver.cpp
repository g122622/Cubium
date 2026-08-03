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

#include "PlayerArmPoseResolver.hpp"
#include "client/renderer/trident/entity/model/player/PlayerModel.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/items/weapon/CrossbowItem.hpp"
#include "common/item/tag/ItemTags.hpp"

namespace mc::client::renderer::entity::renderer::player {

model::player::ArmPose PlayerArmPoseResolver::determineArmPose(::mc::Player& player, ::mc::Hand hand)
{
    // 获取手持物品
    const ::mc::ItemStack& heldStack = player.getHeldItem(hand);
    if (heldStack.isEmpty()) {
        return model::player::ArmPose::Empty;
    }

    const ::mc::Item* item = heldStack.getItem();
    if (item == nullptr) {
        return model::player::ArmPose::Item;
    }

    // 1) 已装填的弩：未挥动时返回 CrossbowHold
    if (!player.isSwingInProgress() && item == ::mc::Items::CROSSBOW) {
        if (::mc::item::CrossbowItem::isCharged(heldStack)) {
            return model::player::ArmPose::CrossbowHold;
        }
    }

    // 2) 正在使用物品且使用的手就是当前判断的手
    if (player.isUsingItem() && player.getActiveHand() == hand && player.getItemInUseCount() > 0) {
        const ::mc::UseAction useAction = item->getUseAction(heldStack);
        switch (useAction) {
            case ::mc::UseAction::Block:
                return model::player::ArmPose::Block;
            case ::mc::UseAction::Bow:
                return model::player::ArmPose::BowAndArrow;
            case ::mc::UseAction::Spear:
                // Trident 是 Spear 的别名（同枚举值），三叉戟与长矛共用 ThrowSpear 姿态
                return model::player::ArmPose::ThrowSpear;
            case ::mc::UseAction::Crossbow:
                return model::player::ArmPose::CrossbowCharge;
            case ::mc::UseAction::Spyglass:
                return model::player::ArmPose::Spyglass;
            case ::mc::UseAction::Brush:
                return model::player::ArmPose::Brush;
            case ::mc::UseAction::Bundle:
                // 收纳袋使用动作类似饮用，第三人称 ArmPose 无 EatOrDrink 枚举，降级为 Item
                // TODO: 若第三人称 ArmPose 扩展 EatOrDrink，应改为返回该姿态
                return model::player::ArmPose::Item;
            default:
                break;
        }
    }

    // 3) 长矛类物品（通过 ItemTags::SPEARS 标签判断）返回 ThrowSpear
    if (::mc::item::tag::ItemTags::isInitialized()) {
        if (item->isIn(::mc::item::tag::ItemTags::SPEARS())) {
            return model::player::ArmPose::ThrowSpear;
        }
    }

    // 4) 默认持有物品姿态
    return model::player::ArmPose::Item;
}

PlayerArmPoseResolver::ArmPosePair PlayerArmPoseResolver::resolveArmPoses(::mc::Player& player)
{
    const ::mc::Hand mainHandSlot = ::mc::Hand::MainHand;
    const ::mc::Hand offHandSlot = ::mc::Hand::OffHand;
    auto mainArmPose = determineArmPose(player, mainHandSlot);
    auto offArmPose = determineArmPose(player, offHandSlot);

    // 双手姿态协调：若主手姿态为双手动作（弓、弩装填、弩持握），
    // 副手姿态降级为 Empty（副手空）或 Item（副手非空）
    if (mainArmPose == model::player::ArmPose::BowAndArrow || mainArmPose == model::player::ArmPose::CrossbowCharge ||
        mainArmPose == model::player::ArmPose::CrossbowHold) {
        const ::mc::ItemStack& offHandStack = player.getHeldItem(offHandSlot);
        offArmPose = offHandStack.isEmpty() ? model::player::ArmPose::Empty : model::player::ArmPose::Item;
    }

    // 根据玩家主手偏好映射到模型右臂/左臂
    // 右撇子：主手姿态 → 右臂，副手姿态 → 左臂
    // 左撇子：主手姿态 → 左臂，副手姿态 → 右臂
    if (player.isRightHanded()) {
        return {offArmPose, mainArmPose}; // (left, right)
    }
    return {mainArmPose, offArmPose}; // (left, right)
}

} // namespace mc::client::renderer::entity::renderer::player
