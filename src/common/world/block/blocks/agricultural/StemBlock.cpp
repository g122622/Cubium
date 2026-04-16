#include "StemBlock.hpp"
#include "../../VanillaBlocks.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include <algorithm>

namespace mc {
namespace blocks {

// ========== StemBlock 实现 ==========

StemBlock::StemBlock(const StemGrownBlock* crop, const BlockProperties& properties)
    : BushBlock(properties)
    , m_crop(crop) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::AGE_0_7())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::AGE_0_7(), 0));

    // 预计算各生长阶段的形状
    // 茎是居中的小柱子，随年龄增长高度增加
    constexpr f32 P = 1.0f / 16.0f;
    constexpr f32 heights[] = {2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f};

    for (int i = 0; i < 8; ++i) {
        // 茎是居中的 2x2 像素柱子
        m_shapesByAge[i] = CollisionShape::box(7.0f * P, 0.0f, 7.0f * P, 9.0f * P, heights[i] * P, 9.0f * P);
    }
}

int StemBlock::getAge(const BlockState& state) const {
    return state.get(BlockStateProperties::AGE_0_7());
}

const BlockState& StemBlock::withAge(int age) const {
    return defaultState().with(BlockStateProperties::AGE_0_7(), std::min(age, getMaxAge()));
}

bool StemBlock::isMaxAge(const BlockState& state) const {
    return getAge(state) >= getMaxAge();
}

BlockState StemBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState().with(BlockStateProperties::AGE_0_7(), 0);
}

bool StemBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 检查下方是否为耕地
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    return canSustain(*belowState, world, belowPos);
}

void StemBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 如果已经成熟，尝试生成果实
    if (isMaxAge(state)) {
        tryGrowFruit(state, world, pos, random);
        return;
    }

    const BlockPos abovePos = pos.up();
    const i32 blockLight = static_cast<i32>(world.getBlockLight(abovePos));
    const i32 skyLight = static_cast<i32>(world.getSkyLight(abovePos));
    if (std::max(blockLight, skyLight) < 9) {
        return;
    }

    // 茎类作物成长速度与普通作物接近，使用基础随机概率。
    if (random.nextInt(25) == 0) {
        world.setBlockState(pos, &withAge(getAge(state) + 1), 2);
    }
}

void StemBlock::grow(IWorld& world, const BlockPos& pos, const BlockState& state) {
    int newAge = std::min(getAge(state) + 2 + (rand() % 4), getMaxAge());
    const BlockState& newState = withAge(newAge);
    world.setBlockState(pos, &newState, 2);

    // 如果达到最大年龄，尝试生成果实
    if (newAge == getMaxAge()) {
        math::Random random;  // TODO: 使用世界随机数
        tryGrowFruit(newState, world, pos, random);
    }
}

const CollisionShape& StemBlock::getShape(const BlockState& state) const {
    int age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 7);
    return m_shapesByAge[age];
}

bool StemBlock::canSustain(
    const BlockState& groundState,
    IWorld& world,
    const BlockPos& groundPos) const {

    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    return VanillaBlocks::FARMLAND != nullptr && groundState.is(VanillaBlocks::FARMLAND);
}

bool StemBlock::tryGrowFruit(const BlockState& state, IWorld& world, const BlockPos& pos, math::IRandom& random) {
    MC_UNUSED(state);

    // 随机选择一个水平方向
    Direction directions[] = {Direction::North, Direction::South, Direction::East, Direction::West};
    Direction dir = directions[random.nextInt(4)];

    BlockPos fruitPos(pos.x + Directions::xOffset(dir), pos.y, pos.z + Directions::zOffset(dir));

    // 检查果实位置是否为空
    const BlockState* fruitState = world.getBlockState(fruitPos);
    if (fruitState != nullptr && !fruitState->isAir()) {
        return false;
    }

    // 检查果实下方是否可以支撑
    BlockPos belowFruitPos(fruitPos.x, fruitPos.y - 1, fruitPos.z);
    const BlockState* belowFruitState = world.getBlockState(belowFruitPos);

    if (belowFruitState == nullptr) {
        return false;
    }

    const bool canSupportFruit =
        (VanillaBlocks::FARMLAND != nullptr && belowFruitState->is(VanillaBlocks::FARMLAND)) ||
        (VanillaBlocks::DIRT != nullptr && belowFruitState->is(VanillaBlocks::DIRT)) ||
        (VanillaBlocks::GRASS_BLOCK != nullptr && belowFruitState->is(VanillaBlocks::GRASS_BLOCK));
    if (!canSupportFruit) {
        return false;
    }

    // 放置果实
    if (m_crop != nullptr) {
        const BlockState& cropDefaultState = m_crop->defaultState();
        world.setBlockState(fruitPos, &cropDefaultState, 3);

        // 将茎变为连接茎
        // TODO: 获取对应的 AttachedStemBlock 并设置朝向
        // const Block* attachedStem = m_crop->getAttachedStem();
        // if (attachedStem != nullptr) {
        //     BlockState stemState = attachedStem->defaultState()
        //         .with(BlockStateProperties::HORIZONTAL_FACING(), dir);
        //     world.setBlockState(pos.x, pos.y, pos.z, &stemState, 3);
        // }

        return true;
    }

    return false;
}

// ========== AttachedStemBlock 实现 ==========

AttachedStemBlock::AttachedStemBlock(const StemGrownBlock* crop, const BlockProperties& properties)
    : BushBlock(properties)
    , m_crop(crop) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));

    // 连接茎形状（细长横杆）
    constexpr f32 P = 1.0f / 16.0f;
    m_shape = CollisionShape::box(7.0f * P, 0.0f, 7.0f * P, 9.0f * P, 16.0f * P, 9.0f * P);
}

BlockState AttachedStemBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), context.horizontalDirection());
}

const BlockState& AttachedStemBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& AttachedStemBlock::mirror(const BlockState& state, Mirror mirror) const {
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

} // namespace blocks
} // namespace mc
