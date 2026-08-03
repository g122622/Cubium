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

#include "MovingPistonBlock.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/redstone/PistonHeadBlock.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/interactive/PistonBlockEntity.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// 构造函数
// ============================================================================

MovingPistonBlock::MovingPistonBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器 - 使用与 PistonHeadBlock 相同的属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::FACING())
            .add(PistonHeadBlock::getTypeProperty())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::FACING(), Direction::North)
            .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal));
}

// ============================================================================
// Block 接口实现
// ============================================================================

void MovingPistonBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 当方块被移除时，清理 PistonBlockEntity
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity) {
        auto* pistonEntity = dynamic_cast<blockentity::PistonBlockEntity*>(entity);
        if (pistonEntity) {
            pistonEntity->clearPistonBlockEntity(world);
        }
    }

    // 调用基类实现
    Block::onBlockRemoved(world, pos, state);
}

std::unique_ptr<BlockEntity> MovingPistonBlock::createBlockEntity(const BlockPos& pos)
{
    MC_UNUSED(pos);
    // 实际的 PistonBlockEntity 由 PistonBlock.extend() 创建
    // 这里返回 nullptr，因为方块实体是由活塞操作时创建的
    return nullptr;
}

// ============================================================================
// MovingPistonBlock 特有方法
// ============================================================================

Direction MovingPistonBlock::getFacing(const BlockState& state)
{
    return state.get(BlockStateProperties::FACING());
}

PistonHeadBlock::Type MovingPistonBlock::getType(const BlockState& state)
{
    return state.get(PistonHeadBlock::getTypeProperty());
}

BlockState MovingPistonBlock::withType(BlockState state, PistonHeadBlock::Type type)
{
    return state.with(PistonHeadBlock::getTypeProperty(), type);
}

const EnumProperty<PistonHeadBlock::Type>& MovingPistonBlock::_getTypeProperty()
{
    // 返回 PistonHeadBlock 的 TYPE_PROP
    return PistonHeadBlock::getTypeProperty();
}

} // namespace blocks
} // namespace mc
