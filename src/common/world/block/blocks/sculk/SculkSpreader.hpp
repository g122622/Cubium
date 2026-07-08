/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#pragma once

#include "common/util/Direction.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/sculk/SculkBehaviour.hpp"

#include <optional>
#include <vector>

namespace mc {
namespace blocks {

class SculkSpreader;

/**
 * @brief 电荷游标（MC SculkSpreader.ChargeCursor）
 *
 * 携带一份电荷在某个方块上，update 时：尝试扩散脉络 → 消耗电荷（attemptUseCharge）
 * → 若仍有电荷则移动到相邻 sculk-behaviour 方块 → 更新衰减延迟与 update 延迟。
 *
 * 定义在 SculkSpreader 之前，使 SculkSpreader 可持有 vector<ChargeCursor> 值成员
 * （MSVC 的 std::vector 析构需元素完整类型）；ChargeCursor 的成员函数在 .cpp 中定义，
 * 彼时 SculkSpreader 已完整。
 */
class ChargeCursor {
public:
    ChargeCursor() = default;
    ChargeCursor(BlockPos pos, i32 charge)
        : m_pos(pos)
        , m_charge(charge)
    {}

    [[nodiscard]] const BlockPos& pos() const noexcept { return m_pos; }
    [[nodiscard]] i32 charge() const noexcept { return m_charge; }
    [[nodiscard]] i32 decayDelay() const noexcept { return m_decayDelay; }
    [[nodiscard]] const std::optional<std::vector<Direction>>& facingData() const noexcept { return m_facings; }

    /// MC ChargeCursor.isPosUnreasonable：切比雪夫距离 > MAX_CURSOR_DISTANCE。
    [[nodiscard]] bool isPosUnreasonable(const BlockPos& origin) const noexcept;

    /// MC ChargeCursor.update。
    void update(
        IWorld& world, const BlockPos& origin, math::IRandom& random, SculkSpreader& spreader, bool shouldUpdateBlocks);

    /// MC ChargeCursor.mergeWith。
    void mergeWith(ChargeCursor& other) noexcept;

private:
    /// MC ChargeCursor.shouldUpdate。
    [[nodiscard]] bool shouldUpdate(bool worldGen) const noexcept;

    /// MC ChargeCursor.getValidMovementPos。
    [[nodiscard]] std::optional<BlockPos> getValidMovementPos(
        IWorld& world, const BlockPos& pos, math::IRandom& random);

    /// MC ChargeCursor.isMovementUnobstructed。
    [[nodiscard]] static bool isMovementUnobstructed(IWorld& world, const BlockPos& from, const BlockPos& to);

    /// MC ChargeCursor.isUnobstructed。
    [[nodiscard]] static bool isUnobstructed(IWorld& world, const BlockPos& from, Direction dir);

    /// MC SculkBehaviour.getBlockBehaviour：方块是否实现 SculkBehaviour，否则用 DEFAULT。
    [[nodiscard]] static SculkBehaviour* getBlockBehaviour(const BlockState& state);

    BlockPos m_pos;
    i32 m_charge = 0;
    i32 m_updateDelay = 0;
    i32 m_decayDelay = 1;
    std::optional<std::vector<Direction>> m_facings;
};

/**
 * @brief 幽匿扩散器（MC SculkSpreader）
 *
 * 维护一组 ChargeCursor，由 SculkPatchFeature / SculkCatalystBlock 驱动 updateCursors
 * 让电荷在可替换方块上扩散、衰减、生成 sculk 生长物（sensor/shrieker）。
 *
 * 两套预设：
 * - createLevelSpreader：运行期，SCULK_REPLACEABLE，growthSpawnCost=10/noGrowthRadius=4/
 *   chargeDecayRate=10/additionalDecayRate=5。
 * - createWorldGenSpreader：世界生成期，SCULK_REPLACEABLE_WORLD_GEN，50/1/5/10。
 *
 * MC 1.21.11 对齐：MAX_CURSORS=32、MAX_CHARGE=1000、MAX_CURSOR_DISTANCE=1024、
 * worldgen 期游标距中心 15 格外清零电荷。
 */
class SculkSpreader {
public:
    /// MC SculkSpreader.MAX_CHARGE。
    static constexpr i32 kMaxCharge = 1000;

    /// MC SculkSpreader.MAX_CURSORS（游标数上限）。
    static constexpr size_t kMaxCursors = 32;

    /// MC SculkSpreader.MAX_CURSOR_DISTANCE（游标距中心切比雪夫距离上限）。
    static constexpr i32 kMaxCursorDistance = 1024;

    /// MC SculkSpreader.createLevelSpreader。
    [[nodiscard]] static SculkSpreader createLevelSpreader();

    /// MC SculkSpreader.createWorldGenSpreader。
    [[nodiscard]] static SculkSpreader createWorldGenSpreader();

    SculkSpreader(bool worldGen,
        const BlockTag& replaceableBlocks,
        i32 growthSpawnCost,
        i32 noGrowthRadius,
        i32 chargeDecayRate,
        i32 additionalDecayRate)
        : m_worldGen(worldGen)
        , m_replaceableBlocks(replaceableBlocks)
        , m_growthSpawnCost(growthSpawnCost)
        , m_noGrowthRadius(noGrowthRadius)
        , m_chargeDecayRate(chargeDecayRate)
        , m_additionalDecayRate(additionalDecayRate)
    {}

    [[nodiscard]] const BlockTag& replaceableBlocks() const noexcept { return m_replaceableBlocks; }
    [[nodiscard]] i32 growthSpawnCost() const noexcept { return m_growthSpawnCost; }
    [[nodiscard]] i32 noGrowthRadius() const noexcept { return m_noGrowthRadius; }
    [[nodiscard]] i32 chargeDecayRate() const noexcept { return m_chargeDecayRate; }
    [[nodiscard]] i32 additionalDecayRate() const noexcept { return m_additionalDecayRate; }
    [[nodiscard]] bool isWorldGeneration() const noexcept { return m_worldGen; }

    /// MC SculkSpreader.addCursors：在 pos 注入 amount 电荷（按 kMaxCharge 分批）。
    void addCursors(const BlockPos& pos, i32 amount);

    /// MC SculkSpreader.updateCursors：推进所有游标一轮，去重合并。
    void updateCursors(IWorld& world, const BlockPos& origin, math::IRandom& random, bool shouldUpdateBlocks);

    /// MC SculkSpreader.clear。
    void clear() noexcept { m_cursors.clear(); }

    [[nodiscard]] const std::vector<ChargeCursor>& cursors() const noexcept { return m_cursors; }

private:
    bool m_worldGen;
    const BlockTag& m_replaceableBlocks;
    i32 m_growthSpawnCost;
    i32 m_noGrowthRadius;
    i32 m_chargeDecayRate;
    i32 m_additionalDecayRate;
    std::vector<ChargeCursor> m_cursors;
};

} // namespace blocks
} // namespace mc
