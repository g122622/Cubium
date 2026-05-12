#include "SpecialBlocks.hpp"
#include "../../../IWorld.hpp"
#include "../../../WorldEvents.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../block/IBucketPickupHandler.hpp"
#include "../../../block/blocks/LiquidBlock.hpp"
#include "../../../fluid/FluidTags.hpp"
#include "../../../dimension/DimensionType.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/Vector3.hpp"
#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../physics/PhysicsConstants.hpp"
#include <queue>
#include <utility>
#include <unordered_set>

namespace mc {
namespace blocks {

// BlockPos 哈希别名
using BlockPosHash = std::hash<BlockPos>;

// ========== BarrierBlock ==========

BarrierBlock::BarrierBlock(const BlockProperties& properties)
    : Block(properties) {
    // 屏障没有状态属性
}

const CollisionShape& BarrierBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape fullShape = CollisionShape::fullBlock();
    return fullShape;
}

// ========== StructureVoidBlock ==========

StructureVoidBlock::StructureVoidBlock(const BlockProperties& properties)
    : Block(properties) {
    // 结构空位没有状态属性
}

const CollisionShape& StructureVoidBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    // 结构空位有一个小的可见轮廓
    static CollisionShape shape = CollisionShape::box(0.375f, 0.375f, 0.375f, 0.625f, 0.625f, 0.625f);
    return shape;
}

const CollisionShape& StructureVoidBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== StructureBlock ==========

StructureBlock::StructureBlock(const BlockProperties& properties)
    : Block(properties) {
    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));
}

StructureBlock::Mode StructureBlock::getMode(const BlockState& state) const {
    MC_UNUSED(state);
    // TODO: 实现 MODE 属性
    return Mode::Save;
}

ActionResultType StructureBlock::onBlockActivated(
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

    // TODO: 打开结构方块界面
    return ActionResultType::Success;
}

// ========== JigsawBlock ==========

JigsawBlock::JigsawBlock(const BlockProperties& properties)
    : Block(properties) {
    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));
}

ActionResultType JigsawBlock::onBlockActivated(
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

    // TODO: 打开拼图方块界面
    return ActionResultType::Success;
}

// ========== CommandBlock ==========

CommandBlock::CommandBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::FACING())
        .add(BlockStateProperties::CONDITIONAL())
        .add(BlockStateProperties::POWERED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::FACING(), Direction::North)
        .with(BlockStateProperties::CONDITIONAL(), false)
        .with(BlockStateProperties::POWERED(), false));
}

Direction CommandBlock::getFacing(const BlockState& state) const {
    return state.get(BlockStateProperties::FACING());
}

bool CommandBlock::isConditional(const BlockState& state) const {
    return state.get(BlockStateProperties::CONDITIONAL());
}

bool CommandBlock::isPowered(const BlockState& state) const {
    return state.get(BlockStateProperties::POWERED());
}

BlockState CommandBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction facing = Directions::opposite(context.getClickedFace());
    return defaultState().with(BlockStateProperties::FACING(), facing);
}

void CommandBlock::neighborChanged(
    IWorld& world,
    const BlockPos& pos,
    Block& neighborBlock,
    const BlockPos& neighborPos,
    bool isMoving) {

    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // TODO: 检测红石信号并触发命令
}

i32 CommandBlock::getWeakPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 命令方块不输出信号
    return 0;
}

const BlockState& CommandBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::FACING(), newFacing);
}

const BlockState& CommandBlock::mirror(const BlockState& state, Mirror mirror) const {
    Direction facing = state.get(BlockStateProperties::FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::FACING(), newFacing);
}

ActionResultType CommandBlock::onBlockActivated(
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

    // TODO: 打开命令方块界面
    return ActionResultType::Success;
}

void CommandBlock::execute(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // TODO: 执行命令
}

// ========== RepeatingCommandBlock ==========

RepeatingCommandBlock::RepeatingCommandBlock(const BlockProperties& properties)
    : CommandBlock(properties) {
}

void RepeatingCommandBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    MC_UNUSED(random);
    // 每个 tick 执行命令
    execute(world, pos, state);
}

// ========== ChainCommandBlock ==========

ChainCommandBlock::ChainCommandBlock(const BlockProperties& properties)
    : CommandBlock(properties) {
}

// ========== SlimeBlock ==========

SlimeBlock::SlimeBlock(const BlockProperties& properties)
    : Block(properties) {
    // 史莱姆块滑度为 0.8
    m_slipperiness = physics::SLIPPERINESS_SLIME;
}

void SlimeBlock::onLanded(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const {
    // MC 1.16.5: SlimeBlock.onLanded
    // 如果实体向下落，反弹
    // 反弹系数：LivingEntity 使用 1.0，其他实体使用 0.8
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    Vector3 velocity = entity.velocity();
    if (velocity.y < 0.0f) {
        // 反弹：Y速度取反并乘以弹跳系数
        // MC 1.16.5: this.setMotion(vec3d.x, -vec3d.y * 0.9D, vec3d.z);
        // 使用非生物实体的弹跳系数（保守值），LivingEntity 会单独处理
        entity.setVelocity(velocity.x, -velocity.y * physics::SLIME_BLOCK_BOUNCE_FACTOR_NON_LIVING, velocity.z);
    } else {
        // 向上或静止时，Y速度归零
        entity.setVelocity(velocity.x, 0.0f, velocity.z);
    }
}

void SlimeBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    // MC 1.16.5: SlimeBlock.onEntityCollision
    // 史莱姆块会减缓实体的Y轴速度（类似于蜘蛛网的效果，但更温和）
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 这个效果主要用于实体在史莱姆块内部时减速
    // 实际弹跳在 onLanded 中处理
}

Material::PushReaction SlimeBlock::getPushReaction(const BlockState& state) const {
    MC_UNUSED(state);
    return Material::PushReaction::Normal;
}

bool SlimeBlock::isStickyBlock(const BlockState& state) const {
    MC_UNUSED(state);
    // MC 1.16.5: SlimeBlock.isStickyBlock returns true
    return true;
}

bool SlimeBlock::canStickTo(const BlockState& state, const BlockState& other) const {
    MC_UNUSED(state);
    // MC 1.16.5: SlimeBlock.canStickTo
    // 黏液块可以粘住黏液块和蜂蜜块
    const Block& otherBlock = other.getBlock();
    return otherBlock.isStickyBlock(other);
}

// ========== HoneyBlock ==========

HoneyBlock::HoneyBlock(const BlockProperties& properties)
    : Block(properties) {
    // 蜂蜜块滑度为 0.98 (MC 1.16.5: Blocks.java:445)
    m_slipperiness = 0.98f;
    // 蜂蜜块跳跃因子为 0.5
    m_jumpFactor = physics::HONEY_BLOCK_JUMP_FACTOR;

    // 蜂蜜块碰撞箱稍小
    m_collisionShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.9375f, 1.0f);
}

void HoneyBlock::onLanded(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const {
    // MC 1.16.5: HoneyBlock.onLanded
    // 蜂蜜块消除摔落伤害，但不弹跳
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    Vector3 velocity = entity.velocity();
    // Y速度归零，但不反弹
    entity.setVelocity(velocity.x, 0.0f, velocity.z);
    // 重置摔落距离（消除摔落伤害）
    entity.setFallDistance(0.0f);
}

void HoneyBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    // MC 1.16.5: HoneyBlock.onEntityCollision
    // 蜂蜜块减缓实体速度
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    Vector3 velocity = entity.velocity();
    // 水平速度减少 40%（乘以 0.4）
    // 垂直下落速度减少（下落时每 tick 减速）
    // MC 1.16.5: entity.setMotion(entity.getMotion().mul(0.4D, 0.9D, 0.4D));
    entity.setVelocity(
        velocity.x * 0.4f,
        velocity.y * 0.9f,
        velocity.z * 0.4f
    );
}

Material::PushReaction HoneyBlock::getPushReaction(const BlockState& state) const {
    MC_UNUSED(state);
    return Material::PushReaction::Normal;
}

bool HoneyBlock::isStickyBlock(const BlockState& state) const {
    MC_UNUSED(state);
    // MC 1.16.5: HoneyBlock.isStickyBlock returns true
    return true;
}

bool HoneyBlock::canStickTo(const BlockState& state, const BlockState& other) const {
    MC_UNUSED(state);
    // MC 1.16.5: HoneyBlock.canStickTo
    // 蜂蜜块只能粘住蜂蜜块（不能粘住黏液块）
    // 参考: AbstractBlock.AbstractBlockState.canStickTo
    // 如果两个都是蜂蜜块，则可以粘连
    // 检查 other 方块是否是蜂蜜块
    return other.is(VanillaBlocks::HONEY_BLOCK);
}

const CollisionShape& HoneyBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_collisionShape;
}

// ========== SpongeBlock ==========

SpongeBlock::SpongeBlock(const BlockProperties& properties)
    : Block(properties) {
}

bool SpongeBlock::tryAbsorbWater(IWorld& world, const BlockPos& pos) {
    i32 absorbedCount = absorb(world, pos);
    if (absorbedCount > 0) {
        // 将海绵变为湿润海绵
        const BlockState& wetSpongeState = VanillaBlocks::WET_SPONGE->defaultState();
        world.setBlockState(pos, &wetSpongeState, 3);

        // 播放水被吸收的视觉效果（事件 2001，data 为水的方块状态 ID）
        // MC 1.16.5: world.playEvent(2001, pos, Block.getStateId(Blocks.WATER.getDefaultState()));
        const BlockState& waterState = VanillaBlocks::WATER->defaultState();
        world.playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, pos, waterState.stateId());

        return true;
    }
    return false;
}

void SpongeBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(state);
    // MC 1.16.5: 放置时尝试吸水
    tryAbsorbWater(world, pos);
}

void SpongeBlock::neighborChanged(
    IWorld& world,
    const BlockPos& pos,
    Block& neighborBlock,
    const BlockPos& neighborPos,
    bool isMoving) {

    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // MC 1.16.5: 邻居更新时尝试吸水
    tryAbsorbWater(world, pos);

    // 调用基类方法
    Block::neighborChanged(world, pos, neighborBlock, neighborPos, isMoving);
}

i32 SpongeBlock::absorb(IWorld& world, const BlockPos& pos) {
    // MC 1.16.5: SpongeBlock.absorb()
    // 使用 BFS 搜索周围的水方块

    // 队列元素：(位置, 深度)
    std::queue<std::pair<BlockPos, i32>> queue;
    queue.push({pos, 0});

    i32 absorbedCount = 0;

    // 已访问的位置集合（用于避免重复处理）
    std::unordered_set<BlockPos, BlockPosHash> visited;
    visited.insert(pos);

    while (!queue.empty()) {
        auto [currentPos, depth] = queue.front();
        queue.pop();

        // 遍历六个方向
        for (Direction dir : Directions::all()) {
            BlockPos neighborPos = currentPos.offset(dir);

            // 获取流体状态
            const fluid::FluidState* fluidState = world.getFluidState(neighborPos);
            if (fluidState == nullptr || fluidState->isEmpty()) {
                continue;
            }

            // 检查是否为水
            if (!fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
                continue;
            }

            // 获取方块状态
            const BlockState* blockState = world.getBlockState(neighborPos);
            if (blockState == nullptr) {
                continue;
            }

            Block& block = const_cast<Block&>(blockState->getBlock());

            // 情况1：可舀取的水源（如水源方块）
            // MC 1.16.5: if (blockstate.getBlock() instanceof IBucketPickupHandler)
            IBucketPickupHandler* bucketPickup = dynamic_cast<IBucketPickupHandler*>(&block);
            if (bucketPickup != nullptr) {
                fluid::Fluid* pickedFluid = bucketPickup->pickupFluid(world, neighborPos, *blockState);
                if (pickedFluid != nullptr) {
                    ++absorbedCount;
                    if (depth < MAX_ABSORB_DEPTH && visited.find(neighborPos) == visited.end()) {
                        visited.insert(neighborPos);
                        queue.push({neighborPos, depth + 1});
                    }
                }
            }
            // 情况2：流动水方块
            // MC 1.16.5: else if (blockstate.getBlock() instanceof FlowingFluidBlock)
            else if (dynamic_cast<block::LiquidBlock*>(&block) != nullptr) {
                // 移除流动水方块，设置为空气
                const BlockState& airState = VanillaBlocks::AIR->defaultState();
                world.setBlockState(neighborPos, &airState, 3);
                ++absorbedCount;
                if (depth < MAX_ABSORB_DEPTH && visited.find(neighborPos) == visited.end()) {
                    visited.insert(neighborPos);
                    queue.push({neighborPos, depth + 1});
                }
            }
            // 情况3：海洋植物/海草
            // MC 1.16.5: else if (material == Material.OCEAN_PLANT || material == Material.SEA_GRASS)
            else {
                const Material& material = blockState->getMaterial();
                if (material == Material::OCEAN_PLANT || material == Material::SEA_GRASS) {
                    // 掉落物品后移除方块
                    // MC 1.16.5: spawnDrops(blockstate, worldIn, blockpos1, tileentity);
                    // TODO: 实现方块掉落（需要 Block::spawnDrops 方法）
                    // 目前直接移除方块
                    const BlockState& airState = VanillaBlocks::AIR->defaultState();
                    world.setBlockState(neighborPos, &airState, 3);
                    ++absorbedCount;
                    if (depth < MAX_ABSORB_DEPTH && visited.find(neighborPos) == visited.end()) {
                        visited.insert(neighborPos);
                        queue.push({neighborPos, depth + 1});
                    }
                }
            }

            // 超过最大吸收数量就停止
            if (absorbedCount >= MAX_ABSORB_COUNT) {
                return absorbedCount;
            }
        }
    }

    return absorbedCount;
}

// ========== WetSpongeBlock ==========

WetSpongeBlock::WetSpongeBlock(const BlockProperties& properties)
    : Block(properties) {
}

void WetSpongeBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(state);

    // MC 1.16.5: WetSpongeBlock.onBlockAdded
    // 在下界（超热维度）放置时变干
    if (world.isUltraWarm()) {
        // 变为干海绵
        const BlockState& spongeState = VanillaBlocks::SPONGE->defaultState();
        world.setBlockState(pos, &spongeState, 3);

        // 播放蒸汽效果（事件 2009）
        world.playEvent(world::WorldEvents::WET_SPONGE_DRY, pos, 0);

        // 播放火焰熄灭音效
        world.playSound(
            ResourceLocation("minecraft", "block.fire.extinguish"),
            sound::SoundCategory::Blocks,
            pos.center(),
            1.0f,
            1.0f
        );
    }
}

// ========== WebBlock ==========

WebBlock::WebBlock(const BlockProperties& properties)
    : Block(properties) {
    // 蜘蛛网形状：完整方块，透明
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

const CollisionShape& WebBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

void WebBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    // MC 1.16.5: WebBlock.onEntityCollision
    // 蜘蛛网大幅减缓实体速度
    // MC 源码: entity.setMotion(entity.getMotion().mul(0.25D, 0.05000000074505806D, 0.25D));
    // 但实际上减速更慢，因为实体每tick都会被再次减速
    // 最终效果是水平速度 * 0.025，垂直下落 * 0.05
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    Vector3 velocity = entity.velocity();
    entity.setVelocity(
        velocity.x * physics::COBWEB_SLOWDOWN_XZ,
        velocity.y < 0.0f ? velocity.y * physics::COBWEB_SLOWDOWN_Y : velocity.y,  // 只减速下落
        velocity.z * physics::COBWEB_SLOWDOWN_XZ
    );
}

} // namespace blocks
} // namespace mc
