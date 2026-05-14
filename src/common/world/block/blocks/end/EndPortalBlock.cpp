/*
* Copyright (c) 2026 Guo Yi
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
*/

#include "EndPortalBlock.hpp"
#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "../../VanillaBlocks.hpp"
#include <array>

namespace mc {
namespace blocks {

// ========== EndPortalBlock ==========

EndPortalBlock::EndPortalBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 传送门没有碰撞箱
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.75f, 1.0f);
}

void EndPortalBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity)
{
    // 参考 MC 1.16.5 EndPortalBlock.onEntityCollision
    // 末地传送门是立即传送的，不需要等待时间
    // 玩家进入末地传送门后会立即传送到末地出生点 (100, 49, 0)

    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 检查传送冷却
    if (!entity.canTeleport()) {
        return; // 还在冷却中
    }

    // 设置传送冷却，防止重复传送
    entity.setPortalCooldown(300); // 15秒冷却

    // 确定目标维度
    // MC 1.16.5 标准：主世界=0，下界=-1，末地=1
    // 主世界 -> 末地: 传送到固定出生点 (100, 49, 0)
    // 末地 -> 主世界: 返回重生点或床
    DimensionId targetDim = (entity.dimension() == 1) ? DimensionId(0) : DimensionId(1); // THE_END=1, OVERWORLD=0

    // 设置实体的目标维度标志
    // 实际传送由 ServerDimensionManager 处理
    // 这里只设置传送请求标志
    entity.setDimension(targetDim);

    // 注意：实际的维度切换逻辑由服务端的 ServerDimensionManager 处理
    // 客户端只需要处理动画效果
}

const CollisionShape& EndPortalBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& EndPortalBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== EndPortalFrameBlock ==========

EndPortalFrameBlock::EndPortalFrameBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::EYE())
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::EYE(), false)
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));

    // 创建形状
    m_frameShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.8125f, 1.0f);
    m_frameWithEyeShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

bool EndPortalFrameBlock::hasEye(const BlockState& state) const
{
    return state.get(BlockStateProperties::EYE());
}

Direction EndPortalFrameBlock::getFacing(const BlockState& state) const
{
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

BlockState EndPortalFrameBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction facing = context.horizontalDirection();
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

const BlockState& EndPortalFrameBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& EndPortalFrameBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const CollisionShape& EndPortalFrameBlock::getShape(const BlockState& state) const
{
    return hasEye(state) ? m_frameWithEyeShape : m_frameShape;
}

// ========== EndGatewayBlock ==========

EndGatewayBlock::EndGatewayBlock(const BlockProperties& properties)
    : Block(properties)
{
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

void EndGatewayBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity)
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(entity);
    // TODO: 折跃门传送逻辑
}

const CollisionShape& EndGatewayBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& EndGatewayBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== ChorusPlantBlock ==========

ChorusPlantBlock::ChorusPlantBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器（6个方向的连接）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::NORTH())
            .add(BlockStateProperties::SOUTH())
            .add(BlockStateProperties::EAST())
            .add(BlockStateProperties::WEST())
            .add(BlockStateProperties::DOWN())
            .add(BlockStateProperties::UP())
            .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
    createBlockState(std::move(container));

    // 设置默认状态（无连接）
    setDefaultState(defaultState()
            .with(BlockStateProperties::NORTH(), false)
            .with(BlockStateProperties::SOUTH(), false)
            .with(BlockStateProperties::EAST(), false)
            .with(BlockStateProperties::WEST(), false)
            .with(BlockStateProperties::DOWN(), false)
            .with(BlockStateProperties::UP(), false));

    // 参考 MC 1.16.5 SixWayBlock.makeShapes
    // apothem = 0.3125F，即 5/16 像素
    // 中心柱尺寸：从方块中心向各方向偏移 apothem
    constexpr f32 apothem = 0.3125f;
    constexpr f32 f = 0.5f - apothem;  // 0.1875 (3 像素)
    constexpr f32 f1 = 0.5f + apothem; // 0.8125 (13 像素)

    // 中心柱形状：(3, 3, 3) -> (13, 13, 13) 像素
    // MC 1.16.5: Block.makeCuboidShape(f*16, f*16, f*16, f1*16, f1*16, f1*16)
    m_centerShape = CollisionShape::box(f, f, f, f1, f1, f1);

    // 计算各方向的臂形状
    // Direction 枚举顺序：Down=0, Up=1, North=2, South=3, West=4, East=5
    // MC 1.16.5: VoxelShapes.create(0.5 + min(-apothem, offset*0.5), ...)
    for (int i = 0; i < 6; ++i) {
        Direction dir = static_cast<Direction>(i);
        f32 xOffset = static_cast<f32>(Directions::xOffset(dir));
        f32 yOffset = static_cast<f32>(Directions::yOffset(dir));
        f32 zOffset = static_cast<f32>(Directions::zOffset(dir));

        // 臂形状边界计算
        // 向正方向延伸到方块边缘，向负方向只延伸到中心柱边缘
        f32 minX = 0.5f + std::min(-apothem, xOffset * 0.5f);
        f32 minY = 0.5f + std::min(-apothem, yOffset * 0.5f);
        f32 minZ = 0.5f + std::min(-apothem, zOffset * 0.5f);
        f32 maxX = 0.5f + std::max(apothem, xOffset * 0.5f);
        f32 maxY = 0.5f + std::max(apothem, yOffset * 0.5f);
        f32 maxZ = 0.5f + std::max(apothem, zOffset * 0.5f);

        m_armShapes[i] = CollisionShape::box(minX, minY, minZ, maxX, maxY, maxZ);
    }

    // 预计算所有 64 种组合形状
    // 索引计算：Down=bit0, Up=bit1, North=bit2, South=bit3, West=bit4, East=bit5
    for (size_t k = 0; k < 64; ++k) {
        CollisionShape shape = m_centerShape;

        for (int j = 0; j < 6; ++j) {
            if ((k & (1ULL << j)) != 0) {
                shape = CollisionShape::combine(shape, m_armShapes[j]);
            }
        }

        m_shapes[k] = shape;
    }
}

BlockState ChorusPlantBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 计算各方向的连接
    const IBlockReader& blockReader = static_cast<const IBlockReader&>(world);
    bool north = canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::North);
    bool south = canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::South);
    bool east = canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::East);
    bool west = canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::West);
    bool up = canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::Up);
    bool down = canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::Down);

    return defaultState()
        .with(BlockStateProperties::NORTH(), north)
        .with(BlockStateProperties::SOUTH(), south)
        .with(BlockStateProperties::EAST(), east)
        .with(BlockStateProperties::WEST(), west)
        .with(BlockStateProperties::UP(), up)
        .with(BlockStateProperties::DOWN(), down);
}

bool ChorusPlantBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 检查是否有至少一个连接
    for (int i = 0; i < 6; ++i) {
        Direction dir = static_cast<Direction>(i);
        if (canConnect(world, pos, dir)) {
            return true;
        }
    }

    return false;
}

BlockState ChorusPlantBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facingPos);

    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    bool connected = canConnect(blockReader, currentPos, facing);

    switch (facing) {
        case Direction::North:
            return state.with(BlockStateProperties::NORTH(), connected);
        case Direction::South:
            return state.with(BlockStateProperties::SOUTH(), connected);
        case Direction::East:
            return state.with(BlockStateProperties::EAST(), connected);
        case Direction::West:
            return state.with(BlockStateProperties::WEST(), connected);
        case Direction::Up:
            return state.with(BlockStateProperties::UP(), connected);
        case Direction::Down:
            return state.with(BlockStateProperties::DOWN(), connected);
        default:
            return state;
    }
}

const CollisionShape& ChorusPlantBlock::getShape(const BlockState& state) const
{
    const size_t index = getShapeIndex(state);
    MC_ASSERT_RELEASE(index < m_shapes.size());
    return m_shapes[index];
}

// static
size_t ChorusPlantBlock::getShapeIndex(const BlockState& state)
{
    // 位掩码索引：Down=bit0, Up=bit1, North=bit2, South=bit3, West=bit4, East=bit5
    // 与 Direction 枚举顺序一致：Down=0, Up=1, North=2, South=3, West=4, East=5
    size_t index = 0;

    if (state.get(BlockStateProperties::DOWN())) index |= 1ULL << 0;  // bit 0
    if (state.get(BlockStateProperties::UP())) index |= 1ULL << 1;    // bit 1
    if (state.get(BlockStateProperties::NORTH())) index |= 1ULL << 2; // bit 2
    if (state.get(BlockStateProperties::SOUTH())) index |= 1ULL << 3; // bit 3
    if (state.get(BlockStateProperties::WEST())) index |= 1ULL << 4;  // bit 4
    if (state.get(BlockStateProperties::EAST())) index |= 1ULL << 5;  // bit 5

    return index;
}

bool ChorusPlantBlock::canConnect(IBlockReader& world, const BlockPos& pos, Direction direction) const
{
    BlockPos adjPos = pos.offset(direction);
    const BlockState* adjState = world.getBlockState(adjPos);

    if (adjState == nullptr) {
        return false;
    }

    const Block& adjBlock = adjState->getBlock();

    // 连接到紫颂植物
    if (adjState->is(this)) {
        return true;
    }

    // 连接到紫颂花
    if (&adjBlock == VanillaBlocks::CHORUS_FLOWER) {
        return true;
    }

    // 仅下方可连接到末地石（作为生长基底）
    if (direction == Direction::Down && &adjBlock == VanillaBlocks::END_STONE) {
        return true;
    }

    return false;
}

// ========== ChorusFlowerBlock ==========

ChorusFlowerBlock::ChorusFlowerBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_5())
            .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::AGE_0_5(), 0));

    // 创建各年龄形状
    for (int i = 0; i < 6; ++i) {
        m_shapesByAge[i] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    }
}

i32 ChorusFlowerBlock::getAge(const BlockState& state) const
{
    return state.get(BlockStateProperties::AGE_0_5());
}

BlockState ChorusFlowerBlock::withAge(i32 age) const
{
    return defaultState().with(BlockStateProperties::AGE_0_5(), std::min(age, 5));
}

BlockState ChorusFlowerBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

bool ChorusFlowerBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 检查下方
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 需要在紫颂植物上或末地石上
    // TODO: 检查特定方块
    return belowState->isSolid();
}

void ChorusFlowerBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    i32 age = getAge(state);

    if (age < getMaxAge()) {
        // 随机生长
        if (random.nextInt(5) == 0) {
            BlockState newState = withAge(age + 1);
            world.setBlockState(pos, &newState, 2);
        }
    }
}

const CollisionShape& ChorusFlowerBlock::getShape(const BlockState& state) const
{
    i32 age = getAge(state);
    return m_shapesByAge[std::min(age, 5)];
}

// ========== DragonEggBlock ==========

DragonEggBlock::DragonEggBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 龙蛋形状
    m_shape = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 1.0f, 0.9375f);
}

BlockState DragonEggBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

ActionResultType DragonEggBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 点击传送
    teleport(world, pos, state);
    return ActionResultType::Success;
}

void DragonEggBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{

    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 邻居变化时可能传送
    // teleport(world, pos, ...);
}

void DragonEggBlock::teleport(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // TODO: 实现传送逻辑
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
}

const CollisionShape& DragonEggBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

} // namespace blocks
} // namespace mc
