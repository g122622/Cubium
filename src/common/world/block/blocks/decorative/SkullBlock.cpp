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

#include "SkullBlock.hpp"

#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/boss/WitherEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/SupportType.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/interactive/SkullBlockEntity.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/registry/VanillaBlocks.hpp"
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

namespace {

/// 站立头颅碰撞形状：中心 8x8 像素、高 8 像素
const CollisionShape& _skullShape()
{
    static const CollisionShape shape = CollisionShape::fromPixelBox(4.0f, 0.0f, 4.0f, 12.0f, 8.0f, 12.0f);
    return shape;
}

/// 猪灵站立头颅碰撞形状：中心 10x8 像素、高 8 像素
const CollisionShape& _piglinSkullShape()
{
    static const CollisionShape shape = CollisionShape::fromPixelBox(3.0f, 0.0f, 4.0f, 13.0f, 8.0f, 12.0f);
    return shape;
}

/// 构造墙壁头颅的形状表（按 North/South/West/East 顺序，占据靠墙一侧 8/16 深度）
void _buildWallShapes(CollisionShape out[4], f32 widthPixels)
{
    // vanilla WallSkullBlock: Shapes.rotateHorizontal(Block.boxZ(8.0, 8.0, 16.0))
    //   boxZ(8.0, 8.0, 16.0) 表示 X 宽 8 居中、Y[8,16]、Z[0,8]
    // 本项目 CollisionShape 用方块本地 0-1 坐标，故 /16。
    const f32 half = widthPixels / 2.0f / 16.0f;
    const f32 cx = 0.5f;
    // North: Z 靠 0 侧（贴北墙）；South: Z 靠 1 侧；West: X 靠 0 侧；East: X 靠 1 侧
    out[0] = CollisionShape::box(cx - half, 8.0f / 16.0f, 0.0f, cx + half, 1.0f, 0.5f); // North
    out[1] = CollisionShape::box(cx - half, 8.0f / 16.0f, 0.5f, cx + half, 1.0f, 1.0f); // South
    out[2] = CollisionShape::box(0.0f, 8.0f / 16.0f, cx - half, 0.5f, 1.0f, cx + half); // West
    out[3] = CollisionShape::box(0.5f, 8.0f / 16.0f, cx - half, 1.0f, 1.0f, cx + half); // East
}

/// 方向到形状表下标（水平方向）
i32 _facingIndex(Direction facing)
{
    switch (facing) {
        case Direction::North:
            return 0;
        case Direction::South:
            return 1;
        case Direction::West:
            return 2;
        case Direction::East:
            return 3;
        default:
            return 0;
    }
}

/// 判断方块状态是否为凋灵骷髅头颅（站立或墙挂）
bool _isWitherSkeletonSkull(const BlockState* state)
{
    return state != nullptr &&
        (state->is(VanillaBlocks::WITHER_SKELETON_SKULL) || state->is(VanillaBlocks::WITHER_SKELETON_WALL_SKULL));
}

/// 判断方块状态是否为凋灵召唤基座方块（灵魂沙/灵魂土）
bool _isWitherBaseBlock(const BlockState* state)
{
    return state != nullptr && BlockTags::WITHER_SUMMON_BASE_BLOCKS().contains(*state);
}

/// 判断方块状态是否为空气
bool _isAirState(const BlockState* state)
{
    return state != nullptr && state->isAir();
}

/**
 * @brief 匹配凋灵 T 形图案（vanilla witherPatternFull）
 *
 * 图案（一个 3x3 局部平面，letters aisle "^^^"/"###"/"~#~"）：
 * ```text
 *   ^  ^  ^    三个凋灵骷髅头颅（顶层）
 *   #  #  #    三个基座方块（中层）
 *   ~  #  ~    顶层两侧为空气、中央为基座（底层）
 * ```
 * 图案可绕 Y 轴以 90 度旋转（X 轴或 Z 轴朝向）。命中时输出顶点头颅坐标与朝向轴。
 *
 * @param world 世界
 * @param pos 新放置的头颅位置（作为匹配起点）
 * @param outSkullPos [出参] 顶层中央头颅坐标
 * @param outForwardsAxisIsX [出参] 图案前行轴是否为 X 轴（true=X，false=Z）
 * @return 是否命中
 */
bool _matchWitherPattern(IWorld& world, const BlockPos& pos, BlockPos& outSkullPos, bool& outForwardsAxisIsX)
{
    // 图案的“前行”方向（从顶点头颅指向同排另一个头颅）可为 X 轴（east/west）或 Z 轴（north/south）。
    // 以候选顶点头颅为中心，分别沿两个轴向展开 3x3 检查。
    for (bool forwardsAxisIsX : {true, false}) {
        const BlockPos dx = forwardsAxisIsX ? BlockPos(1, 0, 0) : BlockPos(0, 0, 1);
        // pos 可能是顶层 3 个头颅中的任意一个：center  ∈ {pos-dx, pos, pos+dx}
        // （pos 为右端头颅时 center=pos-dx；pos 为中心时 center=pos；pos 为左端时 center=pos+dx）。
        for (i32 headOffset = -1; headOffset <= 1; ++headOffset) {
            const BlockPos center = BlockPos(pos.x + dx.x * headOffset, pos.y, pos.z + dx.z * headOffset);
            const BlockPos left = BlockPos(center.x - dx.x, center.y, center.z - dx.z);
            const BlockPos right = BlockPos(center.x + dx.x, center.y, center.z + dx.z);

            // 顶层：三个头颅
            if (!_isWitherSkeletonSkull(world.getBlockState(left)) ||
                !_isWitherSkeletonSkull(world.getBlockState(center)) ||
                !_isWitherSkeletonSkull(world.getBlockState(right))) {
                continue;
            }

            // 中层：三个基座方块
            const bool midOk = _isWitherBaseBlock(world.getBlockState(left.down())) &&
                _isWitherBaseBlock(world.getBlockState(center.down())) &&
                _isWitherBaseBlock(world.getBlockState(right.down()));
            if (!midOk) {
                continue;
            }

            // 底层：两端空气、中央基座
            const bool botOk = _isAirState(world.getBlockState(left.down(2))) &&
                _isWitherBaseBlock(world.getBlockState(center.down(2))) &&
                _isAirState(world.getBlockState(right.down(2)));
            if (!botOk) {
                continue;
            }

            outSkullPos = center;
            outForwardsAxisIsX = forwardsAxisIsX;
            return true;
        }
    }
    return false;
}

/// 清除凋灵图案涉及的全部方块（3 头颅 + 3 中层基座 + 1 底层中央基座 = 7 个方块）。
/// 仅沿命中时确定的图案轴向清除，避免误清垂直于该轴的邻近结构。
void _clearWitherPattern(IWorld& world, const BlockPos& centerSkullPos, bool forwardsAxisIsX)
{
    const BlockState* airState = BlockRegistry::instance().airState();
    if (airState == nullptr) {
        return;
    }

    const BlockPos dx = forwardsAxisIsX ? BlockPos(1, 0, 0) : BlockPos(0, 0, 1);
    for (i32 i = -1; i <= 1; ++i) {
        const BlockPos top(centerSkullPos.x + dx.x * i, centerSkullPos.y, centerSkullPos.z + dx.z * i);
        world.setBlockState(top, airState, world::BlockUpdateFlags::UPDATE_CLIENTS);
        world.setBlockState(top.down(), airState, world::BlockUpdateFlags::UPDATE_CLIENTS);
    }
    // 底层中央基座
    world.setBlockState(BlockPos(centerSkullPos.x, centerSkullPos.y - 2, centerSkullPos.z),
        airState,
        world::BlockUpdateFlags::UPDATE_CLIENTS);
}

} // namespace

// ============================================================================
// AbstractSkullBlock
// ============================================================================

AbstractSkullBlock::AbstractSkullBlock(SkullType type, const BlockProperties& properties)
    : Block(properties)
    , m_type(type)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::POWERED())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::POWERED(), false));
}

std::unique_ptr<BlockEntity> AbstractSkullBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::SkullBlockEntity>(pos);
}

BlockState AbstractSkullBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState().with(BlockStateProperties::POWERED(),
        world::redstone::RedstonePower::isPowered(context.getWorld(), context.placementPos()));
}

void AbstractSkullBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    if (world.isClientSide()) {
        return;
    }

    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return;
    }

    const bool powered = world::redstone::RedstonePower::isPowered(world, pos);
    if (powered != state->get(BlockStateProperties::POWERED())) {
        BlockState newState = state->with(BlockStateProperties::POWERED(), powered);
        world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_CLIENTS);
    }
}

// ============================================================================
// SkullBlock（站立）
// ============================================================================

SkullBlock::SkullBlock(SkullType type, const BlockProperties& properties)
    : AbstractSkullBlock(type, properties)
    , m_shape(_skullShape())
    , m_piglinShape(_piglinSkullShape())
{
    // 站立头颅额外持有 ROTATION_0_15 属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::ROTATION_0_15())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(
        defaultState().with(BlockStateProperties::POWERED(), false).with(BlockStateProperties::ROTATION_0_15(), 0));
}

const CollisionShape& SkullBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return (m_type == SkullType::Piglin) ? m_piglinShape : m_shape;
}

BlockState SkullBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 旋转值：根据玩家 yaw 换算为 0-15（每 22.5 度），对齐 vanilla RotationSegment.convertToSegment
    i32 rotation = static_cast<i32>(std::floor((180.0f + context.getPlayerYaw()) * 16.0f / 360.0f + 0.5f)) & 15;

    bool powered = world::redstone::RedstonePower::isPowered(context.getWorld(), context.placementPos());
    return defaultState()
        .with(BlockStateProperties::POWERED(), powered)
        .with(BlockStateProperties::ROTATION_0_15(), rotation);
}

const BlockState& SkullBlock::rotate(const BlockState& state, Rotation rotation) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::rotateRotation(currentRotation, rotation, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

const BlockState& SkullBlock::mirror(const BlockState& state, Mirror mirror) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::mirrorRotation(currentRotation, mirror, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

// ============================================================================
// WallSkullBlock（贴墙）
// ============================================================================

WallSkullBlock::WallSkullBlock(SkullType type, const BlockProperties& properties)
    : AbstractSkullBlock(type, properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));

    _buildWallShapes(m_shapes, 8.0f);
}

const CollisionShape& WallSkullBlock::_shapeForFacing(Direction facing) const
{
    return m_shapes[_facingIndex(facing)];
}

const CollisionShape& WallSkullBlock::getShape(const BlockState& state) const
{
    return _shapeForFacing(state.get(BlockStateProperties::HORIZONTAL_FACING()));
}

BlockState WallSkullBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 对齐 vanilla WallSkullBlock.getStateForPlacement：遍历最近视线方向，取首个水平方向的
    // 反方向作为 FACING，且要求该方向的相邻方块不可替换（即有可附着的实心墙面）。
    IWorld& world = context.getWorld();
    const BlockPos pos = context.placementPos();

    bool powered = world::redstone::RedstonePower::isPowered(world, pos);

    for (Direction direction : context.getNearestLookingDirections()) {
        if (!Directions::isHorizontal(direction)) {
            continue;
        }
        Direction facing = Directions::opposite(direction);
        const BlockPos attachPos = pos.offset(direction);
        const BlockState* attachState = world.getBlockState(attachPos);
        if (attachState != nullptr && !attachState->isAir() &&
            attachState->isFaceSturdy(world, attachPos, facing, SupportType::Full)) {
            return defaultState()
                .with(BlockStateProperties::POWERED(), powered)
                .with(BlockStateProperties::HORIZONTAL_FACING(), facing);
        }
    }

    return defaultState().with(BlockStateProperties::POWERED(), powered);
}

const BlockState& WallSkullBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), Directions::rotateDirection(facing, rotation));
}

const BlockState& WallSkullBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    return state.with(BlockStateProperties::HORIZONTAL_FACING(),
        Directions::rotateDirection(facing, Directions::mirrorToRotation(mirror, facing)));
}

bool WallSkullBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction attachDir = Directions::opposite(facing);
    const BlockPos attachPos = pos.offset(attachDir);
    const BlockState* attachState = world.getBlockState(attachPos);
    if (attachState == nullptr || attachState->isAir()) {
        return false;
    }
    return attachState->isFaceSturdy(world, attachPos, facing, SupportType::Full);
}

BlockState WallSkullBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 附着面消失时自毁
    Direction skullFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    if (facing == Directions::opposite(skullFacing)) {
        if (!isValidPosition(state, static_cast<IBlockReader&>(world), currentPos)) {
            if (const BlockState* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    return Block::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

// ============================================================================
// PlayerHeadBlock / PlayerWallHeadBlock
// ============================================================================

PlayerHeadBlock::PlayerHeadBlock(const BlockProperties& properties)
    : SkullBlock(SkullType::Player, properties)
{}

void PlayerHeadBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    MC_UNUSED(state);
    // 从物品的 SkullOwner 组件恢复档案写入方块实体（对齐 vanilla PlayerHeadBlock.setPlacedBy）
    if (BlockEntity* be = world.getBlockEntity(pos)) {
        auto* skull = dynamic_cast<blockentity::SkullBlockEntity*>(be);
        if (skull != nullptr) {
            const nlohmann::json* tag = stack.getTag();
            if (tag != nullptr && tag->contains("SkullOwner")) {
                auto result = skin::GameProfile::fromJson((*tag)["SkullOwner"]);
                if (result.success()) {
                    skull->setOwnerProfile(result.value());
                }
            }
        }
    }
}

PlayerWallHeadBlock::PlayerWallHeadBlock(const BlockProperties& properties)
    : WallSkullBlock(SkullType::Player, properties)
{}

void PlayerWallHeadBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    MC_UNUSED(state);
    if (BlockEntity* be = world.getBlockEntity(pos)) {
        auto* skull = dynamic_cast<blockentity::SkullBlockEntity*>(be);
        if (skull != nullptr) {
            const nlohmann::json* tag = stack.getTag();
            if (tag != nullptr && tag->contains("SkullOwner")) {
                auto result = skin::GameProfile::fromJson((*tag)["SkullOwner"]);
                if (result.success()) {
                    skull->setOwnerProfile(result.value());
                }
            }
        }
    }
}

// ============================================================================
// WitherSkullBlock / WitherWallSkullBlock
// ============================================================================

WitherSkullBlock::WitherSkullBlock(const BlockProperties& properties)
    : SkullBlock(SkullType::WitherSkeleton, properties)
{}

void WitherSkullBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    MC_UNUSED(state);
    MC_UNUSED(stack);
    checkSpawn(world, pos);
}

void WitherSkullBlock::checkSpawn(IWorld& world, const BlockPos& pos)
{
    // 对齐 vanilla WitherSkullBlock.checkSpawn(Level, BlockPos)：
    //   仅服务端判定；头颅方块须为凋灵骷髅头颅（站立或墙挂）；
    //   y >= 世界最低高度且难度非和平；匹配凋灵 T 形图案后清除图案方块并生成凋灵。
    if (world.isClientSide()) {
        return;
    }

    const BlockState* skullState = world.getBlockState(pos);
    if (skullState == nullptr) {
        return;
    }

    const bool isWitherSkull = skullState->is(VanillaBlocks::WITHER_SKELETON_SKULL) ||
        skullState->is(VanillaBlocks::WITHER_SKELETON_WALL_SKULL);
    if (!isWitherSkull) {
        return;
    }

    if (pos.y < world::MIN_BUILD_HEIGHT || world.difficulty() == Difficulty::Peaceful) {
        return;
    }

    // 凋灵图案（含头颅层）：以头颅为顶点向下一层为 3 个基座方块、再下一层为空气-基座-空气。
    // 图案可绕 Y 轴旋转（东西或南北方向），故先试以 pos 为顶点，再试其他头颅位置。
    BlockPos skullPos;
    bool forwardsAxisIsX = false;
    if (!_matchWitherPattern(world, pos, skullPos, forwardsAxisIsX)) {
        return;
    }

    // 生成凋灵实体：对应 vanilla EntityType.WITHER.create(level, TRIGGERED)。
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* witherType = registry.getType(entity::EntityTypeKeys::WITHER);
    if (witherType == nullptr) {
        return;
    }
    auto* ecsRegistry = world.entityRegistry();
    if (ecsRegistry == nullptr) {
        return;
    }

    std::unique_ptr<Entity> entity = witherType->create(&world, *ecsRegistry);
    if (entity == nullptr) {
        return;
    }

    // 清除图案方块（3 个基座方块 + 3 个中层基座 + 1 个底层中央基座）后于顶点生成凋灵。
    _clearWitherPattern(world, skullPos, forwardsAxisIsX);

    // 凋灵生成于图案顶点下方一层的中心（vanilla: match.getBlock(1, 2, 0) 即顶点头颅位）。
    entity->setPosition(
        static_cast<f32>(skullPos.x) + 0.5f, static_cast<f32>(skullPos.y) + 0.55f, static_cast<f32>(skullPos.z) + 0.5f);
    // 朝向按图案轴向决定（vanilla：X 轴 0 度、Z 轴 90 度）。
    entity->setRotation(forwardsAxisIsX ? 0.0f : 90.0f, 0.0f);

    auto* mobEntity = dynamic_cast<MobEntity*>(entity.get());
    if (mobEntity != nullptr) {
        entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(world, skullPos);
        mobEntity->finalizeSpawn(world, difficultyInstance, world::spawn::SpawnReason::Event);
    }

    // 对齐 vanilla：凋灵生成后进入 220 tick 生成无敌阶段（WitherBoss.makeInvulnerable）。
    if (auto* wither = dynamic_cast<entity::WitherEntity*>(entity.get())) {
        wither->makeInvulnerable();
    }

    world.spawnEntity(std::move(entity));
}

WitherWallSkullBlock::WitherWallSkullBlock(const BlockProperties& properties)
    : WallSkullBlock(SkullType::WitherSkeleton, properties)
{}

void WitherWallSkullBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    MC_UNUSED(state);
    MC_UNUSED(stack);
    WitherSkullBlock::checkSpawn(world, pos);
}

// ============================================================================
// PiglinWallSkullBlock
// ============================================================================

PiglinWallSkullBlock::PiglinWallSkullBlock(const BlockProperties& properties)
    : WallSkullBlock(SkullType::Piglin, properties)
{
    _buildWallShapes(m_piglinShapes, 10.0f);
}

const CollisionShape& PiglinWallSkullBlock::getShape(const BlockState& state) const
{
    return m_piglinShapes[_facingIndex(state.get(BlockStateProperties::HORIZONTAL_FACING()))];
}

} // namespace blocks
} // namespace mc
