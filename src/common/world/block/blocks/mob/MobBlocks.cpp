#include "MobBlocks.hpp"
#include "../../../IWorld.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../core/BlockRaycastResult.hpp"

namespace mc {
namespace blocks {

// ========== BeehiveBlock ==========

BeehiveBlock::BeehiveBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    // TODO: 添加 HONEY_LEVEL_0_5 属性
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));
}

i32 BeehiveBlock::getHoneyLevel(const BlockState& state) const {
    MC_UNUSED(state);
    // TODO: 返回蜂蜜等级
    return 0;
}

BlockState BeehiveBlock::withHoneyLevel(i32 level) const {
    MC_UNUSED(level);
    // TODO: 设置蜂蜜等级
    return defaultState();
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
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rotation);
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

// ========== TurtleEggBlock ==========

TurtleEggBlock::TurtleEggBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::EGGS_1_4())
        .add(BlockStateProperties::HATCH_0_2())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::EGGS_1_4(), 1)
        .with(BlockStateProperties::HATCH_0_2(), 0));

    // 创建各蛋数量的形状
    m_shapesByEggCount[0] = CollisionShape::box(0.375f, 0.0f, 0.375f, 0.625f, 0.5f, 0.625f);
    m_shapesByEggCount[1] = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 0.5f, 0.75f);
    m_shapesByEggCount[2] = CollisionShape::box(0.1875f, 0.0f, 0.1875f, 0.8125f, 0.5f, 0.8125f);
    m_shapesByEggCount[3] = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 0.5f, 0.875f);
}

i32 TurtleEggBlock::getEggs(const BlockState& state) const {
    return state.get(BlockStateProperties::EGGS_1_4());
}

BlockState TurtleEggBlock::withEggs(i32 count) const {
    return defaultState().with(BlockStateProperties::EGGS_1_4(), std::clamp(count, 1, 4));
}

i32 TurtleEggBlock::getHatch(const BlockState& state) const {
    return state.get(BlockStateProperties::HATCH_0_2());
}

BlockState TurtleEggBlock::withHatch(i32 hatch) const {
    return defaultState().with(BlockStateProperties::HATCH_0_2(), std::clamp(hatch, 0, 2));
}

BlockState TurtleEggBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 随机放置1-4个蛋
    i32 eggs = 1;  // TODO: 随机数
    return defaultState().with(BlockStateProperties::EGGS_1_4(), eggs);
}

bool TurtleEggBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 海龟蛋需要放在沙子上
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);

    if (belowState == nullptr) {
        return false;
    }

    // TODO: 检查是否为沙子
    return belowState->isSolid();
}

void TurtleEggBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 孵化逻辑
    i32 hatch = getHatch(state);
    if (hatch < 2 && random.nextInt(100) < 10) {
        // 孵化进度增加
        world.setBlockState(pos.x, pos.y, pos.z, &withHatch(hatch + 1), 2);
    } else if (hatch >= 2) {
        // 孵化完成，生成海龟
        // TODO: 生成海龟
        i32 eggs = getEggs(state);
        if (eggs > 1) {
            world.setBlockState(pos.x, pos.y, pos.z, &withEggs(eggs - 1).with(BlockStateProperties::HATCH_0_2(), 0), 2);
        } else {
            world.setBlockState(pos.x, pos.y, pos.z, nullptr, 2);
        }
    }
}

void TurtleEggBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    // 踩破蛋的逻辑
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(entity);
    // TODO: 如果实体重量足够，踩破蛋
}

const CollisionShape& TurtleEggBlock::getShape(const BlockState& state) const {
    i32 eggs = getEggs(state);
    return m_shapesByEggCount[static_cast<std::size_t>(std::min(eggs - 1, 3))];
}

// ========== InfestedBlock ==========

InfestedBlock::InfestedBlock(u32 hostBlock, const BlockProperties& properties)
    : Block(properties)
    , m_hostBlock(hostBlock) {
    // 被感染方块没有特殊状态
}

void InfestedBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // TODO: 生成蠹虫
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
}

// ========== SpawnerBlock ==========

SpawnerBlock::SpawnerBlock(const BlockProperties& properties)
    : Block(properties) {
    // 刷怪笼没有特殊状态
}

ActionResultType SpawnerBlock::onBlockActivated(
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

    // 只有创造模式可以打开刷怪笼界面
    return ActionResultType::Pass;
}

// ========== DragonBreathBlock ==========

DragonBreathBlock::DragonBreathBlock(const BlockProperties& properties)
    : Block(properties) {
}

void DragonBreathBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(entity);
    // TODO: 造成伤害
}

const CollisionShape& DragonBreathBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

const CollisionShape& DragonBreathBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

} // namespace blocks
} // namespace mc
