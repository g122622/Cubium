/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "AbstractChestedHorseEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"

namespace mc {

ActionResultType AbstractChestedHorseEntity::interactMob(Player& player, Hand hand)
{
    // 在基类逻辑之前增加箱子装备判断

    bool isSecondaryUse = !isChild() && isTame() && player.isSneaking();

    // 如果没有被骑乘且不是打开容器的场景
    if (!isBeingRidden() && !isSecondaryUse) {
        ItemStack& itemStack = player.getHeldItem(hand);
        const Item* item = itemStack.getItem();

        if (item != nullptr && !itemStack.isEmpty()) {
            // 1. 手持食物时优先喂食
            if (isFoodItem(itemStack)) {
                bool hadEffect = handleEating(&player, itemStack);
                if (hadEffect) {
                    if (m_world != nullptr && m_world->isClientSide()) {
                        return ActionResultType::Consume;
                    }
                    return ActionResultType::Success;
                }
            }

            // 2. 未驯服时让马愤怒
            if (!isTame()) {
                makeMad();
                return ActionResultType::Success;
            }

            // 3. 手持箱子且未装备箱子时装备箱子
            if (!hasChest() && item == Items::CHEST) {
                equipChest(player, itemStack);
                return ActionResultType::Success;
            }
        }
        // 交给基类处理（鞍、马铠装备和骑乘）
        return AbstractHorseEntity::interactMob(player, hand);
    }

    // 被骑乘中或 Shift+已驯服，交给基类处理
    return AbstractHorseEntity::interactMob(player, hand);
}

void AbstractChestedHorseEntity::equipChest(Player& player, ItemStack& itemStack)
{
    // 设置箱子标志、播放音效、消耗物品、重建背包

    setChest(true);

    // 播放箱子装备音效
    IWorld* world = this->world();
    if (world != nullptr) {
        world->playSound(getChestEquipSound(), sound::SoundCategory::Neutral, position(), 1.0f, 1.0f);
    }

    // 消耗一个箱子物品（非创造模式）
    if (!player.isCreative()) {
        itemStack.shrink(1);
    }

    // 重建背包以扩展槽位数
    // 装备箱子后 getInventorySize() 会返回更大的值
    initHorseChest();
}

const ResourceLocation& AbstractChestedHorseEntity::getChestEquipSound() const
{
    // 默认使用驴的箱子装备音效，子类（如 LlamaEntity）可覆写
    return SoundEvents::ENTITY_DONKEY_CHEST;
}

} // namespace mc
