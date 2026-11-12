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

#include "common/world/block/blocks/sculk/SculkVeinBlock.hpp"

#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/blocks/multiface/MultifaceBlock.hpp"
#include "common/world/block/capability/WaterLoggableHelpers.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/support/SupportType.hpp"
#include "common/world/block/tags/BlockTags.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "util/Direction.hpp"
#include "world/block/blocks/sculk/SculkSpreader.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/Fluids.hpp"
#include <cmath>
#include <utility>

namespace mc {
namespace blocks {
namespace {

/// MC SculkVeinBlock.SculkVeinSpreaderConfig：脉络扩散的特殊可替换判定。
class SculkVeinSpreaderConfig : public DefaultSpreaderConfig {
public:
    SculkVeinSpreaderConfig(const SculkVeinBlock& block, std::vector<MultifaceSpreadType> spreadTypes)
        : DefaultSpreaderConfig(block)
        , m_vein(block)
        , m_spreadTypes(std::move(spreadTypes))
    {}

    // 暴露基类 1 参 stateCanBeReplaced，避免 5 参重载将其隐藏（-Woverloaded-virtual）。
    using DefaultSpreaderConfig::stateCanBeReplaced;

    [[nodiscard]] const std::vector<MultifaceSpreadType>& getSpreadTypes() const override
    {
        return m_spreadTypes;
    }

    /// MC SculkVeinSpreaderConfig.isOtherBlockValidAsSource = !is(SCULK_VEIN)。
    [[nodiscard]] bool isOtherBlockValidAsSource(const BlockState& state) const override
    {
        return !state.is(&m_vein);
    }

    /// MC SculkVeinSpreaderConfig.stateCanBeReplaced（覆写为带 sourcePos/face 的版本）。
    [[nodiscard]] bool stateCanBeReplaced(IWorld& world,
        const BlockPos& sourcePos,
        const BlockPos& targetPos,
        Direction face,
        const BlockState& targetState) const
    {
        // MC: target.relative(face) 是 SCULK/SCULK_CATALYST/MOVING_PISTON → 拒绝。
        const BlockState* beyond = world.getBlockState(targetPos.offset(face));
        if (beyond != nullptr && (beyond->is(VanillaBlocks::SCULK) || beyond->is(VanillaBlocks::SCULK_CATALYST))) {
            return false;
        }
        // MC: WRAP_AROUND（sourcePos.distManhattan(targetPos)==2）且 sourcePos.relative(face.opposite)
        //     isFaceSturdy → 拒绝（防止两个 sturdy 面之间包绕）。
        if (distManhattan(sourcePos, targetPos) == 2) {
            const BlockPos back = sourcePos.offset(Directions::opposite(face));
            const BlockState* backState = world.getBlockState(back);
            if (backState != nullptr && backState->isFaceSturdy(world, back, face, SupportType::Full)) {
                return false;
            }
        }
        // MC: 流体非空且非水 → 拒绝；是火 → 拒绝。
        const fluid::FluidState* fluid = targetState.getFluidState();
        if (fluid != nullptr && !fluid->isEmpty() && &fluid->getFluid() != fluid::Fluids::WATER()) {
            return false;
        }
        if (BlockTags::FIRE().contains(targetState)) {
            return false;
        }
        return targetState.canBeReplaced() || DefaultSpreaderConfig::stateCanBeReplaced(targetState);
    }

    /// MC DefaultSpreaderConfig.canSpreadInto = stateCanBeReplaced && isValidStateForPlacement。
    [[nodiscard]] bool canSpreadInto(
        IWorld& world, const BlockPos& sourcePos, const MultifaceSpreadPos& spreadPos) const override
    {
        const BlockState* target = world.getBlockState(spreadPos.pos);
        if (target == nullptr) {
            // nullptr 视为空气：走 isValidStateForPlacement 的 canAttachTo 判定。
            return m_vein.canAttachTo(world, spreadPos.face, spreadPos.pos.offset(spreadPos.face));
        }
        if (!stateCanBeReplaced(world, sourcePos, spreadPos.pos, spreadPos.face, *target)) {
            return false;
        }
        return m_vein.isValidStateForPlacement(world, *target, spreadPos.pos, spreadPos.face);
    }

private:
    static i32 distManhattan(const BlockPos& a, const BlockPos& b) noexcept
    {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y) + std::abs(a.z - b.z);
    }

    const SculkVeinBlock& m_vein;
    std::vector<MultifaceSpreadType> m_spreadTypes;
};

} // namespace

// ============================================================================
// SculkVeinBlock
// ============================================================================

SculkVeinBlock::SculkVeinBlock(const BlockProperties& properties)
    : MultifaceBlock(properties)
    , m_veinSpreader(std::make_unique<SculkVeinSpreaderConfig>(*this, defaultSpreadOrder()))
    , m_sameSpaceSpreader(std::make_unique<SculkVeinSpreaderConfig>(
          *this, std::vector<MultifaceSpreadType>{MultifaceSpreadType::SamePosition}))
{
    buildMultifaceStateContainer();

    // 预计算 64 种面组合形状（2^6，与 GlowLichenBlock 一致：每面 1 像素薄板）。
    const CollisionShape northShape = CollisionShape::fromPixelBox(0, 0, 0, 16, 16, 1);
    const CollisionShape southShape = CollisionShape::fromPixelBox(0, 0, 15, 16, 16, 16);
    const CollisionShape eastShape = CollisionShape::fromPixelBox(15, 0, 0, 16, 16, 16);
    const CollisionShape westShape = CollisionShape::fromPixelBox(0, 0, 0, 1, 16, 16);
    const CollisionShape upShape = CollisionShape::fromPixelBox(0, 15, 0, 16, 16, 16);
    const CollisionShape downShape = CollisionShape::fromPixelBox(0, 0, 0, 16, 1, 16);

    for (i32 down = 0; down <= 1; ++down) {
        for (i32 up = 0; up <= 1; ++up) {
            for (i32 north = 0; north <= 1; ++north) {
                for (i32 south = 0; south <= 1; ++south) {
                    for (i32 east = 0; east <= 1; ++east) {
                        for (i32 west = 0; west <= 1; ++west) {
                            const size_t idx = static_cast<size_t>(
                                (down) | (up << 1) | (north << 2) | (south << 3) | (east << 4) | (west << 5));
                            CollisionShape shape = CollisionShape::empty();
                            if (north) {
                                shape = CollisionShape::combine(shape, northShape);
                            }
                            if (south) {
                                shape = CollisionShape::combine(shape, southShape);
                            }
                            if (east) {
                                shape = CollisionShape::combine(shape, eastShape);
                            }
                            if (west) {
                                shape = CollisionShape::combine(shape, westShape);
                            }
                            if (up) {
                                shape = CollisionShape::combine(shape, upShape);
                            }
                            if (down) {
                                shape = CollisionShape::combine(shape, downShape);
                            }
                            if (!north && !south && !east && !west && !up && !down) {
                                shape = CollisionShape::fullBlock();
                            }
                            m_shapes[idx] = shape;
                        }
                    }
                }
            }
        }
    }
}

size_t SculkVeinBlock::shapeIndex(const BlockState& state)
{
    size_t index = 0;
    if (state.get(BlockStateProperties::DOWN())) {
        index |= 1;
    }
    if (state.get(BlockStateProperties::UP())) {
        index |= 2;
    }
    if (state.get(BlockStateProperties::NORTH())) {
        index |= 4;
    }
    if (state.get(BlockStateProperties::SOUTH())) {
        index |= 8;
    }
    if (state.get(BlockStateProperties::EAST())) {
        index |= 16;
    }
    if (state.get(BlockStateProperties::WEST())) {
        index |= 32;
    }
    return index;
}

BlockState SculkVeinBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const Direction clickedFace = context.getClickedFace();
    const BlockState* current = context.getWorld().getBlockState(context.placementPos());
    const BlockState* placed =
        MultifaceBlock::getStateForPlacement(current, context.getWorld(), context.placementPos(), clickedFace);
    if (placed == nullptr) {
        return defaultState();
    }
    return *placed;
}

BlockState SculkVeinBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }
    return state;
}

const CollisionShape& SculkVeinBlock::getShape(const BlockState& state) const
{
    return m_shapes[shapeIndex(state)];
}

const fluid::FluidState* SculkVeinBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

// ========== SculkVeinBlock 的 SculkBehaviour 实现 ==========

i32 SculkVeinBlock::attemptUseCharge(ChargeCursor& cursor,
    IWorld& world,
    const BlockPos& /*origin*/,
    math::IRandom& random,
    SculkSpreader& spreader,
    bool shouldUpdateBlocks)
{
    // MC SculkVeinBlock.attemptUseCharge:
    //   shouldUpdateBlocks && attemptPlaceSculk → charge - 1；
    //   否则 nextInt(chargeDecayRate)==0 ? floor(charge*0.5) : charge。
    if (shouldUpdateBlocks && attemptPlaceSculk(spreader, world, cursor.pos(), random)) {
        return cursor.charge() - 1;
    }
    if (random.nextInt(spreader.chargeDecayRate()) == 0) {
        return static_cast<i32>(std::floor(static_cast<f32>(cursor.charge()) * 0.5f));
    }
    return cursor.charge();
}

bool SculkVeinBlock::attemptSpreadVein(IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    std::optional<std::vector<Direction>> facings,
    bool worldGen)
{
    // MC SculkBehaviour.DEFAULT.attemptSpreadVein（SculkVeinBlock 未覆写，走接口默认）：
    //   facings==null → sameSpaceSpreader.spreadAll(currentState, world, pos, worldGen) > 0；
    //   facings 非空且非 empty → (!isAir && !fluid.WATER) ? false : regrow(world, pos, state, facings)；
    //   facings empty → super（即 false）。
    if (!facings.has_value()) {
        const BlockState* current = world.getBlockState(pos);
        if (current == nullptr) {
            return m_sameSpaceSpreader.spreadAll(defaultState(), world, pos, worldGen) > 0;
        }
        return m_sameSpaceSpreader.spreadAll(*current, world, pos, worldGen) > 0;
    }
    if (facings.value().empty()) {
        return false;
    }
    if (!state.isAir()) {
        const fluid::FluidState* fluid = state.getFluidState();
        if (fluid == nullptr || &fluid->getFluid() != fluid::Fluids::WATER()) {
            return false;
        }
    }
    return regrow(world, pos, state, facings.value());
}

void SculkVeinBlock::onDischarged(IWorld& world, const BlockState& state, const BlockPos& pos, math::IRandom& random)
{
    // MC SculkVeinBlock.onDischarged: 移除贴在 SCULK 上的面；无面则变 AIR/WATER。
    MC_UNUSED(random);
    if (!state.is(this)) {
        return;
    }
    const BlockState* result = &state;
    for (Direction dir : Directions::all()) {
        if (MultifaceBlock::hasFace(*result, dir)) {
            const BlockState* neighbor = world.getBlockState(pos.offset(dir));
            if (neighbor != nullptr && neighbor->is(VanillaBlocks::SCULK)) {
                result = &result->with(*MultifaceBlock::getFaceProperty(dir), false);
            }
        }
    }
    if (!MultifaceBlock::hasAnyFace(*result)) {
        const fluid::FluidState* fluid =
            world.getBlockState(pos) != nullptr ? world.getBlockState(pos)->getFluidState() : nullptr;
        result = (fluid != nullptr && !fluid->isEmpty()) ? &VanillaBlocks::WATER->defaultState()
                                                         : &VanillaBlocks::AIR->defaultState();
    }
    world.setBlockState(pos, result, world::BlockUpdateFlags::UPDATE_ALL);
}

bool SculkVeinBlock::hasSubstrateAccess(IWorld& world, const BlockState& state, const BlockPos& pos)
{
    // MC SculkVeinBlock.hasSubstrateAccess: is(SCULK_VEIN) 且某面朝向 SCULK_REPLACEABLE。
    if (!state.is(VanillaBlocks::SCULK_VEIN)) {
        return false;
    }
    for (Direction dir : Directions::all()) {
        if (MultifaceBlock::hasFace(state, dir)) {
            const BlockState* neighbor = world.getBlockState(pos.offset(dir));
            if (neighbor != nullptr && BlockTags::SCULK_REPLACEABLE().contains(*neighbor)) {
                return true;
            }
        }
    }
    return false;
}

bool SculkVeinBlock::regrow(
    IWorld& world, const BlockPos& pos, const BlockState& current, const std::vector<Direction>& directions)
{
    // MC SculkVeinBlock.regrow: 默认 vein 状态逐方向加面（仅 canAttachTo 的方向）；
    //   无任何可加面 → false；有流体则 WATERLOGGED。
    BlockState vein = VanillaBlocks::SCULK_VEIN->defaultState();
    bool any = false;
    for (Direction dir : directions) {
        if (MultifaceBlock::canAttachTo(world, dir, pos.offset(dir))) {
            vein = vein.with(*MultifaceBlock::getFaceProperty(dir), true);
            any = true;
        }
    }
    if (!any) {
        return false;
    }
    const fluid::FluidState* fluid = current.getFluidState();
    if (fluid != nullptr && !fluid->isEmpty()) {
        vein = vein.with(BlockStateProperties::WATERLOGGED(), true);
    }
    world.setBlockState(pos, &vein, world::BlockUpdateFlags::UPDATE_ALL);
    return true;
}

bool SculkVeinBlock::attemptPlaceSculk(
    SculkSpreader& spreader, IWorld& world, const BlockPos& pos, math::IRandom& random)
{
    // MC SculkVeinBlock.attemptPlaceSculk:
    //   打乱方向遍历，对 vein 已有面的方向，取相邻格；若属 replaceableBlocks → 放 SCULK +
    //   pushEntitiesUp + veinSpreader.spreadAll(sculk, world, neighbor, worldGen) +
    //   对新 sculk 除来源反方向外的相邻 vein 调 onDischarged。
    const BlockState* veinState = world.getBlockState(pos);
    if (veinState == nullptr) {
        return false;
    }
    const BlockTag& replaceable = spreader.replaceableBlocks();

    std::vector<Direction> dirs = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};
    random.shuffle(dirs);

    for (Direction dir : dirs) {
        if (!MultifaceBlock::hasFace(*veinState, dir)) {
            continue;
        }
        const BlockPos neighbor = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighbor);
        if (neighborState == nullptr || !replaceable.contains(*neighborState)) {
            continue;
        }
        const BlockState* sculkState = &VanillaBlocks::SCULK->defaultState();
        world.setBlockState(neighbor, sculkState, world::BlockUpdateFlags::UPDATE_ALL);
        Block::pushEntitiesUp(*neighborState, *sculkState, world, neighbor);
        // MC: veinSpreader.spreadAll 返回成功扩散数，此处仅用副作用（蔓延脉络），忽略计数。
        (void)m_veinSpreader.spreadAll(*sculkState, world, neighbor, spreader.isWorldGeneration());

        const Direction sourceOpposite = Directions::opposite(dir);
        for (Direction d2 : Directions::all()) {
            if (d2 == sourceOpposite) {
                continue;
            }
            const BlockPos n2 = neighbor.offset(d2);
            const BlockState* n2State = world.getBlockState(n2);
            if (n2State != nullptr && n2State->is(this)) {
                onDischarged(world, *n2State, n2, random);
            }
        }
        return true;
    }
    return false;
}

} // namespace blocks
} // namespace mc
