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
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
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
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态：NorthUp
    setDefaultState(
        defaultState().with(BlockStateProperties::ORIENTATION(), world::gen::jigsaw::JigsawOrientation::NorthUp));
}

BlockState JigsawBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 对齐 vanilla JigsawBlock.getStateForPlacement：front 取点击面；点击面为竖直轴时 top 取玩家
    // 水平朝向的反方向（方块顶面背对玩家），否则 top 为 Up。
    const Direction clickedFace = context.getClickedFace();
    const Direction top = Directions::getAxis(clickedFace) == Axis::Y
        ? Directions::opposite(context.horizontalDirection())
        : Direction::Up;
    return defaultState().with(BlockStateProperties::ORIENTATION(),
        world::gen::jigsaw::JigsawOrientations::fromFacingAndRotation(clickedFace, top));
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

BlockActionResult JigsawBlock::useWithoutItem(
    const BlockState& state, IWorld& world, const BlockPos& pos, Player& player, const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(hit);

    // 游戏管理员方块需要权限才能交互
    if (!player.canUseGameMasterBlocks()) {
        return ActionResultType::Fail;
    }

    // 对齐 vanilla JigsawBlock.useWithoutItem：缺失拼图方块实体（结构生成 NBT 注入管道未接入）
    // 或无权限时返回 PASS/FAIL，仅当两者都满足时打开界面。
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity == nullptr || entity->getType() != BlockEntityType::JigsawBlock || !player.canUseGameMasterBlocks()) {
        return ActionResultType::Pass;
    }

    // TODO(jigsaw-screen): 对齐 vanilla Player.openJigsawBlock 打开拼图方块编辑界面，
    //   服务端需接入容器菜单/界面同步体系后在此打开；当前仅返回 SUCCESS。
    return ActionResultType::Success;
}

bool JigsawBlock::canAttach(world::gen::jigsaw::JigsawOrientation sourceOrientation,
    const std::string& sourceTarget,
    world::gen::jigsaw::JigsawOrientation targetOrientation,
    const std::string& targetName,
    bool sourceJointRollable)
{
    // 对齐 vanilla JigsawBlock.canAttach：
    //   front 互为反方向，且（rollable 或两侧 top 相同），且 source.target == target.name。
    namespace jigsaw = world::gen::jigsaw;
    const Direction sourceFront = jigsaw::JigsawOrientations::getFacing(sourceOrientation);
    const Direction targetFront = jigsaw::JigsawOrientations::getFacing(targetOrientation);
    if (sourceFront != Directions::opposite(targetFront)) {
        return false;
    }
    if (!sourceJointRollable) {
        const Direction sourceTop = jigsaw::JigsawOrientations::getRotation(sourceOrientation);
        const Direction targetTop = jigsaw::JigsawOrientations::getRotation(targetOrientation);
        if (sourceTop != targetTop) {
            return false;
        }
    }
    return sourceTarget == targetName;
}

Direction JigsawBlock::getFrontFacing(const BlockState& state)
{
    return world::gen::jigsaw::JigsawOrientations::getFacing(state.get(BlockStateProperties::ORIENTATION()));
}

Direction JigsawBlock::getTopFacing(const BlockState& state)
{
    return world::gen::jigsaw::JigsawOrientations::getRotation(state.get(BlockStateProperties::ORIENTATION()));
}

} // namespace blocks
} // namespace mc
