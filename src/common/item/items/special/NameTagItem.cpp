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

#include "NameTagItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/text/ITextComponent.hpp"
#include <utility>

namespace mc {
namespace item::items {

NameTagItem::NameTagItem(ItemProperties properties)
    : Item(std::move(properties))
{}

bool NameTagItem::itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand)
{
    // stack 参数由调用方传入（Player::interactOn 传 getHeldItem 值拷贝 Player.cpp:2858，
    // MobEntity 传权威手持引用）。本方法读取 stack 的自定义名（hasCustomName/getCustomNameComponent
    // 为只读，拷贝与权威一致，无回写问题）。仅 shrink 消耗须改用权威手持，见下。
    // 消耗统一以 player.getHeldItem(hand) 为权威源（同 GoldenAppleItem/SaddleItem 修复范式）。

    // 检查物品是否有自定义名称
    if (!stack.hasCustomName()) {
        return false;
    }

    // 不能给玩家命名
    if (dynamic_cast<Player*>(&target) != nullptr) {
        return false;
    }

    // 只能对 MobEntity 命名（非玩家的生物实体）
    auto* mob = dynamic_cast<MobEntity*>(&target);
    if (mob == nullptr) {
        return false;
    }

    // 确保实体存活
    if (!mob->isAlive()) {
        return false;
    }

    // 获取物品的自定义名称
    const text::ITextComponent* customName = stack.getCustomNameComponent();
    if (customName == nullptr) {
        return false;
    }

    // 设置实体的自定义名称
    mob->setCustomNameComponent(customName->deepCopy());

    // 命名牌命名后，实体变为持久化（不会消失）
    mob->enablePersistence();

    // 消耗一个物品（非创造模式）：直接操作玩家权威手持物（player.getHeldItem(hand) 返回引用），
    // 而非 stack 参数（Player 路径下是值拷贝，shrink 不回写权威物品栏——持多个命名牌命名生物时
    // 不消耗的对齐缺陷）。MobEntity 路径下 stack 已是权威引用，与 getHeldItem 等价，不影响。
    if (!player.isCreative()) {
        ItemStack& heldItem = player.getHeldItem(hand);
        heldItem.shrink(1);
    }

    return true;
}

} // namespace item::items
} // namespace mc
