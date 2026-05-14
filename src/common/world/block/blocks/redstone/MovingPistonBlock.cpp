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
#include "../../../IWorld.hpp"
#include "../../../blockentity/BlockEntity.hpp"
#include "../../../blockentity/interactive/PistonBlockEntity.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

// ============================================================================
// 构造函数
// ============================================================================

MovingPistonBlock::MovingPistonBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器 - 使用与 PistonHeadBlock 相同的属性
    auto container = StateContainer<Block, BlockState>::Builder(*this)
                         .add(BlockStateProperties::FACING())
                         .add(PistonHeadBlock::getTypeProperty())
                         .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                             return std::make_unique<BlockState>(block, std::move(values), id);
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
    // MC Java: onReplaced 中调用 clearPistonTileEntity
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
    // MC Java: createNewTileEntity 返回 null
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

const EnumProperty<PistonHeadBlock::Type>& MovingPistonBlock::getTypeProperty()
{
    // 返回 PistonHeadBlock 的 TYPE_PROP
    return PistonHeadBlock::getTypeProperty();
}

} // namespace blocks
} // namespace mc
