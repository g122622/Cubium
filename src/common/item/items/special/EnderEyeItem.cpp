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

#include "EnderEyeItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/Item.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/blocks/end/EndPortalFrameBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/block/state/pattern/BlockInWorld.hpp"
#include "common/world/block/state/pattern/BlockPattern.hpp"
#include "common/world/block/state/pattern/BlockPatternBuilder.hpp"

#include <memory>

namespace mc {
namespace item::items {

EnderEyeItem::EnderEyeItem(ItemProperties properties)
    : Item(std::move(properties))
{}

ActionResultType EnderEyeItem::onItemUse(ItemUseContext& context)
{
    // 对应 MC Java: EnderEyeItem.useOn()
    //
    // 1. 检查目标方块是否为 END_PORTAL_FRAME 且 !hasEye
    // 2. 若是：设置 EYE=true（仅同步客户端），播放 levelEvent(1503)（END_PORTAL_FRAME_FILL）
    // 3. 消耗物品（非创造模式，shrink(1)）
    // 4. 调用 EndPortalFrameBlock::getOrCreatePortalShape().find() 检测 12 框架
    // 5. 若匹配：在内部 3×3 区域放置 END_PORTAL 方块并广播 globalLevelEvent(1038, centerPos, 0)
    const BlockPos& pos = context.blockPos();
    IWorld& world = const_cast<IWorld&>(context.world());
    const BlockState* statePtr = world.getBlockState(pos);

    if (statePtr == nullptr) {
        return ActionResultType::Fail;
    }

    // 检查目标是否为末地传送门框架方块
    const Block& block = statePtr->owner();
    auto* frameBlock = dynamic_cast<const blocks::EndPortalFrameBlock*>(&block);
    if (frameBlock == nullptr) {
        return ActionResultType::Fail;
    }

    // 已经有眼则不再处理
    if (frameBlock->hasEye(*statePtr)) {
        return ActionResultType::Fail;
    }

    // 设置含眼状态：EYE=true
    // 对应 MC Java: level.setBlock(blockpos, blockstate.setValue(HAS_EYE, true), 2);
    const BlockState& newState = statePtr->with(BlockStateProperties::EYE(), true);
    world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_CLIENTS);

    // 播放框架填充音效/粒子事件（事件 1503）
    // 对应 MC Java: level.levelEvent(1503, blockpos, 0);
    world.playEvent(world::WorldEvents::END_PORTAL_FRAME_FILL, pos, 0);

    // 消耗物品（非创造模式）
    Player* player = context.player();
    const bool isCreative = (player != nullptr && player->isCreative());
    if (!isCreative) {
        const_cast<ItemStack&>(context.itemStack()).shrink(1);
    }

    // 检测 12 框架是否全部含眼，形成完整传送门
    // 对应 MC Java 1.21.11:
    //   BlockPattern.BlockPatternMatch match = EndPortalFrameBlock.getOrCreatePortalShape().find(level, blockpos);
    //   if (match != null) {
    //       BlockPos blockpos1 = match.getFrontTopLeft().offset(-3, 0, -3);
    //       for (int i = 0; i < 3; i++) {
    //           for (int j = 0; j < 3; j++) {
    //               BlockPos blockpos2 = blockpos1.offset(i, 0, j);
    //               level.destroyBlock(blockpos2, true, null);
    //               level.setBlock(blockpos2, Blocks.END_PORTAL.defaultBlockState(), 2);
    //           }
    //       }
    //       level.globalLevelEvent(1038, blockpos1.offset(1, 0, 1), 0);
    //   }
    //
    // 注意：MC Java 用 getFrontTopLeft().offset(-3, 0, -3) 硬编码世界坐标偏移，
    // 这依赖于 frontTopLeft 始终位于传送门区域的西北角（即 forwards=SOUTH, up=UP 的匹配方向）。
    // Cubium 的 find() 遍历所有方向组合，frontTopLeft 的语义可能因匹配方向不同而变化，
    // 因此这里改用方向无关的 getBlock() 获取传送门 3×3 区域的精确位置。
    // 模式布局 "?vvv?" / ">???<" / ">???<" / ">???<" / "?^^^?"，
    // 传送门内部 3×3 区域对应模式坐标 (width=1..3, height=1..3, depth=0)。
    const auto portalShape = blocks::EndPortalFrameBlock::getOrCreatePortalShape();
    if (portalShape != nullptr) {
        auto match = portalShape->find(world, pos);
        if (match.has_value()) {
            const BlockState* endPortalState = VanillaBlocks::getState(VanillaBlocks::END_PORTAL);
            if (endPortalState != nullptr) {
                // 传送门 3×3 区域：模式坐标 (width=1+i, height=1+j, depth=0)
                for (i32 i = 0; i < 3; ++i) {
                    for (i32 j = 0; j < 3; ++j) {
                        const BlockPos portalPos = match->getBlock(1 + i, 1 + j, 0).pos();
                        // TODO: 对齐 MC Java level.destroyBlock(portalPos, true, null) —
                        //   放置 END_PORTAL 前需先销毁该位置的方块（掉落物 + 移除）。
                        //   Cubium 目前缺少 destroyBlock 等价接口，暂直接覆盖放置。
                        world.setBlockState(portalPos, endPortalState, world::BlockUpdateFlags::UPDATE_CLIENTS);
                    }
                }
            }

            // 广播末地传送门激活音效（全服跨维度）
            // 对应 MC Java: level.globalLevelEvent(1038, blockpos1.offset(1, 0, 1), 0);
            // 事件位置为传送门 3×3 区域的中心。
            const BlockPos centerPos = match->getBlock(2, 2, 0).pos();
            world.globalLevelEvent(world::WorldEvents::END_PORTAL_SPAWN_SOUND, centerPos, 0);
        }
    }

    return ActionResultType::Success;
}

} // namespace item::items
} // namespace mc
