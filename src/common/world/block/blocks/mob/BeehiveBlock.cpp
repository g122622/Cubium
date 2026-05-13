#include "BeehiveBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"

namespace mc {
namespace blocks {

BeehiveBlock::BeehiveBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    // MC 1.16.5: BeehiveBlock 有 FACING 和 HONEY_LEVEL 两个属性
    // HONEY_LEVEL 范围 0-5，表示蜂巢中的蜂蜜量
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::HONEY_LEVEL_0_5())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::HONEY_LEVEL_0_5(), 0));
}

i32 BeehiveBlock::getHoneyLevel(const BlockState& state) const {
    return state.get(BlockStateProperties::HONEY_LEVEL_0_5());
}

BlockState BeehiveBlock::withHoneyLevel(i32 level) const {
    return defaultState().with(BlockStateProperties::HONEY_LEVEL_0_5(), std::clamp(level, 0, 5));
}

BlockState BeehiveBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction facing = context.horizontalDirection();
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

const BlockState& BeehiveBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& BeehiveBlock::mirror(const BlockState& state, Mirror mirror) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = facing;

    switch (mirror) {
        case Mirror::LeftRight:
            // 左右镜像：东西互换
            if (facing == Direction::East) {
                newFacing = Direction::West;
            } else if (facing == Direction::West) {
                newFacing = Direction::East;
            }
            break;
        case Mirror::FrontBack:
            // 前后镜像：南北互换
            if (facing == Direction::North) {
                newFacing = Direction::South;
            } else if (facing == Direction::South) {
                newFacing = Direction::North;
            }
            break;
        case Mirror::None:
        default:
            break;
    }

    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

ActionResultType BeehiveBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // TODO: 收集蜂蜜或打开蜂巢界面
    return ActionResultType::Pass;
}

} // namespace blocks
} // namespace mc
