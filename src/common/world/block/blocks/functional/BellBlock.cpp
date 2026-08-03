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

#include "BellBlock.hpp"
#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../entity/entities/projectile/ProjectileEntity.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../stats/Stats.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../../blockentity/BlockEntity.hpp"
#include "../../../blockentity/interactive/BellBlockEntity.hpp"
#include "../../../gameevent/GameEvents.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../BlockState.hpp"
#include "../../registry/VanillaBlocks.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <array>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// 常量
// ============================================================================

namespace {

/// 钟方块事件 ID：敲响动画
/// 参考: net.minecraft.world.level.block.BellBlock#EVENT_BELL_RING
constexpr i32 EVENT_BELL_RING = 1;

/// 1 像素 = 1/16 格
constexpr f32 P = 1.0f / 16.0f;

} // namespace

// ============================================================================
// 构造函数
// ============================================================================

BellBlock::BellBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::BELL_ATTACHMENT())
            .add(BlockStateProperties::POWERED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::Floor)
            .with(BlockStateProperties::POWERED(), false));

    // 初始化所有形状
    _initializeShapes();
}

// ============================================================================
// 形状初始化
// ============================================================================

void BellBlock::_initializeShapes()
{
    // 参考 MC 1.21.11 BellBlock.java:
    //   BELL_SHAPE = Shapes.or(Block.column(6.0, 6.0, 13.0), Block.column(8.0, 4.0, 6.0));
    //   SHAPE_CEILING = Shapes.or(BELL_SHAPE, Block.column(2.0, 13.0, 16.0));
    //   SHAPE_FLOOR = Shapes.rotateHorizontalAxis(Block.cube(16.0, 16.0, 8.0));
    //   SHAPE_DOUBLE_WALL = Shapes.rotateHorizontalAxis(
    //       Shapes.or(BELL_SHAPE, Block.column(2.0, 16.0, 13.0, 15.0)));
    //   SHAPE_SINGLE_WALL = Shapes.rotateHorizontal(
    //       Shapes.or(BELL_SHAPE, Block.boxZ(2.0, 13.0, 15.0, 0.0, 13.0)));
    //
    // 简化策略：
    // - BELL_SHAPE: 钟身底部圆盘 (6×6, Y=6-13) + 顶部圆盘 (8×8, Y=4-6)
    // - 天花板：BELL_SHAPE + 顶部连接柱 (2×2, Y=13-16)
    // - 地面：按朝向轴区分的水平长方体 (16×16×8 或 8×16×16)
    // - 双面墙：BELL_SHAPE + 单侧连接柱 (2×2, Y=13-15)
    // - 单面墙：BELL_SHAPE + 朝向方向的连接柱 (2×2, Y=13-15)

    // BELL_SHAPE（钟身）
    const CollisionShape bellBottom =
        CollisionShape::box(5.0f * P, 6.0f * P, 5.0f * P, 11.0f * P, 13.0f * P, 11.0f * P);
    const CollisionShape bellTop = CollisionShape::box(4.0f * P, 4.0f * P, 4.0f * P, 12.0f * P, 6.0f * P, 12.0f * P);
    const CollisionShape bellShape = CollisionShape::combine(bellBottom, bellTop);

    // 天花板：BELL_SHAPE + 顶部连接柱 (2×2, Y=13-16)
    m_ceilingShape = CollisionShape::combine(
        bellShape, CollisionShape::box(7.0f * P, 13.0f * P, 7.0f * P, 9.0f * P, 1.0f, 9.0f * P));

    // 地面（按朝向轴区分）：
    // - X 轴 (East/West)：钟身 + 朝 X 方向延伸的连接柱
    // - Z 轴 (North/South)：钟身 + 朝 Z 方向延伸的连接柱
    // 简化为：钟身长方体 (8 像素厚，沿朝向轴方向)
    // X 轴朝向：8×16×16（厚度在 X 方向）
    m_floorShapeX = CollisionShape::box(4.0f * P, 0.0f, 0.0f, 12.0f * P, 1.0f, 1.0f);
    // Z 轴朝向：16×16×8（厚度在 Z 方向）
    m_floorShapeZ = CollisionShape::box(0.0f, 0.0f, 4.0f * P, 1.0f, 1.0f, 12.0f * P);

    // 双面墙（按朝向轴区分）：BELL_SHAPE + 单侧连接柱 (2×2, Y=13-15)
    // X 轴朝向：连接柱在 X 方向两侧
    m_doubleWallShapeX = CollisionShape::combine(
        bellShape, CollisionShape::box(7.0f * P, 13.0f * P, 7.0f * P, 9.0f * P, 15.0f * P, 9.0f * P));
    // Z 轴朝向：相同（连接柱在 Z 方向两侧，但连接柱本身是中心对称的）
    m_doubleWallShapeZ = m_doubleWallShapeX;

    // 单面墙（按 4 个朝向区分）：
    // - North 朝向：连接柱在 Z=0 侧
    // - South 朝向：连接柱在 Z=1 侧
    // - East 朝向：连接柱在 X=1 侧
    // - West 朝向：连接柱在 X=0 侧
    m_singleWallShapeNorth = CollisionShape::combine(
        bellShape, CollisionShape::box(7.0f * P, 13.0f * P, 0.0f, 9.0f * P, 15.0f * P, 2.0f * P));
    m_singleWallShapeSouth = CollisionShape::combine(
        bellShape, CollisionShape::box(7.0f * P, 13.0f * P, 14.0f * P, 9.0f * P, 15.0f * P, 1.0f));
    m_singleWallShapeEast = CollisionShape::combine(
        bellShape, CollisionShape::box(14.0f * P, 13.0f * P, 7.0f * P, 1.0f, 15.0f * P, 9.0f * P));
    m_singleWallShapeWest = CollisionShape::combine(
        bellShape, CollisionShape::box(0.0f, 13.0f * P, 7.0f * P, 2.0f * P, 15.0f * P, 9.0f * P));

    // 填充 m_shapesByState 缓存（按 (facing_index << 2) | attachment_index 索引）
    // HORIZONTAL_FACING 的 4 个值：North=0, East=1, South=2, West=3（按 Directions::horizontal 顺序）
    // BELL_ATTACHMENT 的 4 个值：Floor=0, Ceiling=1, SingleWall=2, DoubleWall=3
    const std::array<Direction, 4> horizDirs = {Direction::North, Direction::East, Direction::South, Direction::West};
    const std::array<BlockStateProperties::BellAttachment, 4> attachments = {
        BlockStateProperties::BellAttachment::Floor,
        BlockStateProperties::BellAttachment::Ceiling,
        BlockStateProperties::BellAttachment::SingleWall,
        BlockStateProperties::BellAttachment::DoubleWall};

    for (size_t facingIdx = 0; facingIdx < 4; ++facingIdx) {
        for (size_t attachIdx = 0; attachIdx < 4; ++attachIdx) {
            const size_t index = (facingIdx << 2) | attachIdx;
            const Direction facing = horizDirs[facingIdx];
            const BlockStateProperties::BellAttachment attachment = attachments[attachIdx];

            const Axis axis = Directions::getAxis(facing);
            switch (attachment) {
                case BlockStateProperties::BellAttachment::Floor:
                    m_shapesByState[index] = (axis == Axis::X) ? m_floorShapeX : m_floorShapeZ;
                    break;
                case BlockStateProperties::BellAttachment::Ceiling:
                    m_shapesByState[index] = m_ceilingShape;
                    break;
                case BlockStateProperties::BellAttachment::SingleWall:
                    switch (facing) {
                        case Direction::North:
                            m_shapesByState[index] = m_singleWallShapeNorth;
                            break;
                        case Direction::South:
                            m_shapesByState[index] = m_singleWallShapeSouth;
                            break;
                        case Direction::East:
                            m_shapesByState[index] = m_singleWallShapeEast;
                            break;
                        case Direction::West:
                            m_shapesByState[index] = m_singleWallShapeWest;
                            break;
                        default:
                            m_shapesByState[index] = m_singleWallShapeNorth;
                            break;
                    }
                    break;
                case BlockStateProperties::BellAttachment::DoubleWall:
                    m_shapesByState[index] = (axis == Axis::X) ? m_doubleWallShapeX : m_doubleWallShapeZ;
                    break;
            }
        }
    }
}

size_t BellBlock::_shapeIndex(const BlockState& state)
{
    // HORIZONTAL_FACING 的索引：通过 Directions::horizontal 顺序
    // 由于 DirectionProperty::createHorizontal 使用 {North, East, South, West} 顺序，
    // 属性内部索引为 North=0, East=1, South=2, West=3
    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    size_t facingIdx = 0;
    switch (facing) {
        case Direction::North:
            facingIdx = 0;
            break;
        case Direction::East:
            facingIdx = 1;
            break;
        case Direction::South:
            facingIdx = 2;
            break;
        case Direction::West:
            facingIdx = 3;
            break;
        default:
            facingIdx = 0;
            break;
    }

    const BlockStateProperties::BellAttachment attachment = state.get(BlockStateProperties::BELL_ATTACHMENT());
    const size_t attachIdx = static_cast<size_t>(attachment);

    return (facingIdx << 2) | attachIdx;
}

// ============================================================================
// 状态放置与更新
// ============================================================================

BlockState BellBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 参考: net.minecraft.world.level.block.BellBlock#getStateForPlacement
    // 与 MC 1.21.11 一致：
    // - Y 轴点击：FLOOR（顶面）或 CEILING（底面），通过 canSurvive 判定
    //   canSurvive 中 CEILING 走 Block.canSupportCenter(world, pos.above(), DOWN) 中心支撑判定
    //   FLOOR 与 WALL 走 FaceAttachedHorizontalDirectionalBlock.canAttach（基于 isFaceSturdy(FULL)）
    // - 侧面点击：优先双面墙，回退到单面墙；若仍不可存活则尝试 FLOOR，再回退 CEILING
    const Direction clickedFace = context.getClickedFace();
    const BlockPos clickedPos = context.placementPos();
    IWorld& world = context.getWorld();

    const Axis axis = Directions::getAxis(clickedFace);

    if (axis == Axis::Y) {
        // 点击顶面或底面
        const bool isFloor = (clickedFace == Direction::Up);
        BlockState state = defaultState()
                               .with(BlockStateProperties::BELL_ATTACHMENT(),
                                   isFloor ? BlockStateProperties::BellAttachment::Floor
                                           : BlockStateProperties::BellAttachment::Ceiling)
                               .with(BlockStateProperties::HORIZONTAL_FACING(), context.horizontalDirection());
        // canSurvive 判定：
        // - FLOOR: canAttach(world, pos, DOWN) = isFaceSturdy(world, pos.below(), UP, FULL)
        // - CEILING: canSupportCenter(world, pos.above(), DOWN)
        const BlockPos supportPos = clickedPos.offset(isFloor ? Direction::Down : Direction::Up);
        const Direction supportDir = isFloor ? Direction::Up : Direction::Down;
        const bool canSurvive = isFloor ? Block::hasEnoughSolidSide(world, supportPos, supportDir)
                                        : Block::canSupportCenter(world, supportPos, supportDir);
        if (canSurvive) {
            return state;
        }
    } else {
        // 点击侧面：检测是否可形成双面墙
        // 点击方向的反方向是支撑墙所在方向
        const Direction supportDir = Directions::opposite(clickedFace);
        const bool isDoubleWall = [axis, &world, &clickedPos]() {
            if (axis == Axis::X) {
                return Block::hasEnoughSolidSide(world, clickedPos.offset(Direction::West), Direction::East) &&
                    Block::hasEnoughSolidSide(world, clickedPos.offset(Direction::East), Direction::West);
            }
            // Z 轴
            return Block::hasEnoughSolidSide(world, clickedPos.offset(Direction::North), Direction::South) &&
                Block::hasEnoughSolidSide(world, clickedPos.offset(Direction::South), Direction::North);
        }();

        BlockState state = defaultState()
                               .with(BlockStateProperties::HORIZONTAL_FACING(), supportDir)
                               .with(BlockStateProperties::BELL_ATTACHMENT(),
                                   isDoubleWall ? BlockStateProperties::BellAttachment::DoubleWall
                                                : BlockStateProperties::BellAttachment::SingleWall);
        // canSurvive: 检查支撑墙（clickedFace 反方向）是否有朝向钟的固体面
        if (Block::hasEnoughSolidSide(world, clickedPos.offset(supportDir), clickedFace)) {
            return state;
        }

        // 单面墙不行，尝试地面或天花板
        const bool floorOk = Block::hasEnoughSolidSide(world, clickedPos.offset(Direction::Down), Direction::Up);
        state = state.with(BlockStateProperties::BELL_ATTACHMENT(),
            floorOk ? BlockStateProperties::BellAttachment::Floor : BlockStateProperties::BellAttachment::Ceiling);
        // canSurvive 判定（FLOOR 走 hasEnoughSolidSide，CEILING 走 canSupportCenter）
        const BlockPos fallbackPos = clickedPos.offset(floorOk ? Direction::Down : Direction::Up);
        const Direction fallbackDir = floorOk ? Direction::Up : Direction::Down;
        const bool fallbackOk = floorOk ? Block::hasEnoughSolidSide(world, fallbackPos, fallbackDir)
                                        : Block::canSupportCenter(world, fallbackPos, fallbackDir);
        if (fallbackOk) {
            return state;
        }
    }

    // 所有附着方式都不可用，返回默认状态（Floor 朝北）
    return defaultState();
}

BlockState BellBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(currentPos);

    // 参考: net.minecraft.world.level.block.BellBlock#updateShape
    // getConnectedDirection(state): FLOOR→UP, CEILING→DOWN, WALL→FACING.opposite
    // direction = getConnectedDirection(state).opposite(): FLOOR→DOWN, CEILING→UP, WALL→FACING
    // canSurvive 检查 direction 方向的支撑方块
    const BlockStateProperties::BellAttachment attachment = state.get(BlockStateProperties::BELL_ATTACHMENT());
    const Direction bellFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    const Direction connectedDir = [attachment, bellFacing]() -> Direction {
        switch (attachment) {
            case BlockStateProperties::BellAttachment::Floor:
                return Direction::Up;
            case BlockStateProperties::BellAttachment::Ceiling:
                return Direction::Down;
            case BlockStateProperties::BellAttachment::SingleWall:
            case BlockStateProperties::BellAttachment::DoubleWall:
                return Directions::opposite(bellFacing);
            default:
                return Direction::Up;
        }
    }();
    // direction = connectedDir.opposite，即支撑方块所在方向
    const Direction supportDir = Directions::opposite(connectedDir);

    // 支撑失效且不是双面墙：变为空气
    // 参考: direction == p_49745_ && !canSurvive && bellattachtype != DOUBLE_WALL
    if (facing == supportDir && attachment != BlockStateProperties::BellAttachment::DoubleWall) {
        // canSurvive 判定：
        // - CEILING: canSupportCenter(world, facingPos, DOWN)
        // - FLOOR/WALL: hasEnoughSolidSide(world, facingPos, connectedDir)（等价 isFaceSturdy(FULL)）
        const bool canSurvive = (attachment == BlockStateProperties::BellAttachment::Ceiling)
            ? Block::canSupportCenter(world, facingPos, connectedDir)
            : Block::hasEnoughSolidSide(world, facingPos, connectedDir);
        if (!canSurvive) {
            return VanillaBlocks::AIR->defaultState();
        }
    }

    // 朝向轴上的更新：处理 SingleWall ↔ DoubleWall 转换
    if (Directions::getAxis(facing) == Directions::getAxis(bellFacing)) {
        // DoubleWall → SingleWall：支撑失效（一侧墙消失）
        // 参考: bellattachtype == DOUBLE_WALL && !facingState.isFaceSturdy(world, facingPos, facing)
        if (attachment == BlockStateProperties::BellAttachment::DoubleWall &&
            !Block::hasEnoughSolidSide(world, facingPos, facing)) {
            return state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::SingleWall)
                .with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(facing));
        }

        // SingleWall → DoubleWall：另一侧出现支撑
        // 参考: bellattachtype == SINGLE_WALL && direction.opposite() == p_49745_
        //       && facingState.isFaceSturdy(world, facingPos, FACING)
        // 其中 direction = FACING，所以 direction.opposite() = FACING.opposite
        if (attachment == BlockStateProperties::BellAttachment::SingleWall &&
            Directions::opposite(bellFacing) == facing && Block::hasEnoughSolidSide(world, facingPos, bellFacing)) {
            return state.with(
                BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::DoubleWall);
        }
    }

    return state;
}

// ============================================================================
// 旋转
// ============================================================================

const BlockState& BellBlock::rotate(const BlockState& state, Rotation rotation) const
{
    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    const Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& BellBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }
    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    const Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

// ============================================================================
// 形状
// ============================================================================

const CollisionShape& BellBlock::getShape(const BlockState& state) const
{
    const size_t index = _shapeIndex(state);
    return m_shapesByState[index];
}

// ============================================================================
// 方块实体
// ============================================================================

std::unique_ptr<BlockEntity> BellBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::BellBlockEntity>(pos);
}

// ============================================================================
// 交互
// ============================================================================

BlockActionResult BellBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hand);

    // 参考: net.minecraft.world.level.block.BellBlock#useWithoutItem
    if (world.isClientSide()) {
        // 客户端：等待服务端同步，返回 Success 让客户端知道交互已处理
        return ActionResultType::Success;
    }

    if (onHit(world, state, hit, &player, true)) {
        return ActionResultType::Success;
    }
    return ActionResultType::Pass;
}

void BellBlock::onProjectileHit(
    IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile)
{
    // 参考: net.minecraft.world.level.block.BellBlock#onProjectileHit
    Player* player = nullptr;
    auto* projectileEntity = dynamic_cast<entity::ProjectileEntity*>(&projectile);
    if (projectileEntity != nullptr) {
        Entity* shooter = projectileEntity->getShooter();
        if (shooter != nullptr) {
            player = dynamic_cast<Player*>(shooter);
        }
    }

    onHit(world, state, hitResult, player, true);
}

void BellBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 参考: net.minecraft.world.level.block.BellBlock#neighborChanged
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return;
    }

    const bool shouldPower = world::redstone::RedstonePower::isPowered(world, pos);
    const bool isPowered = state->get(BlockStateProperties::POWERED());

    if (shouldPower != isPowered) {
        if (shouldPower) {
            // 红石激活时自动敲响（无方向参数，使用方块朝向）
            attemptToRing(world, pos, state->get(BlockStateProperties::HORIZONTAL_FACING()));
        }
        BlockState newState = state->with(BlockStateProperties::POWERED(), shouldPower);
        world.setBlockState(pos, &newState, 3);
    }
}

// ============================================================================
// 敲钟接口
// ============================================================================

bool BellBlock::attemptToRing(IWorld& world, const BlockPos& pos, Direction direction)
{
    return attemptToRing(nullptr, world, pos, direction);
}

bool BellBlock::attemptToRing(Player* player, IWorld& world, const BlockPos& pos, Direction direction)
{
    // 参考: net.minecraft.world.level.block.BellBlock#attemptToRing
    MC_UNUSED(player);

    if (world.isClientSide()) {
        return false;
    }

    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::Bell) {
        return false;
    }

    auto* bellEntity = static_cast<blockentity::BellBlockEntity*>(blockEntity);
    bellEntity->onHit(world, direction);

    // 播放钟声（音量 2.0，音调 1.0）
    world.playSound(SoundEvents::BLOCK_BELL_USE, sound::SoundCategory::Blocks, pos.center(), 2.0f, 1.0f);

    // 触发 BELL_HIT 游戏事件（通知附近幽匿感测体）
    // 注意：MC Java 使用 BLOCK_CHANGE，但语义上是方块激活；本项目统一使用 BLOCK_ACTIVATE
    world.gameEvent(gameevent::GameEvents::BLOCK_ACTIVATE, pos, static_cast<const BlockState*>(nullptr));

    return true;
}

bool BellBlock::onHit(
    IWorld& world, const BlockState& state, const BlockRaycastResult& hit, Player* player, bool isProjectile)
{
    // 参考: net.minecraft.world.level.block.BellBlock#onHit, isProperHit
    const Direction direction = hit.face();
    const BlockPos blockPos = hit.blockPos();
    const Vector3 hitLocation = hit.hitPosition();
    const f64 hitY = static_cast<f64>(hitLocation.y) - static_cast<f64>(blockPos.y);

    const bool isProper = !isProjectile || _isProperHit(state, direction, hitY);
    if (!isProper) {
        return false;
    }

    if (attemptToRing(player, world, blockPos, direction) && player != nullptr) {
        player->awardCustomStat(ResourceLocation(stats::BELL_RING), 1);
    }
    return true;
}

bool BellBlock::_isProperHit(const BlockState& state, Direction direction, f64 hitY)
{
    // 参考: net.minecraft.world.level.block.BellBlock#isProperHit
    // 若点击方向是 Y 轴，或点击位置 Y > 0.8124，则不是有效敲击
    if (Directions::getAxis(direction) == Axis::Y || hitY > 0.8124) {
        return false;
    }

    const Direction bellFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    const BlockStateProperties::BellAttachment attachment = state.get(BlockStateProperties::BELL_ATTACHMENT());

    switch (attachment) {
        case BlockStateProperties::BellAttachment::Floor:
            return Directions::getAxis(bellFacing) == Directions::getAxis(direction);
        case BlockStateProperties::BellAttachment::SingleWall:
        case BlockStateProperties::BellAttachment::DoubleWall:
            return Directions::getAxis(bellFacing) != Directions::getAxis(direction);
        case BlockStateProperties::BellAttachment::Ceiling:
            return true;
        default:
            return false;
    }
}

} // namespace blocks
} // namespace mc
