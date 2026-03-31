#include "SeaPickleBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {
namespace blocks {

using DoubleBlockHalf = BlockStateProperties::DoubleBlockHalf;

// ========== SeaPickleBlock ==========

SeaPickleBlock::SeaPickleBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::PICKLES_1_4())
        .add(BlockStateProperties::WATERLOGGED())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::PICKLES_1_4(), 1)
        .with(BlockStateProperties::WATERLOGGED(), true));

    // 创建各数量的形状
    // 1个：小型，2个：中型，3个：大型，4个：最大
    m_shapesByCount[0] = CollisionShape::box(0.375f, 0.0f, 0.375f, 0.625f, 0.3125f, 0.625f);
    m_shapesByCount[1] = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 0.375f, 0.75f);
    m_shapesByCount[2] = CollisionShape::box(0.1875f, 0.0f, 0.1875f, 0.8125f, 0.4375f, 0.8125f);
    m_shapesByCount[3] = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 0.5f, 0.875f);
}

i32 SeaPickleBlock::getPickles(const BlockState& state) const {
    return state.get(BlockStateProperties::PICKLES_1_4());
}

BlockState SeaPickleBlock::withPickles(i32 count) const {
    return defaultState().with(BlockStateProperties::PICKLES_1_4(), std::clamp(count, 1, 4));
}

BlockState SeaPickleBlock::getStateForPlacement(BlockItemUseContext& context) {
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查是否在水中
    bool waterlogged = false;  // TODO: 检查流体状态

    // 检查是否已有海泡菜（堆叠）
    const BlockState* existingState = world.getBlockState(pos.x, pos.y, pos.z);
    if (existingState != nullptr && existingState->is(this)) {
        // 增加数量
        i32 count = existingState->get(BlockStateProperties::PICKLES_1_4());
        if (count < 4) {
            return existingState->with(BlockStateProperties::PICKLES_1_4(), count + 1);
        }
        return *existingState;
    }

    return defaultState()
        .with(BlockStateProperties::PICKLES_1_4(), 1)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool SeaPickleBlock::isValidPosition(
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

    // 需要固体支撑
    return belowState->isSolid();
}

BlockState SeaPickleBlock::updatePostPlacement(
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

u8 SeaPickleBlock::getLightLevel(const BlockState& state) const {
    // 在水中时发光，亮度随数量增加
    bool waterlogged = state.get(BlockStateProperties::WATERLOGGED());
    if (!waterlogged) {
        return 0;
    }

    i32 count = getPickles(state);
    // 1个: 6, 2个: 9, 3个: 12, 4个: 15
    return static_cast<u8>(3 + count * 3);
}

const CollisionShape& SeaPickleBlock::getShape(const BlockState& state) const {
    i32 count = getPickles(state);
    return m_shapesByCount[std::clamp(count - 1, 0, 3)];
}

// ========== KelpBlock ==========

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
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    bool waterlogged = true;  // 海带必须在水中

    return defaultState()
        .with(BlockStateProperties::AGE_0_25(), 0)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool KelpBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 检查下方
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);

    if (belowState == nullptr) {
        return false;
    }

    // 可以放置在海带上方或固体方块上
    if (belowState->is(this)) {
        return true;
    }

    // TODO: 检查是否在水中
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
    const BlockState* aboveState = world.getBlockState(abovePos.x, abovePos.y, abovePos.z);

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
        world.setBlockState(abovePos.x, abovePos.y, abovePos.z, &defaultState(), 2);
        world.setBlockState(pos.x, pos.y, pos.z, &withAge(age + 1), 2);
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

// ========== SeagrassBlock ==========

SeagrassBlock::SeagrassBlock(const BlockProperties& properties)
    : Block(properties) {

    // 海草没有特殊状态
    // 形状：小型水下植物
    m_shape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 0.5f, 0.875f);
}

BlockState SeagrassBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

bool SeagrassBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 检查下方支撑
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);

    if (belowState == nullptr) {
        return false;
    }

    // 需要在水中
    // TODO: 检查当前方块是否为水

    return belowState->isSolid();
}

const CollisionShape& SeagrassBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& SeagrassBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== TallSeagrassBlock ==========

TallSeagrassBlock::TallSeagrassBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HALF())
        .add(BlockStateProperties::WATERLOGGED())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HALF(), DoubleBlockHalf::Lower)
        .with(BlockStateProperties::WATERLOGGED(), true));

    // 形状
    m_lowerShape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 1.0f, 0.875f);
    m_upperShape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 1.0f, 0.875f);
}

BlockStateProperties::DoubleBlockHalf TallSeagrassBlock::getHalf(const BlockState& state) const {
    return state.get(BlockStateProperties::HALF());
}

BlockState TallSeagrassBlock::getStateForPlacement(BlockItemUseContext& context) {
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查上方是否有空间
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos.x, abovePos.y, abovePos.z);

    if (aboveState == nullptr || aboveState->isAir()) {
        return defaultState().with(BlockStateProperties::HALF(), DoubleBlockHalf::Lower);
    }

    return defaultState();
}

bool TallSeagrassBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    auto half = getHalf(state);

    if (half == DoubleBlockHalf::Upper) {
        // 上半部分需要在下半部分之上
        BlockPos belowPos(pos.x, pos.y - 1, pos.z);
        const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);
        return belowState != nullptr && belowState->is(this) &&
               belowState->get(BlockStateProperties::HALF()) == DoubleBlockHalf::Lower;
    } else {
        // 下半部分需要支撑
        BlockPos belowPos(pos.x, pos.y - 1, pos.z);
        const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);
        return belowState != nullptr && belowState->isSolid();
    }
}

BlockState TallSeagrassBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    auto half = getHalf(state);

    if (half == DoubleBlockHalf::Upper) {
        // 上半部分检查下方
        if (facing == Direction::Down) {
            if (!facingState.is(this) || facingState.get(BlockStateProperties::HALF()) != DoubleBlockHalf::Lower) {
                if (auto* airState = BlockRegistry::instance().airState()) {
                    return *airState;
                }
            }
        }
    } else {
        // 下半部分检查上方
        if (facing == Direction::Up) {
            if (!facingState.is(this) || facingState.get(BlockStateProperties::HALF()) != DoubleBlockHalf::Upper) {
                // 上方没有上半部分
            }
        }
    }

    return state;
}

const CollisionShape& TallSeagrassBlock::getShape(const BlockState& state) const {
    auto half = getHalf(state);
    return (half == DoubleBlockHalf::Upper) ? m_upperShape : m_lowerShape;
}

const CollisionShape& TallSeagrassBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== BubbleColumnBlock ==========

BubbleColumnBlock::BubbleColumnBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::DRAG())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::DRAG(), false));
}

bool BubbleColumnBlock::isDrag(const BlockState& state) const {
    return state.get(BlockStateProperties::DRAG());
}

BlockState BubbleColumnBlock::getStateForPlacement(BlockItemUseContext& context) {
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 根据下方方块决定是否为下拖
    bool drag = checkSource(world, pos);

    return defaultState().with(BlockStateProperties::DRAG(), drag);
}

bool BubbleColumnBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 气泡柱需要在水中
    // TODO: 检查当前方块是否为水

    // 检查下方是否有岩浆块或灵魂沙
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);

    if (belowState != nullptr) {
        // TODO: 检查是否为岩浆块或灵魂沙
        return true;
    }

    // 或者在另一个气泡柱上方
    // TODO: 检查下方是否为气泡柱

    return false;
}

BlockState BubbleColumnBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    // 检查下方源是否变化
    if (facing == Direction::Down) {
        bool newDrag = checkSource(world, currentPos);
        if (newDrag != isDrag(state)) {
            return state.with(BlockStateProperties::DRAG(), newDrag);
        }
    }

    return state;
}

void BubbleColumnBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    MC_UNUSED(world);
    MC_UNUSED(pos);

    if (isDrag(state)) {
        // 下拖：向下推动实体
        // entity.addVelocity(0, -0.1, 0);
    } else {
        // 上推：向上推动实体
        // entity.addVelocity(0, 0.1, 0);
    }
}

void BubbleColumnBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 更新上方气泡柱
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos.x, abovePos.y, abovePos.z);

    // TODO: 如果上方是水，替换为气泡柱
}

const CollisionShape& BubbleColumnBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

const CollisionShape& BubbleColumnBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

bool BubbleColumnBlock::checkSource(const IWorld& world, const BlockPos& pos) const {
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);

    if (belowState == nullptr) {
        return false;
    }

    // TODO: 检查是否为岩浆块（返回 true，下拖）
    // 灵魂沙返回 false（上推）

    return false;
}

} // namespace blocks
} // namespace mc
