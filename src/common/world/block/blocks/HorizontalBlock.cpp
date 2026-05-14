#include "HorizontalBlock.hpp"
#include "../../../item/context/BlockItemUseContext.hpp"
#include "../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

HorizontalBlock::HorizontalBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器，添加 HORIZONTAL_FACING 属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(FACING()).create(
        [](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 默认朝向为北
    setDefaultState(withFacing(defaultState(), Direction::North));
}

BlockState HorizontalBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 根据玩家水平朝向设置方块方向
    Direction facing = context.horizontalDirection();

    // 某些方块可能需要反向（如熔炉面向玩家，活塞朝向玩家）
    // 子类可以重写此方法来改变行为
    return withFacing(defaultState(), facing);
}

const BlockState& HorizontalBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction currentFacing = getFacing(state);

    // 水平方向旋转
    Direction newFacing = currentFacing;
    switch (rotation) {
        case Rotation::Clockwise90:
            newFacing = Directions::rotateY(currentFacing);
            break;
        case Rotation::Clockwise180:
            newFacing = Directions::opposite(currentFacing);
            break;
        case Rotation::CounterClockwise90:
            newFacing = Directions::rotateYCCW(currentFacing);
            break;
        case Rotation::None:
        default:
            break;
    }

    return withFacing(state, newFacing);
}

const BlockState& HorizontalBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction currentFacing = getFacing(state);
    Direction newFacing = currentFacing;

    switch (mirror) {
        case Mirror::LeftRight:
            // 南北镜像：东西互换
            if (currentFacing == Direction::East) {
                newFacing = Direction::West;
            } else if (currentFacing == Direction::West) {
                newFacing = Direction::East;
            }
            break;
        case Mirror::FrontBack:
            // 前后镜像：南北互换
            if (currentFacing == Direction::North) {
                newFacing = Direction::South;
            } else if (currentFacing == Direction::South) {
                newFacing = Direction::North;
            }
            break;
        case Mirror::None:
        default:
            break;
    }

    return withFacing(state, newFacing);
}

Direction HorizontalBlock::getFacing(const BlockState& state) const
{
    Direction facing = state.get(FACING());
    // 确保朝向是水平的
    MC_ASSERT_DEBUG(Directions::isHorizontal(facing));
    return facing;
}

const BlockState& HorizontalBlock::withFacing(const BlockState& state, Direction facing) const
{
    // 确保朝向是水平的
    MC_ASSERT_DEBUG(Directions::isHorizontal(facing));
    return state.with(FACING(), facing);
}

} // namespace blocks
} // namespace mc
