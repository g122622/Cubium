#include "MobBlocks.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../sound/SoundCategory.hpp"

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

    // 创建各蛋数量的形状 (MC 1.16.5: box(3, 0, 3, 12, 7, 12) for 1 egg, box(1, 0, 1, 15, 7, 15) for 4)
    // 1个蛋: 3/16=0.1875, 12/16=0.75, 7/16=0.4375
    // MC实际形状: 1 egg: (3, 0, 3, 12, 7, 12), 2 eggs: (1, 0, 3, 15, 7, 12), etc
    m_shapesByEggCount[0] = CollisionShape::box(0.1875f, 0.0f, 0.1875f, 0.75f, 0.4375f, 0.75f);   // 1 egg
    m_shapesByEggCount[1] = CollisionShape::box(0.0625f, 0.0f, 0.1875f, 0.9375f, 0.4375f, 0.75f); // 2 eggs
    m_shapesByEggCount[2] = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.4375f, 0.9375f); // 3 eggs
    m_shapesByEggCount[3] = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.4375f, 0.9375f); // 4 eggs
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
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // MC 1.16.5: 如果放置在已有的海龟蛋上，增加蛋数量
    const BlockState* existingState = world.getBlockState(pos);
    if (existingState != nullptr && existingState->is(this)) {
        i32 currentEggs = existingState->get(BlockStateProperties::EGGS_1_4());
        if (currentEggs < 4) {
            return existingState->with(BlockStateProperties::EGGS_1_4(), currentEggs + 1);
        }
        return *existingState;
    }

    return defaultState();
}

bool TurtleEggBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // MC 1.16.5: 海龟蛋只能放在沙子类方块上
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 检查是否为沙子类方块 (沙子、红沙、灵魂沙)
    return BlockTags::SAND().contains(*belowState);
}

bool TurtleEggBlock::canGrow(IWorld& world, math::IRandom& random) const {
    // MC 1.16.5: 在日光下有更高的孵化概率
    // 白天 (skyLight >= 0.65) 或 1/500 随机概率
    // TODO: 实现天空光照检查
    // 暂时使用简化的随机概率
    return random.nextFloat() < 0.002f || random.nextInt(500) == 0;
}

void TurtleEggBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // MC 1.16.5: 孵化逻辑
    // 检查是否在沙子上
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    if (!hasProperHabitat(blockReader, pos)) {
        return;
    }

    // 检查孵化条件
    if (!canGrow(world, random)) {
        return;
    }

    i32 hatch = getHatch(state);
    if (hatch < 2) {
        // 孵化进度增加
        // MC 1.16.5: 播放裂开音效
        if (!world.isClientSide()) {
            world.playSound(
                SoundEvents::ENTITY_TURTLE_EGG_CRACK,
                sound::SoundCategory::Blocks,
                pos.center(),
                0.7f,
                0.9f + random.nextFloat() * 0.2f
            );
        }
        const BlockState& newState = state.with(BlockStateProperties::HATCH_0_2(), hatch + 1);
        world.setBlockState(pos, &newState, 2);
    } else {
        // 孵化完成，生成海龟
        // MC 1.16.5: 播放孵化音效
        if (!world.isClientSide()) {
            world.playSound(
                SoundEvents::ENTITY_TURTLE_EGG_HATCH,
                sound::SoundCategory::Blocks,
                pos.center(),
                0.7f,
                0.9f + random.nextFloat() * 0.2f
            );
        }
        i32 eggs = getEggs(state);

        // 移除方块
        const BlockState* airState = BlockRegistry::instance().airState();
        if (airState != nullptr) {
            world.setBlockState(pos, airState, 2);
        }

        // TODO: 为每个蛋生成一只小海龟
        // for (i32 i = 0; i < eggs; ++i) {
        //     spawnBabyTurtle(world, pos);
        // }
        MC_UNUSED(eggs);
    }
}

void TurtleEggBlock::onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const {
    // MC 1.16.5: 实体走过时尝试踩破蛋
    tryTrample(world, pos, state, entity, 100);
}

void TurtleEggBlock::onFallenUpon(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    Entity& entity,
    f32 fallDistance) {

    // MC 1.16.5: 实体摔落时尝试踩破蛋
    // 僵尸类生物不会踩破蛋（它们会直接走过去）
    MC_UNUSED(fallDistance);

    // 检查是否为僵尸类（僵尸、尸壳、溺尸等）
    // 僵尸类不会踩破蛋
    if (isZombieType(entity)) {
        return;
    }

    tryTrample(world, pos, state, entity, 3);
}

bool TurtleEggBlock::hasProperHabitat(IBlockReader& world, const BlockPos& pos) const {
    // MC 1.16.5: 检查下方是否为沙子
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    return belowState != nullptr && BlockTags::SAND().contains(*belowState);
}

bool TurtleEggBlock::canTrample(IWorld& world, Entity& entity) const {
    // MC 1.16.5: 只有玩家或满足 mobGriefing 的生物才能踩破蛋
    // 海龟和蝙蝠不能踩破蛋

    // 检查是否为海龟
    // TODO: 实现实体类型检查
    // if (entity.getType() == EntityTypes::TURTLE) return false;
    // if (entity.getType() == EntityTypes::BAT) return false;

    // TODO: 检查 mobGriefing 游戏规则
    // 暂时只允许玩家踩破
    return true;
}

void TurtleEggBlock::tryTrample(IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, i32 chance) const {
    // MC 1.16.5: 尝试踩破蛋
    if (!canTrample(world, entity)) {
        return;
    }

    // 随机检查
    if (world.getRandom().nextInt(chance) != 0) {
        return;
    }

    // 踩破一个蛋
    removeOneEgg(world, pos, state);
}

void TurtleEggBlock::removeOneEgg(IWorld& world, const BlockPos& pos, const BlockState& state) const {
    // MC 1.16.5: 移除一个蛋
    // 播放破碎音效
    if (!world.isClientSide()) {
        world.playSound(
            SoundEvents::ENTITY_TURTLE_EGG_BREAK,
            sound::SoundCategory::Blocks,
            pos.center(),
            0.7f,
            0.9f + world.getRandom().nextFloat() * 0.2f
        );
    }

    i32 eggs = getEggs(state);
    if (eggs > 1) {
        // 减少蛋数量，重置孵化进度
        const BlockState& newState = state
            .with(BlockStateProperties::EGGS_1_4(), eggs - 1)
            .with(BlockStateProperties::HATCH_0_2(), 0);
        world.setBlockState(pos, &newState, 2);
    } else {
        // 移除方块
        const BlockState* airState = BlockRegistry::instance().airState();
        if (airState != nullptr) {
            world.setBlockState(pos, airState, 2);
        }
    }
}

bool TurtleEggBlock::isZombieType(Entity& entity) const {
    // MC 1.16.5: 检查实体是否为僵尸类
    // 包括: 僵尸、尸壳、溺尸、僵尸村民、僵尸马
    // TODO: 实现实体类型检查
    MC_UNUSED(entity);
    return false;
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
