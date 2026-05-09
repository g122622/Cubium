#include "KelpBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../WaterLoggableHelpers.hpp"
#include "../../BlockRegistry.hpp"
#include "../../VanillaBlocks.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

KelpBlock::KelpBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::AGE_0_25())
        .add(BlockStateProperties::WATERLOGGED())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::AGE_0_25(), 0)
        .with(BlockStateProperties::WATERLOGGED(), true));

    // 海带形状：细长
    m_shape = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 1.0f, 0.75f);
}

i32 KelpBlock::getAge(const BlockState& state) const {
    return state.get(BlockStateProperties::AGE_0_25());
}

BlockState KelpBlock::withAge(i32 age) const {
    return defaultState().with(BlockStateProperties::AGE_0_25(), std::min(age, 25));
}

BlockState KelpBlock::getStateForPlacement(BlockItemUseContext& context) {
    MC_UNUSED(context);
    // 海带必须在水中
    return defaultState()
        .with(BlockStateProperties::AGE_0_25(), 0)
        .with(BlockStateProperties::WATERLOGGED(), true);
}

bool KelpBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 检查下方
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 可以放置在海带上方或固体方块上
    if (belowState->is(this)) {
        return true;
    }

    if (VanillaBlocks::KELP_PLANT != nullptr && belowState->is(VanillaBlocks::KELP_PLANT)) {
        return true;
    }

    return belowState->isSolid();
}

BlockState KelpBlock::updatePostPlacement(
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

void KelpBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 检查上方是否有空间
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState != nullptr && !aboveState->isAir()) {
        return;  // 上方被占用
    }

    // 检查高度限制（基于年龄）
    i32 age = getAge(state);
    if (age >= 25) {
        return;  // 已达到最大高度
    }

    // 随机生长
    if (random.nextFloat() < 0.14f) {  // 约14%概率
        // 增加上方海带
        const BlockState& kelpState = defaultState();
        world.setBlockState(abovePos, &kelpState, 2);
        const BlockState& agedState = withAge(age + 1);
        world.setBlockState(pos, &agedState, 2);
    }
}

const CollisionShape& KelpBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& KelpBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

} // namespace blocks
} // namespace mc
