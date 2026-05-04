#include "FireBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../dimension/teleport/PortalSize.hpp"
#include "../../VanillaBlocks.hpp"

namespace mc {
namespace blocks {

// ========== FireBlock ==========

FireBlock::FireBlock(const BlockProperties& properties, i32 fireDamage)
    : Block(properties)
    , m_fireDamage(fireDamage) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::AGE_0_15())
        .add(BlockStateProperties::NORTH())
        .add(BlockStateProperties::SOUTH())
        .add(BlockStateProperties::EAST())
        .add(BlockStateProperties::WEST())
        .add(BlockStateProperties::UP())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::AGE_0_15(), 0)
        .with(BlockStateProperties::NORTH(), false)
        .with(BlockStateProperties::SOUTH(), false)
        .with(BlockStateProperties::EAST(), false)
        .with(BlockStateProperties::WEST(), false)
        .with(BlockStateProperties::UP(), false));

    // 火焰形状（无碰撞）
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

i32 FireBlock::getAge(const BlockState& state) const {
    return state.get(BlockStateProperties::AGE_0_15());
}

BlockState FireBlock::withAge(i32 age) const {
    return defaultState().with(BlockStateProperties::AGE_0_15(), std::min(age, 15));
}

BlockState FireBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

bool FireBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 检查周围是否有可支撑的方块
    for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West, Direction::Up, Direction::Down}) {
        BlockPos adjPos = pos.offset(dir);
        const BlockState* adjState = world.getBlockState(adjPos);

        if (adjState != nullptr && adjState->isSolid()) {
            return true;
        }
    }

    return canBurn(world, pos);
}

BlockState FireBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facingPos);

    // 检查是否仍然有支撑
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    if (!isValidPosition(state, blockReader, currentPos)) {
        if (auto* airState = BlockRegistry::instance().airState()) {
            return *airState;
        }
    }

    // 更新连接状态
    bool connected = false;
    switch (facing) {
        case Direction::North:
            connected = facingState.isSolid();
            return state.with(BlockStateProperties::NORTH(), connected);
        case Direction::South:
            connected = facingState.isSolid();
            return state.with(BlockStateProperties::SOUTH(), connected);
        case Direction::East:
            connected = facingState.isSolid();
            return state.with(BlockStateProperties::EAST(), connected);
        case Direction::West:
            connected = facingState.isSolid();
            return state.with(BlockStateProperties::WEST(), connected);
        case Direction::Up:
            connected = facingState.isSolid();
            return state.with(BlockStateProperties::UP(), connected);
        default:
            return state;
    }
}

void FireBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    MC_UNUSED(random);
    // 更新火焰状态
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    if (!isValidPosition(state, blockReader, pos)) {
        world.setBlockState(pos, nullptr, 3);
    }

    // 尝试点燃下界传送门
    // 参考 MC 1.16.5 FireBlock.tick
    tryLightNetherPortal(world, pos);
}

void FireBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    i32 age = getAge(state);

    // 火焰可能熄灭
    if (age < 15 && random.nextInt(3) == 0) {
        world.setBlockState(pos, &withAge(age + 1), 2);
    }

    // 尝试蔓延
    trySpread(world, pos, age, random);
}

void FireBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 对实体造成伤害
    // entity.hurt(DamageSource::IN_FIRE, m_fireDamage);
}

const CollisionShape& FireBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& FireBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

bool FireBlock::canBurn(IBlockReader& world, const BlockPos& pos) const {
    // TODO: 检查是否有可燃方块
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return false;
}

void FireBlock::trySpread(IWorld& world, const BlockPos& pos, i32 age, math::IRandom& random) {
    // TODO: 实现火焰蔓延逻辑
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(age);
    MC_UNUSED(random);
}

void FireBlock::tryLightNetherPortal(IWorld& world, const BlockPos& pos) {
    // 参考 MC 1.16.5 FireBlock
    // 检查火焰周围是否形成有效的下界传送门框架

    if (VanillaBlocks::OBSIDIAN == nullptr) {
        return;
    }

    // 遍历火焰周围的每个位置，尝试检测传送门
    for (Direction dir : Directions::horizontal()) {
        BlockPos adjacentPos = pos.offset(dir);

        // 尝试从相邻位置检测传送门框架
        auto portalResult = PortalSize::findNetherPortal(world, adjacentPos);

        if (portalResult.has_value() && portalResult->valid) {
            // 找到有效的传送门框架，点燃传送门
            PortalSize::lightNetherPortal(world, portalResult.value());
            return;
        }
    }
}

bool FireBlock::isFlammable(const BlockState& state) const {
    return state.getMaterial().isFlammable();
}

// ========== SoulFireBlock ==========

SoulFireBlock::SoulFireBlock(const BlockProperties& properties)
    : FireBlock(properties, 2) {  // 灵魂火伤害更高
}

bool SoulFireBlock::canBurn(IBlockReader& world, const BlockPos& pos) const {
    // TODO: 灵魂火只在灵魂土/灵魂沙上燃烧
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return false;
}

// ========== NetherPortalBlock ==========

NetherPortalBlock::NetherPortalBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_AXIS())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), Axis::X));

    // 创建形状
    m_xAxisShape = CollisionShape::box(0.0f, 0.0f, 0.375f, 1.0f, 1.0f, 0.625f);
    m_zAxisShape = CollisionShape::box(0.375f, 0.0f, 0.0f, 0.625f, 1.0f, 1.0f);
}

Axis NetherPortalBlock::getAxis(const BlockState& state) const {
    return state.get(BlockStateProperties::HORIZONTAL_AXIS());
}

BlockState NetherPortalBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction facing = context.horizontalDirection();
    Axis axis = Directions::getAxis(facing);
    if (axis == Axis::Y) axis = Axis::X;  // 水平轴
    return defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), axis);
}

bool NetherPortalBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // TODO: 检查传送门框架
    return true;
}

BlockState NetherPortalBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查传送门是否仍然有效
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    if (!isValidPosition(state, blockReader, currentPos)) {
        if (auto* airState = BlockRegistry::instance().airState()) {
            return *airState;
        }
    }

    return state;
}

void NetherPortalBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    // 参考 MC 1.16.5 NetherPortalBlock.onEntityCollision
    // 实体进入传送门后开始传送计时
    // 玩家需要站立在传送门中约 4 秒（80 ticks）才能传送
    // 其他实体约 1 tick

    // MC: if (!entity.isPassenger() && !entity.isBeingRidden() && entity.canTeleport()) {
    // 检查实体是否是乘客或被骑乘
    if (entity.isRiding() || entity.hasPassengers()) {
        return;
    }

    // 检查传送冷却
    if (!entity.canTeleport()) {
        return;
    }

    // 设置实体在传送门中
    entity.setInPortal(true);
    // 记录传送门位置
    entity.setPortalPos(pos);

    // 注意：传送逻辑由 Entity::tickPortal() 处理
    // 玩家的 getMaxInPortalTime() 返回 80 ticks
    // 其他实体的 getMaxInPortalTime() 返回 1 tick
}

const CollisionShape& NetherPortalBlock::getShape(const BlockState& state) const {
    Axis axis = getAxis(state);
    return (axis == Axis::X) ? m_xAxisShape : m_zAxisShape;
}

const CollisionShape& NetherPortalBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== NetherWartBlock ==========

NetherWartBlock::NetherWartBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::AGE_0_3())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::AGE_0_3(), 0));

    // 创建各年龄的形状
    m_shapesByAge[0] = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 0.25f, 0.75f);
    m_shapesByAge[1] = CollisionShape::box(0.1875f, 0.0f, 0.1875f, 0.8125f, 0.375f, 0.8125f);
    m_shapesByAge[2] = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 0.5f, 0.875f);
    m_shapesByAge[3] = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.625f, 0.9375f);
}

i32 NetherWartBlock::getAge(const BlockState& state) const {
    return state.get(BlockStateProperties::AGE_0_3());
}

BlockState NetherWartBlock::withAge(i32 age) const {
    return defaultState().with(BlockStateProperties::AGE_0_3(), std::min(age, 3));
}

BlockState NetherWartBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

bool NetherWartBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 检查下方是否为灵魂沙
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // TODO: 检查是否为灵魂沙
    return belowState->isSolid();
}

void NetherWartBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    i32 age = getAge(state);

    if (age < getMaxAge()) {
        // 随机生长
        if (random.nextInt(10) == 0) {
            world.setBlockState(pos, &withAge(age + 1), 2);
        }
    }
}

const CollisionShape& NetherWartBlock::getShape(const BlockState& state) const {
    i32 age = getAge(state);
    return m_shapesByAge[std::min(age, 3)];
}

const CollisionShape& NetherWartBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

} // namespace blocks
} // namespace mc
