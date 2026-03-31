#include "MushroomBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {
namespace blocks {

// ========== MushroomBlock ==========

MushroomBlock::MushroomBlock(const BlockProperties& properties)
    : Block(properties) {

    // 蘑菇形状：小型圆形
    m_shape = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 0.5f, 0.75f);
}

BlockState MushroomBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

bool MushroomBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 检查下方是否有支撑
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);

    if (belowState == nullptr) {
        return false;
    }

    // 蘑菇可以放置在固体方块上
    // TODO: 检查是否为菌岩、草方块、泥土等
    return belowState->isSolid();
}

void MushroomBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    MC_UNUSED(state);

    // 检查是否可以生长成巨型蘑菇
    // TODO: 实现光照检查和生长逻辑
    // 1. 检查周围空间是否足够
    // 2. 检查光照等级
    // 3. 随机决定是否生长

    // 简化实现：低概率生长
    if (random.nextFloat() < 0.01f) {
        // TODO: 生成巨型蘑菇
    }
}

const CollisionShape& MushroomBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& MushroomBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    // 蘑菇没有碰撞箱
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== HugeMushroomBlock ==========

HugeMushroomBlock::HugeMushroomBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器（6个方向的布尔属性）
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::DOWN())
        .add(BlockStateProperties::UP())
        .add(BlockStateProperties::NORTH())
        .add(BlockStateProperties::SOUTH())
        .add(BlockStateProperties::EAST())
        .add(BlockStateProperties::WEST())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态：所有面都显示蘑菇皮纹理
    setDefaultState(defaultState()
        .with(BlockStateProperties::DOWN(), true)
        .with(BlockStateProperties::UP(), true)
        .with(BlockStateProperties::NORTH(), true)
        .with(BlockStateProperties::SOUTH(), true)
        .with(BlockStateProperties::EAST(), true)
        .with(BlockStateProperties::WEST(), true));
}

const BlockState& HugeMushroomBlock::rotate(const BlockState& state, Rotation rotation) const {
    // 旋转各方向的面
    bool north = state.get(BlockStateProperties::NORTH());
    bool south = state.get(BlockStateProperties::SOUTH());
    bool east = state.get(BlockStateProperties::EAST());
    bool west = state.get(BlockStateProperties::WEST());

    switch (rotation) {
        case Rotation::None:
            return state;
        case Rotation::Clockwise90:
            return state
                .with(BlockStateProperties::NORTH(), west)
                .with(BlockStateProperties::SOUTH(), east)
                .with(BlockStateProperties::EAST(), north)
                .with(BlockStateProperties::WEST(), south);
        case Rotation::Clockwise180:
            return state
                .with(BlockStateProperties::NORTH(), south)
                .with(BlockStateProperties::SOUTH(), north)
                .with(BlockStateProperties::EAST(), west)
                .with(BlockStateProperties::WEST(), east);
        case Rotation::CounterClockwise90:
            return state
                .with(BlockStateProperties::NORTH(), east)
                .with(BlockStateProperties::SOUTH(), west)
                .with(BlockStateProperties::EAST(), south)
                .with(BlockStateProperties::WEST(), north);
        default:
            return state;
    }
}

const BlockState& HugeMushroomBlock::mirror(const BlockState& state, Mirror mirror) const {
    switch (mirror) {
        case Mirror::None:
            return state;
        case Mirror::LeftRight: {
            bool north = state.get(BlockStateProperties::NORTH());
            bool south = state.get(BlockStateProperties::SOUTH());
            return state
                .with(BlockStateProperties::NORTH(), south)
                .with(BlockStateProperties::SOUTH(), north);
        }
        case Mirror::FrontBack: {
            bool east = state.get(BlockStateProperties::EAST());
            bool west = state.get(BlockStateProperties::WEST());
            return state
                .with(BlockStateProperties::EAST(), west)
                .with(BlockStateProperties::WEST(), east);
        }
        default:
            return state;
    }
}

const CollisionShape& HugeMushroomBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape fullShape = CollisionShape::fullBlock();
    return fullShape;
}

} // namespace blocks
} // namespace mc
