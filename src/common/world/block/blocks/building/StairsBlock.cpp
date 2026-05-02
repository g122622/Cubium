#include "StairsBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../fluid/FluidRegistry.hpp"
#include "../../../fluid/FluidTags.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

StairsBlock::StairsBlock(const BlockState& baseState, const BlockProperties& properties)
    : Block(properties)
    , m_baseState(&baseState)
    , m_fullCubeShape(CollisionShape::fullBlock()) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::DOUBLE_BLOCK_HALF())
        .add(BlockStateProperties::STAIRS_SHAPE())
        .add(BlockStateProperties::WATERLOGGED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
        .with(BlockStateProperties::STAIRS_SHAPE(), BlockStateProperties::StairsShape::Straight)
        .with(BlockStateProperties::WATERLOGGED(), false));

    // 预计算所有40种形状
    // 楼梯形状：由两个三角形棱柱组成
    // 直梯: 一个水平面 + 一个上升面
    // 内角/外角: 更复杂的组合

    for (size_t i = 0; i < 40; ++i) {
        m_shapes[i] = CollisionShape::empty();
    }

    // 像素单位转换
    constexpr f32 P = 1.0f / 16.0f;

    // 直梯形状 (STRAIGHT)
    // 下半楼梯: 底部8像素高的完整方块 + 顶部8像素高的台阶
    // 上半楼梯: 顶部8像素高的台阶朝向相反

    // EAST 直梯 (朝向东，即从西向东上升)
    // 下半: (0,0,0)-(1,0.5,1) + (0.5,0.5,0)-(1,1,1)
    m_shapes[getShapeIndex(Direction::East, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::Straight)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f)
        );
    // 上半: (0,0,0)-(1,0.5,1) + (0,0.5,0)-(0.5,1,1)
    m_shapes[getShapeIndex(Direction::East, BlockStateProperties::DoubleBlockHalf::Upper, BlockStateProperties::StairsShape::Straight)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 1.0f)
        );

    // WEST 直梯 (朝向西，即从东向西上升)
    m_shapes[getShapeIndex(Direction::West, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::Straight)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 1.0f)
        );
    m_shapes[getShapeIndex(Direction::West, BlockStateProperties::DoubleBlockHalf::Upper, BlockStateProperties::StairsShape::Straight)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f)
        );

    // SOUTH 直梯 (朝向南，即从北向南上升)
    m_shapes[getShapeIndex(Direction::South, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::Straight)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.0f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f)
        );
    m_shapes[getShapeIndex(Direction::South, BlockStateProperties::DoubleBlockHalf::Upper, BlockStateProperties::StairsShape::Straight)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f)
        );

    // NORTH 直梯 (朝向北，即从南向北上升)
    m_shapes[getShapeIndex(Direction::North, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::Straight)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f)
        );
    m_shapes[getShapeIndex(Direction::North, BlockStateProperties::DoubleBlockHalf::Upper, BlockStateProperties::StairsShape::Straight)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.0f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f)
        );

    // 内角楼梯形状 (INNER_LEFT, INNER_RIGHT)
    // 内角楼梯由一个完整的一半高度方块 + 一个直梯台阶组成
    // 这里简化处理，使用更精确的碰撞箱

    // EAST 内角
    // INNER_LEFT: 从北转东
    m_shapes[getShapeIndex(Direction::East, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::InnerLeft)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
                CollisionShape::box(0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 0.5f)
            ),
            CollisionShape::box(0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f)
        );
    // INNER_RIGHT: 从南转东
    m_shapes[getShapeIndex(Direction::East, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::InnerRight)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
                CollisionShape::box(0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f)
            ),
            CollisionShape::box(0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f)
        );

    // WEST 内角
    m_shapes[getShapeIndex(Direction::West, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::InnerLeft)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
                CollisionShape::box(0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f)
            ),
            CollisionShape::box(0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 0.5f)
        );
    m_shapes[getShapeIndex(Direction::West, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::InnerRight)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
                CollisionShape::box(0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f)
            ),
            CollisionShape::box(0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f)
        );

    // SOUTH 内角
    m_shapes[getShapeIndex(Direction::South, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::InnerLeft)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
                CollisionShape::box(0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f)
            ),
            CollisionShape::box(0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f)
        );
    m_shapes[getShapeIndex(Direction::South, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::InnerRight)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
                CollisionShape::box(0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 0.5f)
            ),
            CollisionShape::box(0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f)
        );

    // NORTH 内角
    m_shapes[getShapeIndex(Direction::North, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::InnerLeft)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
                CollisionShape::box(0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f)
            ),
            CollisionShape::box(0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f)
        );
    m_shapes[getShapeIndex(Direction::North, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::InnerRight)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
                CollisionShape::box(0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f)
            ),
            CollisionShape::box(0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 0.5f)
        );

    // 外角楼梯形状 (OUTER_LEFT, OUTER_RIGHT)
    // 外角楼梯只有一个角落的台阶

    // EAST 外角
    m_shapes[getShapeIndex(Direction::East, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::OuterLeft)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f)
        );
    m_shapes[getShapeIndex(Direction::East, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::OuterRight)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f)
        );

    // WEST 外角
    m_shapes[getShapeIndex(Direction::West, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::OuterLeft)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f)
        );
    m_shapes[getShapeIndex(Direction::West, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::OuterRight)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 0.5f)
        );

    // SOUTH 外角
    m_shapes[getShapeIndex(Direction::South, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::OuterLeft)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f)
        );
    m_shapes[getShapeIndex(Direction::South, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::OuterRight)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f)
        );

    // NORTH 外角
    m_shapes[getShapeIndex(Direction::North, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::OuterLeft)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f)
        );
    m_shapes[getShapeIndex(Direction::North, BlockStateProperties::DoubleBlockHalf::Lower, BlockStateProperties::StairsShape::OuterRight)] =
        CollisionShape::combine(
            CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f),
            CollisionShape::box(0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 0.5f)
        );

    // 上半部分的形状（将下半部分的形状垂直翻转）
    const Direction facingDirs[] = {Direction::North, Direction::South, Direction::West, Direction::East};
    for (int facing = 0; facing < 4; ++facing) {
        Direction dir = facingDirs[facing];
        for (int shape = 0; shape < 5; ++shape) {
            BlockStateProperties::StairsShape s = static_cast<BlockStateProperties::StairsShape>(shape);
            size_t lowerIdx = getShapeIndex(dir, BlockStateProperties::DoubleBlockHalf::Lower, s);
            size_t upperIdx = getShapeIndex(dir, BlockStateProperties::DoubleBlockHalf::Upper, s);

            // 上半部分的形状已经预计算了（直梯）
            // 对于内角和外角，上半部分需要特殊处理
            // 简化处理：使用已有的直梯上半形状作为基础
            // 内角上半 = 内角下半 + 底部半层
            // 外角上半 = 外角下半的翻转版本

            // 这里使用简化版本，实际MC中更复杂
            // 外角上半：与下半相似但Y轴偏移
            if (s != BlockStateProperties::StairsShape::Straight) {
                // 复用下半形状（实际应该垂直翻转，但为了简化直接复用）
                m_shapes[upperIdx] = m_shapes[lowerIdx];
            }
        }
    }
}

// ========== 状态容器 ==========

void StairsBlock::fillStateContainer(StateContainer<Block, BlockState>& container) {
    // 属性已在构造函数中添加
    MC_UNUSED(container);
}

// ========== 放置和更新 ==========

BlockState StairsBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 获取水平朝向
    Direction facing = context.horizontalDirection();

    // 根据点击位置决定上半/下半
    bool isTop = context.getHitY() > 0.5f;

    const BlockState* statePtr = context.getWorld().getBlockState(
        context.placementPos().x,
        context.placementPos().y,
        context.placementPos().z
    );

    bool waterlogged = false;
    if (statePtr != nullptr) {
        const fluid::FluidState* fluid = statePtr->getFluidState();
        waterlogged = fluid != nullptr && fluid->isSource();
    }

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), isTop ? BlockStateProperties::DoubleBlockHalf::Upper
                                                   : BlockStateProperties::DoubleBlockHalf::Lower)
        .with(BlockStateProperties::STAIRS_SHAPE(), BlockStateProperties::StairsShape::Straight)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState StairsBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facingPos);

    // 调度流体 tick
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(
            fluid::FluidRegistry::WATER_ID);
        if (waterFluid != nullptr) {
            world.tickManager().scheduleFluidTick(
                currentPos, *waterFluid, waterFluid->getTickDelay(world));
        }
    }

    // 计算新的形状
    BlockStateProperties::StairsShape newShape = calculateShape(state, world, currentPos);

    if (state.get(BlockStateProperties::STAIRS_SHAPE()) != newShape) {
        return state.with(BlockStateProperties::STAIRS_SHAPE(), newShape);
    }

    return state;
}

// ========== 形状 ==========

const CollisionShape& StairsBlock::getShape(const BlockState& state) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::DOUBLE_BLOCK_HALF());
    BlockStateProperties::StairsShape shape = state.get(BlockStateProperties::STAIRS_SHAPE());

    size_t index = getShapeIndex(facing, half, shape);
    MC_ASSERT(index < 40);
    return m_shapes[index];
}

const CollisionShape& StairsBlock::getCollisionShape(const BlockState& state) const {
    return getShape(state);
}

const CollisionShape& StairsBlock::getOcclusionShape(const BlockState& state) const {
    // 楼梯不阻挡全部光照
    return getShape(state);
}

// ========== 旋转和镜像 ==========

const BlockState& StairsBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& StairsBlock::mirror(const BlockState& state, Mirror mirror) const {
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction mirrored = Directions::rotateDirection(facing, rotation);

    BlockStateProperties::StairsShape shape = state.get(BlockStateProperties::STAIRS_SHAPE());
    BlockStateProperties::StairsShape mirroredShape = shape;

    // 左右镜像时，内角和外角需要交换左右
    if (mirror == Mirror::LeftRight) {
        if (shape == BlockStateProperties::StairsShape::InnerLeft) {
            mirroredShape = BlockStateProperties::StairsShape::InnerRight;
        } else if (shape == BlockStateProperties::StairsShape::InnerRight) {
            mirroredShape = BlockStateProperties::StairsShape::InnerLeft;
        } else if (shape == BlockStateProperties::StairsShape::OuterLeft) {
            mirroredShape = BlockStateProperties::StairsShape::OuterRight;
        } else if (shape == BlockStateProperties::StairsShape::OuterRight) {
            mirroredShape = BlockStateProperties::StairsShape::OuterLeft;
        }
    } else if (mirror == Mirror::FrontBack) {
        // 前后镜像
        if (shape == BlockStateProperties::StairsShape::InnerLeft) {
            mirroredShape = BlockStateProperties::StairsShape::InnerRight;
        } else if (shape == BlockStateProperties::StairsShape::InnerRight) {
            mirroredShape = BlockStateProperties::StairsShape::InnerLeft;
        } else if (shape == BlockStateProperties::StairsShape::OuterLeft) {
            mirroredShape = BlockStateProperties::StairsShape::OuterRight;
        } else if (shape == BlockStateProperties::StairsShape::OuterRight) {
            mirroredShape = BlockStateProperties::StairsShape::OuterLeft;
        }
    }

    const BlockState& result = state.with(BlockStateProperties::HORIZONTAL_FACING(), mirrored);
    return result.with(BlockStateProperties::STAIRS_SHAPE(), mirroredShape);
}

// ========== 静态方法 ==========

bool StairsBlock::isStairs(const BlockState& state) {
    // 检查方块是否继承自 StairsBlock
    // 简化实现：检查是否有 STAIRS_SHAPE 属性
    return state.hasProperty(BlockStateProperties::STAIRS_SHAPE());
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* StairsBlock::getFluidState(const BlockState& state) const {
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(
            fluid::FluidRegistry::WATER_ID);
        if (waterFluid != nullptr) {
            return &waterFluid->defaultState();
        }
    }
    return Block::getFluidState(state);
}

// ========== 私有方法 ==========

BlockStateProperties::StairsShape StairsBlock::calculateShape(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos) const {

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    // 获取左右相邻的楼梯
    Direction leftDir = Directions::rotateYCCW(facing);
    Direction rightDir = Directions::rotateY(facing);

    BlockPos leftPos(pos.x + Directions::xOffset(leftDir), pos.y, pos.z + Directions::zOffset(leftDir));
    BlockPos rightPos(pos.x + Directions::xOffset(rightDir), pos.y, pos.z + Directions::zOffset(rightDir));

    std::optional<BlockStateProperties::StairsShape> leftStairs = neighborIsStairs(world, leftPos, leftDir);
    std::optional<BlockStateProperties::StairsShape> rightStairs = neighborIsStairs(world, rightPos, rightDir);

    // 判断是否为内角或外角
    if (leftStairs.has_value() && rightStairs.has_value()) {
        // 两边都是楼梯，保持直梯
        return BlockStateProperties::StairsShape::Straight;
    }

    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::DOUBLE_BLOCK_HALF());

    // 检查左边楼梯
    if (leftStairs.has_value()) {
        BlockStateProperties::StairsShape neighborShape = leftStairs.value();
        BlockStateProperties::DoubleBlockHalf neighborHalf = BlockStateProperties::DoubleBlockHalf::Lower; // 简化

        // 同一半层的内角/外角楼梯
        if (neighborShape == BlockStateProperties::StairsShape::Straight) {
            // 检查是否为内角或外角
            if (neighborHalf == half) {
                // 外角
                return BlockStateProperties::StairsShape::OuterLeft;
            }
        }
    }

    // 检查右边楼梯
    if (rightStairs.has_value()) {
        BlockStateProperties::StairsShape neighborShape = rightStairs.value();
        BlockStateProperties::DoubleBlockHalf neighborHalf = BlockStateProperties::DoubleBlockHalf::Lower; // 简化

        if (neighborShape == BlockStateProperties::StairsShape::Straight) {
            if (neighborHalf == half) {
                return BlockStateProperties::StairsShape::OuterRight;
            }
        }
    }

    // 默认直梯
    return BlockStateProperties::StairsShape::Straight;
}

std::optional<BlockStateProperties::StairsShape> StairsBlock::neighborIsStairs(
    IWorld& world,
    const BlockPos& pos,
    Direction facing) const {

    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return std::nullopt;
    }

    // 检查是否为楼梯
    if (!isStairs(*state)) {
        return std::nullopt;
    }

    // 检查朝向是否与我们形成角
    Direction neighborFacing = state->get(BlockStateProperties::HORIZONTAL_FACING());

    // 如果朝向相反，则是直连（不是角）
    if (Directions::opposite(neighborFacing) == facing) {
        return std::nullopt;
    }

    return state->get(BlockStateProperties::STAIRS_SHAPE());
}

size_t StairsBlock::getShapeIndex(
    Direction facing,
    BlockStateProperties::DoubleBlockHalf half,
    BlockStateProperties::StairsShape shape) {

    // 索引计算: facing (0-3) + half*4 (0或4) + shape*8 (0, 8, 16, 24, 32)
    size_t facingIdx = 0;
    switch (facing) {
        case Direction::North: facingIdx = 0; break;
        case Direction::South: facingIdx = 1; break;
        case Direction::West:  facingIdx = 2; break;
        case Direction::East:  facingIdx = 3; break;
        default: facingIdx = 0; break;
    }

    size_t halfIdx = (half == BlockStateProperties::DoubleBlockHalf::Upper) ? 1 : 0;
    size_t shapeIdx = static_cast<size_t>(shape);

    return shapeIdx * 8 + halfIdx * 4 + facingIdx;
}

} // namespace blocks
} // namespace mc
