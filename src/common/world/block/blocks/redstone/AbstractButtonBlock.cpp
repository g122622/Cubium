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

#include "AbstractButtonBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/SupportType.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <array>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// 使用 BlockStateProperties 中的 AttachFace
using AttachFace = BlockStateProperties::AttachFace;
using entity::AbstractArrowEntity;

AbstractButtonBlock::AbstractButtonBlock(const BlockProperties& properties, i32 ticksToStayPressed, bool arrowSensitive)
    : Block(properties)
    , m_ticksToStayPressed(ticksToStayPressed)
    , m_arrowSensitive(arrowSensitive)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::ATTACH_FACE())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::ATTACH_FACE(), AttachFace::Wall));
}

BlockState AbstractButtonBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 朝向与附着面由玩家点击面决定（附墙方块语义，与朝玩家视线的 dispenser/piston 不同）：
    //   点击顶面 → ATTACH_FACE=Floor，facing=玩家水平朝向（horizontalDirection，仅 yaw）；
    //   点击底面 → ATTACH_FACE=Ceiling，facing=玩家水平朝向；
    //   点击墙面 → ATTACH_FACE=Wall，facing=点击面（水平四向）。
    // 此前未重写该方法，落回基类 Block::getStateForPlacement 返回 defaultState()（HORIZONTAL_FACING 恒
    // North、ATTACH_FACE 恒 Wall），与预期按点击面决定朝向/附着面的行为不一致。重写后修正。
    // 石按钮/木按钮继承本类，继承本方法自动获得正确朝向。
    Direction clickedFace = context.getClickedFace();
    Direction horizontalFacing = context.horizontalDirection();

    AttachFace attachFace;
    Direction finalFacing = horizontalFacing;

    if (clickedFace == Direction::Up) {
        attachFace = AttachFace::Floor;
    } else if (clickedFace == Direction::Down) {
        attachFace = AttachFace::Ceiling;
    } else {
        attachFace = AttachFace::Wall;
        finalFacing = clickedFace;
    }

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), finalFacing)
        .with(BlockStateProperties::ATTACH_FACE(), attachFace);
}

bool AbstractButtonBlock::isPowered(const BlockState& state) noexcept
{
    return state.get(BlockStateProperties::POWERED());
}

BlockState AbstractButtonBlock::withPowered(BlockState state, bool powered) noexcept
{
    return state.with(BlockStateProperties::POWERED(), powered);
}

Direction AbstractButtonBlock::getFacing(const BlockState& state) noexcept
{
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

void AbstractButtonBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 按钮放置时不触发信号
}

void AbstractButtonBlock::_updateNeighbours(IWorld& world, const BlockPos& pos)
{
    // 手柄输出方向反向即附着（支撑）方向：方块型按钮为 Up（下方支撑），贴墙按钮为 FACING 反向。
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return;
    }
    const Direction supportDirection = Directions::opposite(getAttachDirection(*state));

    // 按钮状态变化要使自身六个方向邻居重算形状，并额外通知支撑方块本身，
    // 覆盖紧贴支撑的红石导线等元件。与按下/弹起时的 setBlockState(UPDATE_CLIENTS)
    // 分开执行：该次写入只同步客户端，邻居通知统一在此完成。
    world.updateNeighbors(pos, *this);
    world.updateNeighbors(pos.offset(supportDirection), *this);
}

BlockState AbstractButtonBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    // 对齐 vanilla FaceAttachedHorizontalDirectionalBlock.updateShape：
    //   getConnectedDirection(state).getOpposite() == direction && !canSurvive => AIR
    // getConnectedDirection（vanilla）：CEILING→Down, FLOOR→Up, WALL→FACING（朝向/输出方向）。
    // 其 opposite = 支撑方向。仅当邻居变化方向 == 支撑方向时检查 canSurvive。
    // canSurvive = 支撑方块 isFaceSturdy；项目用 isAir 判定支撑缺失（简化）。
    // 此方法由 setBlockState 邻居循环调用，受 UPDATE_NEIGHBORS 门控；结构放置不带该位，
    // 故不触发，避免按钮在支撑未放置时自毁。
    const AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());
    Direction connectedDir;
    switch (attachFace) {
        case AttachFace::Ceiling:
            connectedDir = Direction::Down;
            break;
        case AttachFace::Floor:
            connectedDir = Direction::Up;
            break;
        case AttachFace::Wall:
        default:
            connectedDir = getFacing(state);
            break;
    }

    const Direction supportDir = Directions::opposite(connectedDir);
    if (supportDir != facing) {
        return state;
    }

    const BlockPos supportPos = currentPos.offset(supportDir);
    const BlockState* supportState = world.getBlockState(supportPos);
    if (!supportState || supportState->isAir()) {
        return *BlockRegistry::instance().airState();
    }

    return state;
}

void AbstractButtonBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    if (!isPowered(state)) {
        return;
    }

    // 计时到期：重查是否仍有箭矢停留在按钮内。有则维持按下并续期（木按钮可被箭持续触发），
    // 无则弹起。与实体碰撞入口共用 checkPressed，避免两条路径行为分叉。
    checkPressed(world, pos, state);
}

BlockActionResult AbstractButtonBlock::useWithoutItem(
    const BlockState& state, IWorld& world, const BlockPos& pos, Player& player, const BlockRaycastResult& hit)
{
    MC_UNUSED(player);
    MC_UNUSED(hit);

    // 已按下的按钮不再响应，避免重复调度与重复音效。
    if (isPowered(state)) {
        return ActionResultType::Consume;
    }
    if (!world.isClientSide()) {
        press(world, pos, state);
    }
    return ActionResultType::Success;
}

void AbstractButtonBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 仅木按钮可被箭矢触发；已按下的续期由计划刻负责。
    if (!m_arrowSensitive || world.isClientSide() || isPowered(state)) {
        return;
    }
    if (dynamic_cast<const AbstractArrowEntity*>(&entity) == nullptr) {
        return;
    }
    // onEntityCollision 为 const 回调，而状态写入需要非 const 实例（与 updatePostPlacement 同语义）。
    const_cast<AbstractButtonBlock*>(this)->checkPressed(world, pos, state);
}

/// 判断包围盒内是否停留着（未被移除的）箭矢
bool _containsArrow(const IWorld& world, const AxisAlignedBB& box)
{
    for (const Entity* candidate : world.getEntitiesInAABB(box)) {
        if (candidate == nullptr || candidate->isRemoved()) {
            continue;
        }
        if (dynamic_cast<const AbstractArrowEntity*>(candidate) != nullptr) {
            return true;
        }
    }
    return false;
}

void AbstractButtonBlock::checkPressed(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    bool arrowInside = false;
    if (m_arrowSensitive) {
        // 以按钮形状所有局部盒的并集判定"箭矢停留在按钮内"：按钮形状单盒，取首个盒即可，
        // 静止在地面的箭矢同样算作持续按下。
        const CollisionShape& shape = getShape(state);
        if (shape.boxCount() > 0) {
            const AxisAlignedBB local = shape.boxes().front();
            const AxisAlignedBB box = AxisAlignedBB(static_cast<f32>(pos.x) + local.minX,
                static_cast<f32>(pos.y) + local.minY,
                static_cast<f32>(pos.z) + local.minZ,
                static_cast<f32>(pos.x) + local.maxX,
                static_cast<f32>(pos.y) + local.maxY,
                static_cast<f32>(pos.z) + local.maxZ);
            arrowInside = _containsArrow(world, box);
        }
    }

    if (arrowInside != isPowered(state)) {
        BlockState newState = withPowered(state, arrowInside);
        world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_CLIENTS);
        _updateNeighbours(world, pos);
        playClickSound(world, pos, arrowInside);
    }

    // 仍处于按下状态则续期，下一个计划刻再复查；否则本次即为弹起。
    if (arrowInside) {
        world.tickManager().scheduleBlockTick(pos, *this, m_ticksToStayPressed, world::tick::TickPriority::High);
    }
}

namespace {

/// 按钮按下时的弱信号强度（原版按钮按下输出 15）
constexpr i32 BUTTON_SIGNAL_POWER = 15;

/// 判断查询方向是否与按钮输出方向一致
bool _isSignalSide(const BlockState& state, Direction side)
{
    return side != Direction::None && side == AbstractButtonBlock::getOutputDirection(state);
}

} // namespace

Direction AbstractButtonBlock::getAttachDirection(const BlockState& state) noexcept
{
    const AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());
    switch (attachFace) {
        case AttachFace::Ceiling:
            return Direction::Up;
        case AttachFace::Floor:
            return Direction::Down;
        case AttachFace::Wall:
        default:
            return getFacing(state);
    }
}

Direction AbstractButtonBlock::getOutputDirection(const BlockState& state) noexcept
{
    return Directions::opposite(getAttachDirection(state));
}

i32 AbstractButtonBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 对齐 vanilla 1.21.11 ButtonBlock.getSignal：按下即输出 15，**不区分方向**。
    //
    // 这里曾经加了「仅向附着面相反方向供电」的方向限制，导致红石线永远取不到按钮信号：
    // SignalGetter.getBestNeighborSignal 查询邻居时传的是「从查询点指向邻居」的方向
    // （RedstonePower::getRedstonePowerFromNeighbors 也是这么传的），与按钮的
    // getOutputDirection 并不是同一个朝向，判等必然失败。表现就是按钮按下后
    // powered=true、命令方块也收到了 neighborChanged，但红石线功率恒为 0。
    //
    // 方向限制属于**强信号**的语义（vanilla getDirectSignal 才比较
    // getConnectedDirection(state) == side），弱信号不该有。
    if (!isPowered(state)) {
        return 0;
    }
    return BUTTON_SIGNAL_POWER;
}

i32 AbstractButtonBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 对齐 vanilla 1.21.11 ButtonBlock.getDirectSignal：只有「输出方向」能拿到强信号，
    // 用于强充能附着面反向的导体。其余方向为 0。
    if (!isPowered(state) || !_isSignalSide(state, side)) {
        return 0;
    }
    return BUTTON_SIGNAL_POWER;
}

const CollisionShape& AbstractButtonBlock::getShape(const BlockState& state) const
{
    // 采用原版按钮的大致体素尺寸：墙面 6x4 像素，地板/天花板 6x6 像素。
    static const CollisionShape floorUnpressed = CollisionShape::fromPixelBox(5.0f, 0.0f, 5.0f, 11.0f, 2.0f, 11.0f);
    static const CollisionShape floorPressed = CollisionShape::fromPixelBox(5.0f, 0.0f, 5.0f, 11.0f, 1.0f, 11.0f);
    static const CollisionShape ceilingUnpressed = CollisionShape::fromPixelBox(5.0f, 14.0f, 5.0f, 11.0f, 16.0f, 11.0f);
    static const CollisionShape ceilingPressed = CollisionShape::fromPixelBox(5.0f, 15.0f, 5.0f, 11.0f, 16.0f, 11.0f);

    static const CollisionShape wallNorthUnpressed =
        CollisionShape::fromPixelBox(5.0f, 6.0f, 14.0f, 11.0f, 10.0f, 16.0f);
    static const CollisionShape wallNorthPressed = CollisionShape::fromPixelBox(5.0f, 6.0f, 15.0f, 11.0f, 10.0f, 16.0f);
    static const CollisionShape wallSouthUnpressed = CollisionShape::fromPixelBox(5.0f, 6.0f, 0.0f, 11.0f, 10.0f, 2.0f);
    static const CollisionShape wallSouthPressed = CollisionShape::fromPixelBox(5.0f, 6.0f, 0.0f, 11.0f, 10.0f, 1.0f);
    static const CollisionShape wallWestUnpressed =
        CollisionShape::fromPixelBox(14.0f, 6.0f, 5.0f, 16.0f, 10.0f, 11.0f);
    static const CollisionShape wallWestPressed = CollisionShape::fromPixelBox(15.0f, 6.0f, 5.0f, 16.0f, 10.0f, 11.0f);
    static const CollisionShape wallEastUnpressed = CollisionShape::fromPixelBox(0.0f, 6.0f, 5.0f, 2.0f, 10.0f, 11.0f);
    static const CollisionShape wallEastPressed = CollisionShape::fromPixelBox(0.0f, 6.0f, 5.0f, 1.0f, 10.0f, 11.0f);

    const bool powered = isPowered(state);
    const AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());
    const Direction facing = getFacing(state);

    if (attachFace == AttachFace::Floor) {
        return powered ? floorPressed : floorUnpressed;
    }
    if (attachFace == AttachFace::Ceiling) {
        return powered ? ceilingPressed : ceilingUnpressed;
    }

    switch (facing) {
        case Direction::North:
            return powered ? wallNorthPressed : wallNorthUnpressed;
        case Direction::South:
            return powered ? wallSouthPressed : wallSouthUnpressed;
        case Direction::West:
            return powered ? wallWestPressed : wallWestUnpressed;
        case Direction::East:
            return powered ? wallEastPressed : wallEastUnpressed;
        default:
            return powered ? wallNorthPressed : wallNorthUnpressed;
    }
}

void AbstractButtonBlock::press(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 如果已经按下，不重复触发
    if (isPowered(state)) {
        return;
    }

    // 按下按钮
    BlockState newState = withPowered(state, true);
    world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_CLIENTS);

    // 播放按下音效
    playClickSound(world, pos, true);

    // 通知相邻方块更新
    _updateNeighbours(world, pos);

    // 调度弹起
    world.tickManager().scheduleBlockTick(pos, *this, m_ticksToStayPressed, world::tick::TickPriority::High);
}

bool AbstractButtonBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 对应原版 FaceAttachedHorizontalDirectionalBlock#canSurvive：
    //   canAttach(world, pos, getConnectedDirection(state).getOpposite())
    //
    // `getConnectedDirection` 返回「从方块指向附着面」的方向（WALL 时就是 FACING），
    // 取反之后才是「从方块指向支撑方块」的方向。`getAttachDirection` 给的正是
    // `getConnectedDirection`，因此这里必须再 `opposite` 一次。
    //
    // 此前缺了那次取反，按钮去检查的是**背面**的方块：facing=north 的按钮本该贴在南边
    // 的支撑上，却去问北边谁坚固。表现在差分能力 block_support 上，就是「贴在台阶上的
    // 按钮」全部判错——台阶南北两侧的坚固性不同，方向一反就对不上。
    const Direction connected = getAttachDirection(state);
    const BlockPos supportPos = pos.offset(Directions::opposite(connected));
    const BlockState* support = world.getBlockState(supportPos.x, supportPos.y, supportPos.z);
    if (support == nullptr) {
        return false;
    }
    // 附着面 = 支撑方块朝向按钮的那一面，即 connected 本身。
    return support->isFaceSturdy(static_cast<IWorld&>(world), supportPos, connected, SupportType::Full);
}

bool AbstractButtonBlock::canAttachToFace(Direction facing) const
{
    // 按钮可以附着在任何水平方向
    return Directions::isHorizontal(facing);
}

} // namespace blocks
} // namespace mc
