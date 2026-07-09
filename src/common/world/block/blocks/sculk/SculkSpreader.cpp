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

#include "SculkSpreader.hpp"
#include "SculkBehaviour.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/SupportType.hpp"
#include "common/world/block/blocks/MultifaceBlock.hpp"
#include "common/world/block/registry/SculkBlocks.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace mc {
namespace blocks {

namespace {

/// MC MultifaceSpreader.ChargeCursor.NON_CORNER_NEIGHBOURS：3×3×3 立方中至少一轴为 0
/// 且非原点的 18 个偏移（切角剔除）。
const std::vector<BlockPos>& nonCornerNeighbourOffsets()
{
    static const std::vector<BlockPos> offsets = [] {
        std::vector<BlockPos> v;
        v.reserve(18);
        for (i32 dx = -1; dx <= 1; ++dx) {
            for (i32 dy = -1; dy <= 1; ++dy) {
                for (i32 dz = -1; dz <= 1; ++dz) {
                    if (dx == 0 || dy == 0 || dz == 0) {
                        if (!(dx == 0 && dy == 0 && dz == 0)) {
                            v.emplace_back(dx, dy, dz);
                        }
                    }
                }
            }
        }
        return v;
    }();
    return offsets;
}

/// MC BlockPos.distChessboard：切比雪夫距离（三轴绝对值最大）。
i32 distChessboard(const BlockPos& a, const BlockPos& b) noexcept
{
    const i32 dx = std::abs(a.x - b.x);
    const i32 dy = std::abs(a.y - b.y);
    const i32 dz = std::abs(a.z - b.z);
    return std::max({dx, dy, dz});
}

/// MC BlockPos.distManhattan：曼哈顿距离（三轴绝对值之和）。
i32 distManhattan(const BlockPos& a, const BlockPos& b) noexcept
{
    return std::abs(a.x - b.x) + std::abs(a.y - b.y) + std::abs(a.z - b.z);
}

/// MC Vec3i.closerThan / BlockPos.closerThan：欧氏距离平方 < dist²。
bool closerThan(const BlockPos& a, const BlockPos& b, f64 distance) noexcept
{
    return static_cast<f64>(a.distanceSq(b)) < distance * distance;
}

/// MC SculkBehaviour.DEFAULT：非 SculkBehaviour 方块的占位行为。
class DefaultSculkBehaviour final : public SculkBehaviour {
public:
    [[nodiscard]] bool canChangeBlockStateOnSpread() const override { return true; }

    [[nodiscard]] i32 updateDecayDelay(i32 decayDelay) const override { return std::max(decayDelay - 1, 0); }

    [[nodiscard]] bool attemptSpreadVein(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        std::optional<std::vector<Direction>> facings,
        bool worldGen) override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        MC_UNUSED(facings);
        MC_UNUSED(worldGen);
        // DEFAULT.attemptSpreadVein 依赖 SculkVeinBlock 的 sameSpaceSpreader/spreadAll，
        // 此处仅在非 vein 方块上被调用——实际游戏中 SculkCatalyst 触发的扩散源始终是 vein
        // 或 sculk，故 DEFAULT 路径不发生脉络扩散。
        return false;
    }

    [[nodiscard]] i32 attemptUseCharge(ChargeCursor& cursor,
        IWorld& world,
        const BlockPos& origin,
        math::IRandom& random,
        SculkSpreader& spreader,
        bool shouldUpdateBlocks) override
    {
        MC_UNUSED(world);
        MC_UNUSED(origin);
        MC_UNUSED(random);
        MC_UNUSED(spreader);
        MC_UNUSED(shouldUpdateBlocks);
        // MC DEFAULT.attemptUseCharge: decayDelay > 0 ? charge : 0
        return cursor.decayDelay() > 0 ? cursor.charge() : 0;
    }
};

DefaultSculkBehaviour& defaultBehaviour()
{
    static DefaultSculkBehaviour instance;
    return instance;
}

/// MC SculkVeinBlock.hasSubstrateAccess：候选格是 vein 且其某面朝向 SCULK_REPLACEABLE 方块。
/// getValidMovementPos 仅在 SculkBehaviour 方块上调用，此处先判 is(SCULK_VEIN)。
bool hasSubstrateAccess(IWorld& world, const BlockState& state, const BlockPos& pos)
{
    if (!state.is(block_registry::SculkBlocks::SCULK_VEIN)) {
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

} // namespace

// ============================================================================
// SculkSpreader
// ============================================================================

SculkSpreader SculkSpreader::createLevelSpreader()
{
    return SculkSpreader(false, BlockTags::SCULK_REPLACEABLE(), 10, 4, 10, 5);
}

SculkSpreader SculkSpreader::createWorldGenSpreader()
{
    return SculkSpreader(true, BlockTags::SCULK_REPLACEABLE_WORLD_GEN(), 50, 1, 5, 10);
}

void SculkSpreader::addCursors(const BlockPos& pos, i32 amount)
{
    // MC addCursors: 按 MAX_CHARGE 分批注入游标，总数受 MAX_CURSORS 限制。
    while (amount > 0) {
        const i32 chunk = std::min(amount, kMaxCharge);
        if (m_cursors.size() < kMaxCursors) {
            m_cursors.emplace_back(pos, chunk);
        }
        amount -= chunk;
    }
}

void SculkSpreader::updateCursors(IWorld& world, const BlockPos& origin, math::IRandom& random, bool shouldUpdateBlocks)
{
    if (m_cursors.empty()) {
        return;
    }

    // MC updateCursors：list 收集存活游标，map[pos] 记录同位代表游标在 list 中的索引，
    // object2intmap[pos] 累加同位电荷。用索引而非指针，避免 vector 扩容导致引用失效。
    std::vector<ChargeCursor> nextCursors;
    std::unordered_map<u64, size_t> repIndexByPos; // pos.toId() → 代表游标在 nextCursors 中的下标
    std::unordered_map<u64, i32> chargeSumByPos;   // pos.toId() → 同位电荷合计

    for (ChargeCursor& cursor : m_cursors) {
        if (cursor.isPosUnreasonable(origin)) {
            continue;
        }
        cursor.update(world, origin, random, *this, shouldUpdateBlocks);

        if (cursor.charge() <= 0) {
            world.playEvent(world::WorldEvents::SCULK_CHARGE, cursor.pos(), 0);
            continue;
        }

        const u64 key = cursor.pos().toId();
        chargeSumByPos[key] += cursor.charge();

        auto it = repIndexByPos.find(key);
        if (it == repIndexByPos.end()) {
            repIndexByPos.emplace(key, nextCursors.size());
            nextCursors.push_back(std::move(cursor));
        } else if (!m_worldGen) {
            // 运行期：同位电荷合并（合计 <= MAX_CHARGE 才合并）。
            ChargeCursor& existing = nextCursors[it->second];
            if (existing.charge() + cursor.charge() <= kMaxCharge) {
                existing.mergeWith(cursor);
            } else {
                const i32 cursorCharge = cursor.charge(); // std::move 前捕获，用于比较
                const size_t newIdx = nextCursors.size();
                nextCursors.push_back(std::move(cursor));
                if (cursorCharge < existing.charge()) {
                    it->second = newIdx;
                }
            }
        } else {
            // worldgen 期不合并，直接保留。
            nextCursors.push_back(std::move(cursor));
        }
    }

    // MC updateCursors 末尾：对每个有面数据的同位聚合点发 levelEvent 3006，data 编码电荷与面掩码。
    for (const auto& [key, total] : chargeSumByPos) {
        if (total <= 0) {
            continue;
        }
        auto it = repIndexByPos.find(key);
        if (it == repIndexByPos.end()) {
            continue;
        }
        const ChargeCursor& representative = nextCursors[it->second];
        if (!representative.facingData().has_value()) {
            continue;
        }
        const i32 level = static_cast<i32>(std::log1p(static_cast<f64>(total)) / 2.3f) + 1;
        const i32 data = (level << 6) | static_cast<i32>(MultifaceBlock::pack(representative.facingData().value()));
        world.playEvent(world::WorldEvents::SCULK_CHARGE, representative.pos(), data);
    }

    m_cursors = std::move(nextCursors);
}

// ============================================================================
// ChargeCursor
// ============================================================================

bool ChargeCursor::isPosUnreasonable(const BlockPos& origin) const noexcept
{
    return distChessboard(m_pos, origin) > SculkSpreader::kMaxCursorDistance;
}

bool ChargeCursor::shouldUpdate(bool worldGen) const noexcept
{
    if (m_charge <= 0) {
        return false;
    }
    if (worldGen) {
        return true;
    }
    // 运行期需 ServerLevel.shouldTickBlocksAt；worldgen 路径始终 worldGen=true，此处仅作占位。
    return false;
}

SculkBehaviour* ChargeCursor::getBlockBehaviour(const BlockState& state)
{
    // MC: state.getBlock() instanceof SculkBehaviour ? sculkbehaviour : DEFAULT
    Block* block = const_cast<Block*>(&state.getBlock());
    if (block == nullptr) {
        return &defaultBehaviour();
    }
    auto* behaviour = dynamic_cast<SculkBehaviour*>(block);
    return behaviour != nullptr ? behaviour : &defaultBehaviour();
}

void ChargeCursor::update(
    IWorld& world, const BlockPos& origin, math::IRandom& random, SculkSpreader& spreader, bool shouldUpdateBlocks)
{
    if (!shouldUpdate(spreader.isWorldGeneration())) {
        return;
    }

    if (m_updateDelay > 0) {
        --m_updateDelay;
        return;
    }

    const BlockState* statePtr = world.getBlockState(m_pos);
    if (statePtr == nullptr) {
        return;
    }
    BlockState state = *statePtr;
    SculkBehaviour* behaviour = getBlockBehaviour(state);

    if (shouldUpdateBlocks &&
        behaviour->attemptSpreadVein(world, m_pos, state, m_facings, spreader.isWorldGeneration())) {
        if (behaviour->canChangeBlockStateOnSpread()) {
            const BlockState* refreshed = world.getBlockState(m_pos);
            if (refreshed != nullptr) {
                state = *refreshed;
                behaviour = getBlockBehaviour(state);
            }
        }
        // MC 此处播放 SCULK_BLOCK_SPREAD 音效；worldgen 期忽略音效。
    }

    m_charge = behaviour->attemptUseCharge(*this, world, origin, random, spreader, shouldUpdateBlocks);

    if (m_charge <= 0) {
        behaviour->onDischarged(world, state, m_pos, random);
        return;
    }

    const std::optional<BlockPos> movePos = getValidMovementPos(world, m_pos, random);
    if (movePos.has_value()) {
        behaviour->onDischarged(world, state, m_pos, random);
        m_pos = movePos.value();

        // MC worldgen 期：游标移动后若距中心（仅 XZ）超过 15 格则清零电荷。
        const BlockPos xzOrigin(origin.x, m_pos.y, origin.z);
        if (spreader.isWorldGeneration() && !closerThan(m_pos, xzOrigin, 15.0)) {
            m_charge = 0;
            return;
        }

        const BlockState* movedState = world.getBlockState(m_pos);
        if (movedState != nullptr) {
            state = *movedState;
        }
    }

    if (dynamic_cast<const SculkBehaviour*>(&state.getBlock()) != nullptr) {
        m_facings = MultifaceBlock::availableFaces(state);
    }

    m_decayDelay = behaviour->updateDecayDelay(m_decayDelay);
    m_updateDelay = behaviour->getSculkSpreadDelay();
}

void ChargeCursor::mergeWith(ChargeCursor& other) noexcept
{
    m_charge += other.m_charge;
    other.m_charge = 0;
    m_updateDelay = std::min(m_updateDelay, other.m_updateDelay);
}

std::optional<BlockPos> ChargeCursor::getValidMovementPos(IWorld& world, const BlockPos& pos, math::IRandom& random)
{
    // MC: 遍历打乱后的 18 个非切角邻居偏移，找到第一个是 SculkBehaviour 且移动无阻挡的位置；
    //     若该位置有 substrate 访问（vein 贴可替换方块）则提前 break。
    std::vector<BlockPos> offsets = nonCornerNeighbourOffsets();
    random.shuffle(offsets);

    std::optional<BlockPos> result;
    for (const BlockPos& offset : offsets) {
        const BlockPos candidate = pos + offset;
        const BlockState* candidateState = world.getBlockState(candidate);
        if (candidateState == nullptr) {
            continue;
        }
        if (dynamic_cast<const SculkBehaviour*>(&candidateState->getBlock()) == nullptr) {
            continue;
        }
        if (!isMovementUnobstructed(world, pos, candidate)) {
            continue;
        }
        result = candidate;
        // MC SculkVeinBlock.hasSubstrateAccess：若 vein 的某面朝向 SCULK_REPLACEABLE 方块则 break。
        if (hasSubstrateAccess(world, *candidateState, candidate)) {
            break;
        }
    }
    return result;
}

bool ChargeCursor::isMovementUnobstructed(IWorld& world, const BlockPos& from, const BlockPos& to)
{
    // MC: 曼哈顿距离为 1（直接相邻）则无阻挡；否则需检查两个轴方向的中间格是否非 sturdy。
    if (distManhattan(from, to) == 1) {
        return true;
    }
    const BlockPos delta = to - from;
    const Direction dirX =
        Directions::fromAxisAndDirection(Axis::X, delta.x < 0 ? AxisDirection::Negative : AxisDirection::Positive);
    const Direction dirY =
        Directions::fromAxisAndDirection(Axis::Y, delta.y < 0 ? AxisDirection::Negative : AxisDirection::Positive);
    const Direction dirZ =
        Directions::fromAxisAndDirection(Axis::Z, delta.z < 0 ? AxisDirection::Negative : AxisDirection::Positive);

    if (delta.x == 0) {
        return isUnobstructed(world, from, dirY) || isUnobstructed(world, from, dirZ);
    }
    if (delta.y == 0) {
        return isUnobstructed(world, from, dirX) || isUnobstructed(world, from, dirZ);
    }
    return isUnobstructed(world, from, dirX) || isUnobstructed(world, from, dirY);
}

bool ChargeCursor::isUnobstructed(IWorld& world, const BlockPos& from, Direction dir)
{
    // MC: 中间格在 dir 反方向的面不是 sturdy → 无阻挡。
    const BlockPos neighbor = from.offset(dir);
    const BlockState* state = world.getBlockState(neighbor);
    if (state == nullptr) {
        return true;
    }
    return !state->isFaceSturdy(world, neighbor, Directions::opposite(dir), SupportType::Full);
}

} // namespace blocks
} // namespace mc
