#include "MobBlocks.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../entity/entities/monster/arthropod/EndermiteEntity.hpp"
#include "../../../../entity/entities/passive/special/TurtleEntity.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../core/Types.hpp"

namespace mc {
namespace blocks {

// ========== BeehiveBlock ==========

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

        // MC 1.16.5: 为每个蛋生成一只小海龟
        // 参考: TurtleEggBlock.randomTick
        // for(int j = 0; j < state.get(EGGS); ++j) {
        //     TurtleEntity turtleentity = EntityType.TURTLE.create(worldIn);
        //     turtleentity.setGrowingAge(-24000);
        //     turtleentity.setHome(pos);
        //     turtleentity.setLocationAndAngles(
        //         (double)pos.getX() + 0.3D + (double)j * 0.2D,
        //         (double)pos.getY(),
        //         (double)pos.getZ() + 0.3D,
        //         0.0F, 0.0F);
        //     worldIn.addEntity(turtleentity);
        // }
        for (i32 i = 0; i < eggs; ++i) {
            auto turtle = std::make_unique<TurtleEntity>(LegacyEntityType::Turtle, EntityId(0));
            if (turtle) {
                // MC 1.16.5: 设置为幼体（-24000 ticks = 20分钟）
                turtle->setChild(true);

                // MC 1.16.5: 设置出生位置（小海龟会记住这个位置作为"家"）
                turtle->setHomePos(pos);

                // 设置位置：多个蛋时错开位置
                // x = pos.x + 0.3 + i * 0.2
                // z = pos.z + 0.3
                turtle->setPosition(
                    static_cast<f32>(pos.x) + 0.3f + static_cast<f32>(i) * 0.2f,
                    static_cast<f32>(pos.y),
                    static_cast<f32>(pos.z) + 0.3f
                );
                turtle->setRotation(0.0f, 0.0f);

                // 生成到世界
                world.spawnEntity(std::move(turtle));
            }
        }
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

bool TurtleEggBlock::canTrample(IWorld& /*world*/, Entity& entity) const {
    // MC 1.16.5: 只有玩家或满足 mobGriefing 的生物才能踩破蛋
    // 海龟和蝙蝠不能踩破蛋

    // MC 1.16.5 TurtleEggBlock.canTrample():
    // if (!(trampler instanceof TurtleEntity) && !(trampler instanceof BatEntity)) {
    //     if (!(trampler instanceof LivingEntity)) {
    //         return false;
    //     } else {
    //         return trampler instanceof PlayerEntity || net.minecraftforge.event.ForgeEventFactory.getMobGriefingEvent(worldIn, trampler);
    //     }
    // } else {
    //     return false;
    // }

    // 获取实体类型
    LegacyEntityType type = entity.legacyType();

    // 海龟和蝙蝠不能踩破蛋
    if (type == LegacyEntityType::Turtle || type == LegacyEntityType::Bat) {
        return false;
    }

    // 非生物实体不能踩破（物品、箭矢等）
    // 检查是否为生物实体：玩家和怪物类实体可以踩破
    // LegacyEntityType 中 Player = 1, 被动生物 10-49, 敌对生物 50-150
    // 使用动态类型检查判断是否为 LivingEntity
    auto* living = dynamic_cast<LivingEntity*>(&entity);
    if (living == nullptr) {
        return false;
    }

    // 玩家总是可以踩破
    if (type == LegacyEntityType::Player) {
        return true;
    }

    // TODO: 检查 mobGriefing 游戏规则
    // return world.getGameRules().getBoolean(GameRule::MOB_GRIEFING);
    // 暂时返回 true（其他生物可以踩破）
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
    // 使用 instanceof ZombieEntity 检查，由于 ZombieEntity 是基类，
    // HuskEntity、DrownedEntity 等是子类
    // 但在当前项目中，这些是独立的实体类型，需要通过 LegacyEntityType 检查

    LegacyEntityType type = entity.legacyType();

    // MC 1.16.5: 只有 ZombieEntity 及其子类（Husk、Drowned）会踩破海龟蛋
    // 注意：MC 中 ZombieVillager 也是 ZombieEntity 的子类，但当前项目中未定义
    // Skeleton、Stray、WitherSkeleton 虽然是亡灵，但不是僵尸类，不会踩破蛋
    switch (type) {
        case LegacyEntityType::Zombie:
        case LegacyEntityType::Husk:
        case LegacyEntityType::Drowned:
            // 僵尸及其变种会踩破海龟蛋
            return true;
        default:
            return false;
    }
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
    // MC 1.16.5: InfestedBlock.spawnAdditionalDrops()
    // 当被破坏时，有概率生成蠹虫
    // 注意：实际生成条件需要检查游戏规则 doTileDrops 和精准采集附魔
    // 这些检查在 onBlockHarvested 或 spawnAdditionalDrops 中进行
    // 这里简化处理：直接生成蠹虫

    MC_UNUSED(state);

    // 只在服务端生成
    if (world.isClientSide()) {
        return;
    }

    // MC 1.16.5: 创建蠹虫实体
    // SilverfishEntity silverfishentity = EntityType.SILVERFISH.create(world);
    // silverfishentity.setLocationAndAngles(pos.getX() + 0.5D, pos.getY(), pos.getZ() + 0.5D, 0.0F, 0.0F);
    // world.addEntity(silverfishentity);
    // silverfishentity.spawnExplosionParticle();

    auto silverfish = std::make_unique<SilverfishEntity>(LegacyEntityType::Silverfish, EntityId(0));
    if (silverfish) {
        // 设置位置（方块中心）
        silverfish->setPosition(
            static_cast<f32>(pos.x) + 0.5f,
            static_cast<f32>(pos.y),
            static_cast<f32>(pos.z) + 0.5f
        );
        silverfish->setRotation(0.0f, 0.0f);

        // 生成到世界
        world.spawnEntity(std::move(silverfish));
    }
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
