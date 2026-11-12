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

#include "common/world/block/blocks/decorative/PlayerHeadBlock.hpp"

#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/blockentity/interactive/SkullBlockEntity.hpp"
#include <string>

namespace mc {
namespace blocks {

namespace {

/**
 * @brief 把物品上的 `minecraft:note_block_sound` 组件写入头颅方块实体
 *
 * 该组件对任意头颅都有效（玩家头、生物头皆可），头颅放在音符盒上方时音符盒改用这个
 * 声音。原版在 `SkullBlock` 的放置路径统一搬运该组件，这里同样做成共享助手，
 * 避免四个 `onBlockPlacedBy` 重写里各写一遍。
 */
void _applyNoteBlockSoundToSkull(IWorld& world, const BlockPos& pos, const ItemStack& stack)
{
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr) {
        return;
    }
    auto* skull = dynamic_cast<blockentity::SkullBlockEntity*>(blockEntity);
    if (skull == nullptr) {
        return;
    }

    // 组件缺失时不动方块实体（保持其原有值），而不是清空——
    // 结构模板放置等场景可能已经写好了值。
    const std::string* sound = stack.getNoteBlockSound();
    if (sound != nullptr) {
        skull->setNoteBlockSound(*sound);
    }
}

} // namespace

// ============================================================================
// PlayerHeadBlock / PlayerWallHeadBlock
// ============================================================================

PlayerHeadBlock::PlayerHeadBlock(const BlockProperties& properties)
    : SkullBlock(SkullType::Player, properties)
{}

void PlayerHeadBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const Entity* placer, const ItemStack& stack)
{
    MC_UNUSED(state);
    MC_UNUSED(placer);
    // 从物品的 SkullOwner 组件恢复档案写入方块实体
    if (BlockEntity* be = world.getBlockEntity(pos)) {
        auto* skull = dynamic_cast<blockentity::SkullBlockEntity*>(be);
        if (skull != nullptr) {
            const nlohmann::json* tag = stack.getTag();
            if (tag != nullptr && tag->contains("SkullOwner")) {
                auto result = skin::GameProfile::fromJson((*tag)["SkullOwner"]);
                if (result.success()) {
                    skull->setOwnerProfile(result.value());
                }
            }
        }
    }
    _applyNoteBlockSoundToSkull(world, pos, stack);
}

PlayerWallHeadBlock::PlayerWallHeadBlock(const BlockProperties& properties)
    : WallSkullBlock(SkullType::Player, properties)
{}

void PlayerWallHeadBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const Entity* placer, const ItemStack& stack)
{
    MC_UNUSED(state);
    MC_UNUSED(placer);
    if (BlockEntity* be = world.getBlockEntity(pos)) {
        auto* skull = dynamic_cast<blockentity::SkullBlockEntity*>(be);
        if (skull != nullptr) {
            const nlohmann::json* tag = stack.getTag();
            if (tag != nullptr && tag->contains("SkullOwner")) {
                auto result = skin::GameProfile::fromJson((*tag)["SkullOwner"]);
                if (result.success()) {
                    skull->setOwnerProfile(result.value());
                }
            }
        }
    }
    _applyNoteBlockSoundToSkull(world, pos, stack);
}

} // namespace blocks
} // namespace mc
