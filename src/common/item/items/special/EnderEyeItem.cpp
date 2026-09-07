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
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/block/state/pattern/BlockInWorld.hpp"
#include "common/world/block/state/pattern/BlockPattern.hpp"
#include "common/world/block/state/pattern/BlockPatternBuilder.hpp"

#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace item::items {

std::unique_ptr<blockpattern::BlockPattern> EnderEyeItem::s_portalShape;

EnderEyeItem::EnderEyeItem(ItemProperties properties)
    : Item(std::move(properties))
{}

ActionResultType EnderEyeItem::onItemUse(ItemUseContext& context)
{
    // 参考: net.minecraft.world.item.EnderEyeItem#useOn
    // 仅当点击的是末地传送门框架且 EYE=false 时生效：
    //   1. 设 EYE=true，pushEntitiesUp（框架升高 13/16→1.0，把上方实体顶起）
    //   2. 播放末影之眼放入框架的事件（levelEvent 1503）
    //   3. 用 BlockPattern 检测完整传送门图案，若匹配则在内部 3×3 区域生成 end_portal 方块
    // 注：物品消耗不在本方法内执行（对齐 Java 版 useOn 不调 stack.shrink）。
    //   消耗由上层 SimulatedPlayer::useItemOnBlock 在 onItemUse 返回 Success 后统一对权威槽
    //   shrink(1) 回写完成（SimulatedPlayer.cpp:400-408）。本方法返回 Success 即触发该消耗。

    const BlockPos& pos = context.blockPos();
    IWorld& world = const_cast<IWorld&>(context.world());
    const BlockState* statePtr = world.getBlockState(pos);

    // 非末地传送门框架或已有眼 → 传递（PASS）
    if (statePtr == nullptr || !statePtr->is(VanillaBlocks::END_PORTAL_FRAME) ||
        statePtr->get(BlockStateProperties::EYE())) {
        return ActionResultType::Pass;
    }

    // 设 EYE=true 并推起上方实体（框架高度从 13/16 升至 1.0）
    const BlockState& newState = statePtr->with(BlockStateProperties::EYE(), true);
    Block::pushEntitiesUp(*statePtr, newState, world, pos);
    world.setBlockState(pos, &newState, 2);

    // TODO: 红石比较器模拟信号输出（updateNeighbourForOutputSignal）暂未实现，跳过。
    //   放入末影之眼后框架的模拟信号输出从 0 变为 15，需通知相邻比较器更新。
    //   待红石比较器模拟信号检测链路完善后补全。

    // 播放末影之眼放入框架的事件（levelEvent 1503）
    world.playEvent(world::WorldEvents::END_PORTAL_FRAME_FILL, pos, 0);

    // 检测完整传送门图案，若匹配则在内部 3×3 区域生成 end_portal 方块
    blockpattern::BlockPattern& portalShape = getOrCreatePortalShape();
    auto match = portalShape.find(world, pos);
    if (match.has_value()) {
        // match.frontTopLeft() 是图案左上角（角落 ? 位置），
        // 内部 3×3 传送门区域从 frontTopLeft.offset(-3, 0, -3) 开始。
        // 参考 Java: blockpatternmatch.getFrontTopLeft().offset(-3, 0, -3)
        BlockPos topLeft = match->frontTopLeft().west(3).north(3);

        const BlockState* endPortal = nullptr;
        if (VanillaBlocks::END_PORTAL != nullptr) {
            endPortal = &VanillaBlocks::END_PORTAL->defaultState();
        }

        if (endPortal != nullptr) {
            for (i32 dx = 0; dx < 3; ++dx) {
                for (i32 dz = 0; dz < 3; ++dz) {
                    BlockPos portalPos = topLeft.east(dx).south(dz);
                    world.setBlockState(portalPos, endPortal, 2);
                }
            }
        }

        // 播放末地传送门生成音效（globalLevelEvent 1038）
        // TODO: globalLevelEvent 暂未在 IWorld 实现，使用 playEvent 替代。
        world.playEvent(world::WorldEvents::END_PORTAL_SPAWN_SOUND, topLeft.east(1).south(1), 0);
    }

    return ActionResultType::Success;
}

blockpattern::BlockPattern& EnderEyeItem::getOrCreatePortalShape()
{
    // 参考: net.minecraft.world.level.block.EndPortalFrameBlock#getOrCreatePortalShape
    // 图案为 5×5 单层（aisle 传入单字符串数组，对应一层深度）：
    //   ? v v v ?      v = 框架(FACING=NORTH, HAS_EYE=true)
    //   > ? ? ? <      > = 框架(FACING=WEST,  HAS_EYE=true)
    //   > ? ? ? <      < = 框架(FACING=EAST,  HAS_EYE=true)
    //   > ? ? ? <      ? = 任意方块（角落与内部区域）
    //   ? ^ ^ ^ ?      ^ = 框架(FACING=SOUTH, HAS_EYE=true)
    // 注：字符串中 ? 用 \? 转义，避免 "??" 被 C++ 解析为 trigraph。
    if (s_portalShape == nullptr) {
        s_portalShape = blockpattern::BlockPatternBuilder::start()
                            .aisle({"?vvv?", ">\?\?\?<", ">\?\?\?<", ">\?\?\?<", "?^^^?"})
                            .where('?', blockpattern::BlockInWorld::hasState([](const BlockState&) { return true; }))
                            .where('^', blockpattern::BlockInWorld::hasState([](const BlockState& s) {
                                return s.is(VanillaBlocks::END_PORTAL_FRAME) && s.get(BlockStateProperties::EYE()) &&
                                    s.get(BlockStateProperties::HORIZONTAL_FACING()) == Direction::South;
                            }))
                            .where('>', blockpattern::BlockInWorld::hasState([](const BlockState& s) {
                                return s.is(VanillaBlocks::END_PORTAL_FRAME) && s.get(BlockStateProperties::EYE()) &&
                                    s.get(BlockStateProperties::HORIZONTAL_FACING()) == Direction::West;
                            }))
                            .where('v', blockpattern::BlockInWorld::hasState([](const BlockState& s) {
                                return s.is(VanillaBlocks::END_PORTAL_FRAME) && s.get(BlockStateProperties::EYE()) &&
                                    s.get(BlockStateProperties::HORIZONTAL_FACING()) == Direction::North;
                            }))
                            .where('<', blockpattern::BlockInWorld::hasState([](const BlockState& s) {
                                return s.is(VanillaBlocks::END_PORTAL_FRAME) && s.get(BlockStateProperties::EYE()) &&
                                    s.get(BlockStateProperties::HORIZONTAL_FACING()) == Direction::East;
                            }))
                            .build();
    }
    return *s_portalShape;
}

} // namespace item::items
} // namespace mc
