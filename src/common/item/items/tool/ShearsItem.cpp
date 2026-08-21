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

#include "ShearsItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/interfaces/IShearable.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <utility>
#include <vector>

namespace mc {
namespace item {
namespace tool {

ShearsItem::ShearsItem(ItemProperties properties)
    : Item(std::move(properties))
{}

f32 ShearsItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const
{
    (void)stack;

    // 对蜘蛛网返回 15.0（高效率）
    if (VanillaBlocks::COBWEB && &state.owner() == VanillaBlocks::COBWEB) {
        return 15.0f;
    }

    // 使用 BlockTags 检查树叶
    if (BlockTags::LEAVES().contains(state)) {
        return 15.0f;
    }

    // 对羊毛返回 5.0
    if (BlockTags::WOOL().contains(state)) {
        return 5.0f;
    }

    // 其他方块返回基础速度
    return 1.0f;
}

bool ShearsItem::canHarvestBlock(const BlockState& state) const
{
    // 剪刀可以采集蜘蛛网、红石线、绊线
    if (VanillaBlocks::COBWEB && &state.owner() == VanillaBlocks::COBWEB) {
        return true;
    }
    if (VanillaBlocks::REDSTONE_WIRE && &state.owner() == VanillaBlocks::REDSTONE_WIRE) {
        return true;
    }
    if (VanillaBlocks::TRIPWIRE && &state.owner() == VanillaBlocks::TRIPWIRE) {
        return true;
    }

    // 其他方块使用默认采集规则
    return state.getHarvestTool() == TOOL_TYPE_NONE;
}

bool ShearsItem::onBlockDestroyed(
    ItemStack& stack, IWorld& world, const BlockState& state, const BlockPos& pos, LivingEntity& entity)
{
    (void)world;
    (void)pos;
    (void)entity;

    // 以下方块不消耗耐久：树叶、羊毛、蛛网、草、蕨、枯萎灌木、藤蔓、绊线、火
    if (BlockTags::LEAVES().contains(state)) {
        return true; // 树叶不消耗耐久
    }
    if (BlockTags::WOOL().contains(state)) {
        return true; // 羊毛不消耗耐久
    }

    // 检查特定方块
    const Block& block = state.owner();
    if (VanillaBlocks::COBWEB && &block == VanillaBlocks::COBWEB) {
        return true; // 蛛网不消耗耐久
    }
    if (VanillaBlocks::SHORT_GRASS && &block == VanillaBlocks::SHORT_GRASS) {
        return true; // 草不消耗耐久
    }
    if (VanillaBlocks::FERN && &block == VanillaBlocks::FERN) {
        return true; // 蕨不消耗耐久
    }
    if (VanillaBlocks::DEAD_BUSH && &block == VanillaBlocks::DEAD_BUSH) {
        return true; // 枯萎灌木不消耗耐久
    }
    if (VanillaBlocks::VINE && &block == VanillaBlocks::VINE) {
        return true; // 藤蔓不消耗耐久
    }
    if (VanillaBlocks::TRIPWIRE && &block == VanillaBlocks::TRIPWIRE) {
        return true; // 绊线不消耗耐久
    }

    // 火方块不消耗耐久
    if (BlockTags::FIRE().contains(state)) {
        return true;
    }

    // 其他硬度>0的方块消耗耐久，若物品损坏则触发 onEquippedItemBroken 回调
    if (state.hardness() > 0.0f) {
        LivingEntity::hurtAndBreak(stack, 1, &entity, EquipmentSlot::MainHand);
    }
    return true;
}

bool ShearsItem::itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand)
{
    // stack 参数由调用方传入（Player::interactOn 传 getHeldItem 值拷贝 Player.cpp:2858，
    // MobEntity 传权威手持引用），本方法不使用 stack（耐久损耗改用权威手持，见下），显式弃用。
    (void)stack;

    // 剪刀可以剪羊毛、雪傀儡的南瓜、哞菇的蘑菇

    // 检查实体是否实现 IShearable 接口
    auto* shearable = dynamic_cast<entity::IShearable*>(&target);
    if (shearable == nullptr) {
        return false;
    }

    // 检查是否可以被剪
    if (!shearable->isShearable()) {
        return false;
    }

    // 执行剪毛
    std::vector<ItemStack> drops = shearable->shear(&player);

    // 在世界中生成掉落物
    IWorld* world = target.world();
    if (world != nullptr && !drops.empty()) {
        math::Random& rng = world->getRandom();

        for (auto& drop : drops) {
            if (!drop.isEmpty()) {
                // 使用 ItemDropHelper 统一生成物品实体
                ItemDropHelper::spawnItemEntity(world,
                    drop,
                    target.x(),
                    target.y() + 0.5,
                    target.z(),
                    rng,
                    ItemDropHelper::DEFAULT_PICKUP_DELAY,
                    player.uuid());
            }
        }
    }

    // 消耗剪刀耐久（非创造模式），若物品损坏则触发 onEquippedItemBroken 回调。
    // stack 参数由调用方传入（Player::interactOn 传 getHeldItem 值拷贝 Player.cpp:2858，
    // MobEntity 传权威手持引用），为统一两路径行为，直接以 player.getHeldItem(hand) 为权威
    // 手持源做 hurtAndBreak（耐久回写权威槽），同 GoldenAppleItem/BucketItem 修复范式。
    if (!player.isCreative()) {
        ItemStack& heldItem = player.getHeldItem(hand);
        LivingEntity::hurtAndBreak(heldItem, 1, &player, LivingEntity::handToEquipmentSlot(hand));
    }

    return true;
}

} // namespace tool
} // namespace item
} // namespace mc
