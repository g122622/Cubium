#include "HopperBlock.hpp"
#include "../../blockentity/transport/HopperEntity.hpp"
#include "../../IWorld.hpp"
#include "../../../entity/Player.hpp"
#include "../../../item/ItemStack.hpp"
#include "../../../item/BlockItemUseContext.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

HopperBlock::HopperBlock(const BlockProperties& properties)
    : Block(properties) {
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::FACING_EXCEPT_UP())
        .add(BlockStateProperties::ENABLED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));
    setDefaultState(defaultState()
        .with(BlockStateProperties::FACING_EXCEPT_UP(), Direction::Down)
        .with(BlockStateProperties::ENABLED(), true));
    initShapes();
}

// ========== 放置和更新 ==========

BlockState HopperBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 漏斗输出方向为玩家点击面的反方向
    Direction facing = context.getClickedFace();
    Direction outputDir = Directions::opposite(facing);

    // 如果输出方向是上，改为向下
    if (outputDir == Direction::Up) {
        outputDir = Direction::Down;
    }

    return defaultState()
        .with(BlockStateProperties::FACING_EXCEPT_UP(), outputDir)
        .with(BlockStateProperties::ENABLED(), true);
}

void HopperBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 检查红石信号并更新启用状态
    updateState(world, pos, state);
}

void HopperBlock::neighborChanged(
    IWorld& world,
    const BlockPos& pos,
    Block& neighborBlock,
    const BlockPos& neighborPos,
    bool isMoving) {

    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 更新红石状态
    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (state != nullptr) {
        updateState(world, pos, *state);
    }
}

// ========== 形状 ==========

const CollisionShape& HopperBlock::getShape(const BlockState& state) const {
    Direction facing = state.get(BlockStateProperties::FACING_EXCEPT_UP());
    return m_shapes[static_cast<size_t>(facing)];
}

const CollisionShape& HopperBlock::getRaytraceShape(const BlockState& state) const {
    Direction facing = state.get(BlockStateProperties::FACING_EXCEPT_UP());
    return m_raytraceShapes[static_cast<size_t>(facing)];
}

// ========== 方块实体 ==========

std::unique_ptr<BlockEntity> HopperBlock::createBlockEntity(const BlockPos& pos) {
    return std::make_unique<blockentity::HopperEntity>(pos);
}

// ========== 交互 ==========

ActionResult HopperBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 客户端直接返回成功
    // TODO: 检查 world.isRemote()
    // if (world.isRemote()) {
    //     return ActionResult::SUCCESS;
    // }

    // 打开漏斗GUI
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::Hopper) {
        // auto* hopper = static_cast<blockentity::HopperEntity*>(blockEntity);
        // player.openContainer(hopper);
        // player.addStat(Stats.INSPECT_HOPPER);
        return ActionResult::Consume;
    }

    return ActionResult::Pass;
}

// ========== 红石 ==========

i32 HopperBlock::getComparatorInputOverride(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::Hopper) {
        return 0;
    }

    auto* hopper = static_cast<blockentity::HopperEntity*>(blockEntity);
    IInventory* inventory = hopper->getInventory();
    if (inventory == nullptr) {
        return 0;
    }

    // 计算红石比较器信号
    // 公式: 信号 = floor(填充度 * 14) + (非空? 1 : 0)
    i32 totalItems = 0;
    i32 totalSlots = inventory->getContainerSize();

    for (i32 i = 0; i < totalSlots; ++i) {
        const ItemStack& stack = inventory->getItem(i);
        if (!stack.isEmpty()) {
            totalItems += stack.getCount();
        }
    }

    i32 maxItems = totalSlots * 64;  // 假设最大堆叠为64
    if (maxItems == 0) {
        return 0;
    }

    f32 fillRatio = static_cast<f32>(totalItems) / static_cast<f32>(maxItems);
    i32 signal = static_cast<i32>(fillRatio * 14.0f);

    // 如果有任何物品，+1
    if (totalItems > 0) {
        signal += 1;
    }

    return std::min(signal, 15);
}

// ========== 实体碰撞 ==========

void HopperBlock::onEntityCollision(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Entity& entity) {

    MC_UNUSED(state);

    // 获取漏斗实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::Hopper) {
        return;
    }

    auto* hopper = static_cast<blockentity::HopperEntity*>(blockEntity);
    hopper->onEntityCollision(world, &entity);
}

// ========== 旋转和镜像 ==========

const BlockState& HopperBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::FACING_EXCEPT_UP());
    Direction rotated = Directions::rotateDirection(facing, rotation);

    // 如果旋转后变成 Up，保持原方向
    if (rotated == Direction::Up) {
        return state;
    }

    return state.with(BlockStateProperties::FACING_EXCEPT_UP(), rotated);
}

const BlockState& HopperBlock::mirror(const BlockState& state, Mirror mirror) const {
    Direction facing = state.get(BlockStateProperties::FACING_EXCEPT_UP());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

// ========== 静态工具方法 ==========

Direction HopperBlock::getFacing(const BlockState& state) {
    return state.get(BlockStateProperties::FACING_EXCEPT_UP());
}

bool HopperBlock::isEnabled(const BlockState& state) {
    return state.get(BlockStateProperties::ENABLED());
}

// ========== 私有方法 ==========

void HopperBlock::updateState(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 检查红石信号
    // 如果有红石信号，禁用漏斗；否则启用
    // TODO: 实现 world.isBlockPowered(pos)
    // bool powered = world.isBlockPowered(pos);
    bool powered = false;  // 暂时假设无红石信号

    bool enabled = !powered;
    bool currentEnabled = isEnabled(state);

    if (enabled != currentEnabled) {
        // 更新方块状态
        const BlockState* newState = world.getBlockState(pos.x, pos.y, pos.z);
        if (newState != nullptr) {
            // world.setBlockState(pos, state.with(BlockStateProperties::ENABLED(), enabled), 4);
        }
    }
}

void HopperBlock::initShapes() {
    // 漏斗形状定义（像素坐标，16像素=1方块）
    //
    // 输入碗形状 (INPUT_SHAPE): (0, 10, 0) -> (16, 16, 16)
    // 中间管道形状 (MIDDLE_SHAPE): (4, 4, 4) -> (12, 10, 12)
    // 内部碗形状 (INSIDE_BOWL_SHAPE): (2, 11, 2) -> (14, 16, 14)
    //
    // 不同方向的输出管道形状:
    // - DOWN: (6, 0, 6) -> (10, 4, 10)
    // - NORTH: (6, 4, 0) -> (10, 8, 4)
    // - SOUTH: (6, 4, 12) -> (10, 8, 16)
    // - WEST: (0, 4, 6) -> (4, 8, 10)
    // - EAST: (12, 4, 6) -> (16, 8, 10)

    // 输入碗 (顶部)
    CollisionShape inputShape = CollisionShape::fromPixelBox(0, 10, 0, 16, 16, 16);

    // 中间管道
    CollisionShape middleShape = CollisionShape::fromPixelBox(4, 4, 4, 12, 10, 12);

    // 合并输入碗和中间管道
    CollisionShape baseShape = CollisionShape::combine(
        inputShape, middleShape, CollisionShape::CombineOp::OR);

    // 向下方向的输出管道
    CollisionShape downSpout = CollisionShape::fromPixelBox(6, 0, 6, 10, 4, 10);
    m_shapes[static_cast<size_t>(Direction::Down)] = CollisionShape::combine(
        baseShape, downSpout, CollisionShape::CombineOp::OR);

    // 向北方向的输出管道
    CollisionShape northSpout = CollisionShape::fromPixelBox(6, 4, 0, 10, 8, 4);
    m_shapes[static_cast<size_t>(Direction::North)] = CollisionShape::combine(
        baseShape, northSpout, CollisionShape::CombineOp::OR);

    // 向南方向的输出管道
    CollisionShape southSpout = CollisionShape::fromPixelBox(6, 4, 12, 10, 8, 16);
    m_shapes[static_cast<size_t>(Direction::South)] = CollisionShape::combine(
        baseShape, southSpout, CollisionShape::CombineOp::OR);

    // 向西方向的输出管道
    CollisionShape westSpout = CollisionShape::fromPixelBox(0, 4, 6, 4, 8, 10);
    m_shapes[static_cast<size_t>(Direction::West)] = CollisionShape::combine(
        baseShape, westSpout, CollisionShape::CombineOp::OR);

    // 向东方向的输出管道
    CollisionShape eastSpout = CollisionShape::fromPixelBox(12, 4, 6, 16, 8, 10);
    m_shapes[static_cast<size_t>(Direction::East)] = CollisionShape::combine(
        baseShape, eastSpout, CollisionShape::CombineOp::OR);

    // Up 方向不会使用，但初始化为 baseShape
    m_shapes[static_cast<size_t>(Direction::Up)] = baseShape;

    // 射线追踪形状（内部空间）
    // 内部碗形状: (2, 11, 2) -> (14, 16, 14)
    CollisionShape insideBowl = CollisionShape::fromPixelBox(2, 11, 2, 14, 16, 14);

    // 各方向的射线追踪形状
    m_raytraceShapes[static_cast<size_t>(Direction::Down)] = insideBowl;
    m_raytraceShapes[static_cast<size_t>(Direction::Up)] = insideBowl;

    m_raytraceShapes[static_cast<size_t>(Direction::North)] = CollisionShape::combine(
        insideBowl, CollisionShape::fromPixelBox(6, 8, 0, 10, 10, 4), CollisionShape::CombineOp::OR);

    m_raytraceShapes[static_cast<size_t>(Direction::South)] = CollisionShape::combine(
        insideBowl, CollisionShape::fromPixelBox(6, 8, 12, 10, 10, 16), CollisionShape::CombineOp::OR);

    m_raytraceShapes[static_cast<size_t>(Direction::West)] = CollisionShape::combine(
        insideBowl, CollisionShape::fromPixelBox(0, 8, 6, 4, 10, 10), CollisionShape::CombineOp::OR);

    m_raytraceShapes[static_cast<size_t>(Direction::East)] = CollisionShape::combine(
        insideBowl, CollisionShape::fromPixelBox(12, 8, 6, 16, 10, 10), CollisionShape::CombineOp::OR);
}

} // namespace blocks
} // namespace mc
