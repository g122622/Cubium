#include "TrapDoorBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/BlockItemUseContext.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

TrapDoorBlock::TrapDoorBlock(const BlockProperties& properties, bool isIron)
    : Block(properties)
    , m_isIron(isIron) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::OPEN())
        .add(BlockStateProperties::HALF())
        .add(BlockStateProperties::POWERED())
        .add(BlockStateProperties::WATERLOGGED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::OPEN(), false)
        .with(BlockStateProperties::HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
        .with(BlockStateProperties::POWERED(), false)
        .with(BlockStateProperties::WATERLOGGED(), false));

    // 预计算碰撞形状
    // 像素单位
    constexpr f32 P = 1.0f / 16.0f;

    // 关闭状态：完整薄板 (厚度3像素)
    CollisionShape closedBottom = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f, 3.0f * P, 16.0f);
    CollisionShape closedTop = CollisionShape::box(0.0f, 13.0f * P, 0.0f, 16.0f, 16.0f, 16.0f);

    // 打开状态：侧面薄板
    // NORTH: z=0-3
    CollisionShape openBottomNorth = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f, 16.0f, 3.0f * P);
    CollisionShape openTopNorth = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f, 16.0f, 3.0f * P);
    // SOUTH: z=13-16
    CollisionShape openBottomSouth = CollisionShape::box(0.0f, 0.0f, 13.0f * P, 16.0f, 16.0f, 16.0f);
    CollisionShape openTopSouth = CollisionShape::box(0.0f, 0.0f, 13.0f * P, 16.0f, 16.0f, 16.0f);
    // EAST: x=13-16
    CollisionShape openBottomEast = CollisionShape::box(13.0f * P, 0.0f, 0.0f, 16.0f, 16.0f, 16.0f);
    CollisionShape openTopEast = CollisionShape::box(13.0f * P, 0.0f, 0.0f, 16.0f, 16.0f, 16.0f);
    // WEST: x=0-3
    CollisionShape openBottomWest = CollisionShape::box(0.0f, 0.0f, 0.0f, 3.0f * P, 16.0f, 16.0f);
    CollisionShape openTopWest = CollisionShape::box(0.0f, 0.0f, 0.0f, 3.0f * P, 16.0f, 16.0f);

    // 初始化形状缓存
    // 索引: facing * 4 + (open ? 2 : 0) + (half == Upper ? 1 : 0)
    // facing: NORTH=0, SOUTH=1, EAST=2, WEST=3

    // 关闭状态
    m_shapes[getShapeIndex(Direction::North, false, BlockStateProperties::DoubleBlockHalf::Lower)] = closedBottom;
    m_shapes[getShapeIndex(Direction::North, false, BlockStateProperties::DoubleBlockHalf::Upper)] = closedTop;
    m_shapes[getShapeIndex(Direction::South, false, BlockStateProperties::DoubleBlockHalf::Lower)] = closedBottom;
    m_shapes[getShapeIndex(Direction::South, false, BlockStateProperties::DoubleBlockHalf::Upper)] = closedTop;
    m_shapes[getShapeIndex(Direction::East, false, BlockStateProperties::DoubleBlockHalf::Lower)] = closedBottom;
    m_shapes[getShapeIndex(Direction::East, false, BlockStateProperties::DoubleBlockHalf::Upper)] = closedTop;
    m_shapes[getShapeIndex(Direction::West, false, BlockStateProperties::DoubleBlockHalf::Lower)] = closedBottom;
    m_shapes[getShapeIndex(Direction::West, false, BlockStateProperties::DoubleBlockHalf::Upper)] = closedTop;

    // 打开状态
    m_shapes[getShapeIndex(Direction::North, true, BlockStateProperties::DoubleBlockHalf::Lower)] = openBottomNorth;
    m_shapes[getShapeIndex(Direction::North, true, BlockStateProperties::DoubleBlockHalf::Upper)] = openTopNorth;
    m_shapes[getShapeIndex(Direction::South, true, BlockStateProperties::DoubleBlockHalf::Lower)] = openBottomSouth;
    m_shapes[getShapeIndex(Direction::South, true, BlockStateProperties::DoubleBlockHalf::Upper)] = openTopSouth;
    m_shapes[getShapeIndex(Direction::East, true, BlockStateProperties::DoubleBlockHalf::Lower)] = openBottomEast;
    m_shapes[getShapeIndex(Direction::East, true, BlockStateProperties::DoubleBlockHalf::Upper)] = openTopEast;
    m_shapes[getShapeIndex(Direction::West, true, BlockStateProperties::DoubleBlockHalf::Lower)] = openBottomWest;
    m_shapes[getShapeIndex(Direction::West, true, BlockStateProperties::DoubleBlockHalf::Upper)] = openTopWest;
}

// ========== 放置和更新 ==========

BlockState TrapDoorBlock::getStateForPlacement(BlockItemUseContext& context) {
    BlockPos pos = context.placementPos();
    Direction clickedFace = context.getClickedFace();
    const IWorld& world = context.getWorld();

    // 确定朝向（朝向玩家相反的方向）
    Direction facing = Directions::opposite(context.horizontalDirection());

    // 确定上半/下半
    BlockStateProperties::DoubleBlockHalf half;
    if (clickedFace == Direction::Up) {
        half = BlockStateProperties::DoubleBlockHalf::Lower;
    } else if (clickedFace == Direction::Down) {
        half = BlockStateProperties::DoubleBlockHalf::Upper;
    } else {
        // 点击侧面，根据点击位置决定
        half = (context.getHitY() > 0.5f)
            ? BlockStateProperties::DoubleBlockHalf::Upper
            : BlockStateProperties::DoubleBlockHalf::Lower;
    }

    // 检查是否含水
    const BlockState* existingState = world.getBlockState(pos.x, pos.y, pos.z);
    bool waterlogged = false;
    if (existingState != nullptr) {
        const fluid::FluidState* fluid = existingState->getFluidState();
        waterlogged = fluid != nullptr && fluid->isSource();
    }

    // TODO: 检查红石信号
    bool powered = false;

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::OPEN(), false)
        .with(BlockStateProperties::HALF(), half)
        .with(BlockStateProperties::POWERED(), powered)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool TrapDoorBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 检查下方是否有支撑
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);

    // 简化检查：下方有固体方块即可
    // 实际MC中更复杂，需要检查方块是否提供支撑面
    return belowState != nullptr && belowState->isSolid();
}

BlockState TrapDoorBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查支撑
    if (facing == Direction::Down) {
        BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::HALF());
        if (half == BlockStateProperties::DoubleBlockHalf::Lower) {
            // 下方支撑被破坏
            const BlockState* belowState = world.getBlockState(currentPos.x, currentPos.y - 1, currentPos.z);
            if (belowState == nullptr || !belowState->isSolid()) {
                return world.getBlockState(currentPos.x, currentPos.y, currentPos.z)->getBlock().defaultState();
            }
        }
    }

    return state;
}

void TrapDoorBlock::neighborChanged(IWorld& world, const BlockPos& pos,
                                     Block& neighborBlock, const BlockPos& neighborPos,
                                     bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* statePtr = world.getBlockState(pos.x, pos.y, pos.z);
    if (statePtr == nullptr || &statePtr->getBlock() != this) {
        return;
    }

    BlockState state = *statePtr;
    bool wasPowered = state.get(BlockStateProperties::POWERED());

    // TODO: 实现红石信号检测
    bool isPowered = false; // world.isBlockPowered(pos);

    if (isPowered != wasPowered) {
        // 红石信号改变，切换开关状态
        bool wasOpen = state.get(BlockStateProperties::OPEN());
        BlockState newState = state
            .with(BlockStateProperties::POWERED(), isPowered)
            .with(BlockStateProperties::OPEN(), isPowered);

        world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);

        if (wasOpen != isPowered) {
            playSound(world, pos, isPowered);
        }
    }
}

// ========== 交互 ==========

ActionResult TrapDoorBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 铁活板门不能手动开关
    if (m_isIron) {
        return ActionResult::Pass;
    }

    // 切换开关状态
    bool wasOpen = state.get(BlockStateProperties::OPEN());
    BlockState newState = state.with(BlockStateProperties::OPEN(), !wasOpen);
    world.setBlockState(pos.x, pos.y, pos.z, &newState, 10);

    playSound(world, pos, !wasOpen);

    return ActionResult::Success;
}

// ========== 形状 ==========

const CollisionShape& TrapDoorBlock::getShape(const BlockState& state) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    bool open = state.get(BlockStateProperties::OPEN());
    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::HALF());

    size_t index = getShapeIndex(facing, open, half);
    MC_ASSERT(index < 16);
    return m_shapes[index];
}

const CollisionShape& TrapDoorBlock::getCollisionShape(const BlockState& state) const {
    // 打开时无碰撞
    if (state.get(BlockStateProperties::OPEN())) {
        static CollisionShape emptyShape = CollisionShape::empty();
        return emptyShape;
    }
    return getShape(state);
}

// ========== 旋转和镜像 ==========

const BlockState& TrapDoorBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& TrapDoorBlock::mirror(const BlockState& state, Mirror mirror) const {
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction mirrored = Directions::rotateDirection(facing, rotation);

    return state.with(BlockStateProperties::HORIZONTAL_FACING(), mirrored);
}

// ========== 静态方法 ==========

bool TrapDoorBlock::isOpen(const BlockState& state) {
    return state.get(BlockStateProperties::OPEN());
}

void TrapDoorBlock::toggle(IWorld& world, const BlockPos& pos, const BlockState& state, bool open) {
    if (state.get(BlockStateProperties::OPEN()) != open) {
        BlockState newState = state.with(BlockStateProperties::OPEN(), open);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 10);
    }
}

// ========== 私有方法 ==========

void TrapDoorBlock::playSound(IWorld& world, const BlockPos& pos, bool isOpening) {
    // TODO: 实现音效系统
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(isOpening);
}

size_t TrapDoorBlock::getShapeIndex(Direction facing, bool open, BlockStateProperties::DoubleBlockHalf half) {
    size_t facingIdx = 0;
    switch (facing) {
        case Direction::North: facingIdx = 0; break;
        case Direction::South: facingIdx = 1; break;
        case Direction::East:  facingIdx = 2; break;
        case Direction::West:  facingIdx = 3; break;
        default: facingIdx = 0; break;
    }

    size_t halfIdx = (half == BlockStateProperties::DoubleBlockHalf::Upper) ? 1 : 0;
    size_t openIdx = open ? 2 : 0;

    return facingIdx * 4 + openIdx + halfIdx;
}

} // namespace blocks
} // namespace mc
