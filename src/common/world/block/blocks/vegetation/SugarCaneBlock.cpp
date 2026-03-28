#include "SugarCaneBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "../../../../item/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {
namespace blocks {

SugarCaneBlock::SugarCaneBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::AGE_0_15())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::AGE_0_15(), 0));

    // 甘蔗形状：稍小的方块
    m_shape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 1.0f, 0.875f);
}

i32 SugarCaneBlock::getAge(const BlockState& state) const {
    return state.get(BlockStateProperties::AGE_0_15());
}

BlockState SugarCaneBlock::withAge(i32 age) const {
    return defaultState().with(BlockStateProperties::AGE_0_15(), std::min(age, 15));
}

BlockState SugarCaneBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

bool SugarCaneBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 检查下方方块
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);

    if (belowState == nullptr) {
        return false;
    }

    // 甘蔗可以放置在甘蔗上
    if (belowState->is(this)) {
        return true;
    }

    // TODO: 检查是否为草方块、泥土、沙子、红沙等
    const Material& material = belowState->getMaterial();
    if (material.isSolid()) {
        // 检查是否靠近水源
        return isNearWater(world, pos);
    }

    return false;
}

bool SugarCaneBlock::isNearWater(IBlockReader& world, const BlockPos& pos) const {
    // 检查四个方向是否有水
    for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
        BlockPos adjPos = pos.offset(dir);
        const BlockState* adjState = world.getBlockState(adjPos.x, adjPos.y, adjPos.z);

        if (adjState != nullptr) {
            // TODO: 检查是否为水方块
            const Material& material = adjState->getMaterial();
            if (material.isLiquid()) {
                return true;
            }
        }
    }
    return false;
}

BlockState SugarCaneBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查下方支撑
    if (facing == Direction::Down) {
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (!isValidPosition(state, blockReader, currentPos)) {
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    return state;
}

void SugarCaneBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 检查上方是否有空间
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos.x, abovePos.y, abovePos.z);

    if (aboveState != nullptr && !aboveState->isAir()) {
        return;
    }

    // 检查高度限制（最高3格）
    int height = 1;
    for (int i = 1; i < 3; ++i) {
        BlockPos checkPos(pos.x, pos.y - i, pos.z);
        const BlockState* checkState = world.getBlockState(checkPos.x, checkPos.y, checkPos.z);
        if (checkState == nullptr || !checkState->is(this)) {
            break;
        }
        height++;
    }

    if (height >= 3) {
        return;  // 已达到最高高度
    }

    // 随机生长
    if (random.nextInt(16) == 0) {
        i32 age = getAge(state);
        if (age >= 15) {
            // 生长新的甘蔗
            world.setBlockState(abovePos.x, abovePos.y, abovePos.z, &defaultState(), 2);
            world.setBlockState(pos.x, pos.y, pos.z, &withAge(0), 2);
        } else {
            // 增加年龄
            world.setBlockState(pos.x, pos.y, pos.z, &withAge(age + 1), 2);
        }
    }
}

const CollisionShape& SugarCaneBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& SugarCaneBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

} // namespace blocks
} // namespace mc
