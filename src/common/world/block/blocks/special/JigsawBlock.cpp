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

#include "JigsawBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/gen/jigsaw/JigsawOrientation.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== JigsawBlock ==========

JigsawBlock::JigsawBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器，添加 ORIENTATION 属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::ORIENTATION())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态：NorthUp
    setDefaultState(
        defaultState().with(BlockStateProperties::ORIENTATION(), world::gen::jigsaw::JigsawOrientation::NorthUp));
}

const BlockState& JigsawBlock::rotate(const BlockState& state, Rotation rotation) const
{
    world::gen::jigsaw::JigsawOrientation orientation = state.get(BlockStateProperties::ORIENTATION());
    world::gen::jigsaw::JigsawOrientation newOrientation =
        world::gen::jigsaw::JigsawOrientations::rotate(orientation, rotation);
    return state.with(BlockStateProperties::ORIENTATION(), newOrientation);
}

const BlockState& JigsawBlock::mirror(const BlockState& state, Mirror mirror) const
{
    world::gen::jigsaw::JigsawOrientation orientation = state.get(BlockStateProperties::ORIENTATION());
    world::gen::jigsaw::JigsawOrientation newOrientation =
        world::gen::jigsaw::JigsawOrientations::mirror(orientation, mirror);
    return state.with(BlockStateProperties::ORIENTATION(), newOrientation);
}

BlockActionResult JigsawBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 游戏管理员方块需要权限才能交互
    if (!player.canUseGameMasterBlocks()) {
        return ActionResultType::Fail;
    }

    // TODO: 打开拼图方块界面
    return ActionResultType::Success;
}

} // namespace blocks
} // namespace mc
