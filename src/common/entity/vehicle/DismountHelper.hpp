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

#pragma once

#include "common/core/Types.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <optional>

namespace mc {

class IWorld;
class BlockState;

/**
 * @brief 下马/出生落点辅助工具
 *
 * 为"实体从一个方块柱附近落地"这类需求（下马、床与重生锚的起身位置、重生锚安全点）
 * 提供与碰撞、地面高度、世界边界、危险方块相关的统一判定。
 */
class DismountHelper {
public:
    /**
     * @brief 方块顶面到其上表面之间的地面高度是否可用
     *
     * 对应 vanilla DismountHelper.isBlockFloorValid：地面高度必须有限且小于 1.0
     * （大于等于 1.0 意味着下方无可站立面，实体只能卡在方块内部）。
     *
     * @param floorHeight 地面高度（方块顶面为 0.0，向上为正）
     */
    [[nodiscard]] static bool isBlockFloorValid(f64 floorHeight) noexcept;

    /**
     * @brief 在指定方块柱寻找实体的安全落点
     *
     * 判定链（任一不满足返回 nullopt）：
     * 1. safeMode 且该方块危险（燃烧类方块、凋灵玫瑰、甜浆果丛、仙人掌、细雪）→ 拒绝；
     * 2. 地面高度（跳过可攀爬方块与开启的陷阱门后向下测量的碰撞面高度）不可用 → 拒绝；
     * 3. safeMode 且地面高度 <= 0 且下方方块危险 → 拒绝；
     * 4. 实体碰撞箱与该位置方块碰撞 → 拒绝；
     * 5. 玩家额外要求当前方块与上方方块都不在 INVALID_SPAWN_INSIDE 中；
     * 6. 碰撞箱必须位于世界边界内。
     *
     * @param world 世界
     * @param pos 方块柱位置（实体底面中心将落在该方块中心）
     * @param width 实体碰撞箱宽度
     * @param height 实体碰撞箱高度
     * @param isPlayer 是否按玩家规则检查 INVALID_SPAWN_INSIDE
     * @param fireImmune 实体是否免疫火焰（免疫实体不受燃烧类方块影响）
     * @param safeMode 是否启用安全模式（拒绝危险方块与无地面悬空）
     * @return 实体脚底位置；找不到安全落点时返回 nullopt
     */
    [[nodiscard]] static std::optional<Vector3> findSafeDismountLocation(
        const IWorld& world, const BlockPos& pos, f32 width, f32 height, bool isPlayer, bool fireImmune, bool safeMode);

    /**
     * @brief 计算实体碰撞箱在某方块柱上的地面高度
     *
     * 对应 vanilla CollisionGetter.getBlockFloorHeight：自上而下取第一个不与实体
     * 碰撞的"地板"——方块本身与下方方块都只有可攀爬方块/开启陷阱门的碰撞形状被忽略。
     *
     * @return 地面高度（方块顶面为 0.0）；间距不可跨越（> 2 格）时返回正无穷
     */
    [[nodiscard]] static f64 getBlockFloorHeight(const IWorld& world, const BlockPos& pos, f32 width, f32 height);

    /**
     * @brief 方块状态是否会对实体造成危险（落点意义上的危险）
     */
    [[nodiscard]] static bool isBlockDangerous(const BlockState& state, bool fireImmune);

private:
    DismountHelper() = delete;
};

} // namespace mc
