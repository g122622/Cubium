#include "SweetBerryBushBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

SweetBerryBushBlock::SweetBerryBushBlock(const BlockProperties& properties)
    : BushBlock(properties) {

    // 创建状态容器，添加 AGE 属性
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(AGE())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(AGE(), 0));

    // 初始化形状
    initShapes();
}

int SweetBerryBushBlock::getAge(const BlockState& state) const {
    return static_cast<int>(state.get(AGE()));
}

const BlockState& SweetBerryBushBlock::withAge(const BlockState& state, int age) const {
    return state.with(AGE(), std::clamp(age, 0, getMaxAge()));
}

bool SweetBerryBushBlock::isMaxAge(const BlockState& state) const {
    return getAge(state) >= getMaxAge();
}

BlockState SweetBerryBushBlock::getStateForPlacement(BlockItemUseContext& context) {
    MC_UNUSED(context);
    return defaultState();
}

void SweetBerryBushBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    int age = getAge(state);
    if (age >= getMaxAge()) {
        return;
    }

    // 光照检查：需要光照 >= 9
    const BlockPos abovePos = pos.up();
    const i32 blockLight = static_cast<i32>(world.getBlockLight(abovePos));
    const i32 skyLight = static_cast<i32>(world.getSkyLight(abovePos));
    if (std::max(blockLight, skyLight) < 9) {
        return;
    }

    // 1/5 概率生长
    // 参考: net.minecraft.block.SweetBerryBushBlock#randomTick
    if (random.nextInt(5) == 0) {
        const BlockState& newState = withAge(state, age + 1);
        world.setBlockState(pos, &newState, 2);
    }
}

bool SweetBerryBushBlock::canGrow(
    IBlockReader& world,
    const BlockPos& pos,
    const BlockState& state,
    bool isClientSide) const {

    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(isClientSide);

    return !isMaxAge(state);
}

bool SweetBerryBushBlock::canUseBonemeal(
    IWorld& world,
    math::IRandom& random,
    const BlockPos& pos,
    const BlockState& state) const {

    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);

    return true;
}

void SweetBerryBushBlock::grow(
    IWorld& world,
    math::IRandom& random,
    const BlockPos& pos,
    const BlockState& state) {

    MC_UNUSED(random);

    int age = getAge(state);
    if (age < getMaxAge()) {
        const BlockState& newState = withAge(state, age + 1);
        world.setBlockState(pos, &newState, 2);
    }
}

const CollisionShape& SweetBerryBushBlock::getShape(const BlockState& state) const {
    int age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 3);
    return m_shapesByAge[age];
}

const CollisionShape& SweetBerryBushBlock::getCollisionShape(const BlockState& state) const {
    int age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 3);
    return m_collisionShapesByAge[age];
}

void SweetBerryBushBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    MC_UNUSED(pos);

    // 参考: net.minecraft.block.SweetBerryBushBlock#onEntityCollision
    // 只对 LivingEntity 生效（需要在 Entity 类中添加 isLiving() 方法）
    // TODO: 检查实体类型，狐狸和蜜蜂免疫伤害

    int age = getAge(state);

    // 减速效果
    // entity.setMotionMultiplier(state, Vector3d(0.8, 0.75, 0.8));
    // TODO: 实现 Entity::setMotionMultiplier

    // 伤害逻辑（只在服务端执行）
    // 只有 AGE > 0 时才造成伤害
    if (age > 0) {
        // TODO: 检查实体是否移动（lastTickPos != currentPos）
        // TODO: 检查移动距离 >= 0.003
        // TODO: 检查实体类型（狐狸和蜜蜂免疫）
        // entity.attackEntityFrom(DamageSource::SWEET_BERRY_BUSH, 1.0F);
        MC_UNUSED(world);
        MC_UNUSED(entity);
    }
}

ActionResultType SweetBerryBushBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    int age = getAge(state);
    bool fullyGrown = (age == 3);

    // AGE > 1 时可以采摘
    if (age > 1) {
        // 计算掉落数量
        // AGE 2: 1-2 个浆果
        // AGE 3: 2-3 个浆果
        // 参考: int j = 1 + worldIn.rand.nextInt(2);
        // spawnAsEntity(..., new ItemStack(Items.SWEET_BERRIES, j + (flag ? 1 : 0)));

        // TODO: 生成 ItemEntity 掉落浆果
        MC_UNUSED(fullyGrown);

        // AGE 重置为 1
        const BlockState& newState = withAge(state, 1);
        world.setBlockState(pos, &newState, 2);

        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

bool SweetBerryBushBlock::canSustain(
    const BlockState& groundState,
    IWorld& world,
    const BlockPos& groundPos) const {

    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    // 甜浆果丛可以种在草地、泥土、砂土、灰化土、耕地上
    // 参考: net.minecraft.block.SweetBerryBushBlock#isValidGround
    const Block* block = &groundState.owner();

    // TODO: 使用 BlockTags 检查
    // return state.isIn(BlockTags.VALID_SWEET_BERRY_BUSH_GROUND);

    // 简化实现：检查具体方块
    // GRASS_BLOCK, DIRT, COARSE_DIRT, PODZOL, FARMLAND
    MC_UNUSED(block);
    return true; // 临时：总是返回 true
}

void SweetBerryBushBlock::initShapes() {
    // 参考 MC 1.16.5 SweetBerryBushBlock 的形状定义
    constexpr f32 P = 1.0f / 16.0f;

    // AGE 0: 幼苗形状 (3, 0, 3) -> (13, 8, 13)
    m_shapesByAge[0] = CollisionShape::box(
        3.0f * P, 0.0f, 3.0f * P,
        13.0f * P, 8.0f * P, 13.0f * P);

    // AGE 1-3: 完整灌木形状 (1, 0, 1) -> (15, 16, 15)
    CollisionShape fullShape = CollisionShape::box(
        1.0f * P, 0.0f, 1.0f * P,
        15.0f * P, 16.0f * P, 15.0f * P);

    m_shapesByAge[1] = fullShape;
    m_shapesByAge[2] = fullShape;
    m_shapesByAge[3] = fullShape;

    // 碰撞形状
    // AGE 0: 无碰撞
    m_collisionShapesByAge[0] = CollisionShape::empty();

    // AGE 1-3: 有碰撞
    m_collisionShapesByAge[1] = fullShape;
    m_collisionShapesByAge[2] = fullShape;
    m_collisionShapesByAge[3] = fullShape;
}

} // namespace blocks
} // namespace mc
