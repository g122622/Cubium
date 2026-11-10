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

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Entity;

namespace blocks {

/**
 * @brief 床方块
 *
 * 双方块结构（头部和脚部），支持16种颜色。
 * 在下界和末地会爆炸，在主世界可以设置重生点。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 朝向 (NORTH, SOUTH, EAST, WEST)
 * - BED_PART: 部分 (HEAD, FOOT)
 * - OCCUPIED: 是否被占用
 */
class BedBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param color 染料颜色
     * @param properties 方块属性
     */
    BedBlock(DyeColor color, const BlockProperties& properties);
    ~BedBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 旋转方块状态
     *
     * 原版床继承 `HorizontalDirectionalBlock`，其 `rotate` 会旋转 HORIZONTAL_FACING。
     * 结构放置按构件旋转整块模板，漏掉这里会让床保持模板内朝向（试炼密室的床朝向差异
     * 即由此暴露）。
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像方块状态
     *
     * 原版 `HorizontalDirectionalBlock#mirror`：由当前朝向求出镜像对应的旋转，再旋转朝向。
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 硬度 ==========

    [[nodiscard]] f32 getExplosionResistance(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        // 在下界爆炸
        return 0.2f;
    }

    // ========== 工具方法 ==========

    /**
     * @brief 获取床的颜色
     */
    [[nodiscard]] DyeColor getColor() const
    {
        return m_color;
    }

    /**
     * @brief 设置床被占用状态
     */
    static void setOccupied(IWorld& world, const BlockPos& pos, BlockState& state, bool occupied);

    /**
     * @brief 检查床是否可用
     */
    [[nodiscard]] static bool isBed(IWorld& world, const BlockPos& pos);

    /**
     * @brief 获取床的朝向
     *
     * 读取床方块的 HORIZONTAL_FACING 属性。如果指定位置不是床方块，
     * 返回 Direction::None。
     *
     * @param world 世界引用
     * @param pos 床头位置
     * @return 床的朝向，如果不是床则返回 Direction::None
     */
    [[nodiscard]] static Direction getBedOrientation(IWorld& world, const BlockPos& pos);

    /**
     * @brief 获取床的连接方向
     *
     * 根据床的部分（头部/脚部）和朝向，返回指向另一部分的方向。
     * 头部指向脚部，脚部指向头部。
     *
     * @param state 床方块状态
     * @return 连接方向
     */
    [[nodiscard]] static Direction getConnectedDirection(const BlockState& state);

    /**
     * @brief 计算从床上起身的安全位置
     *
     * 以床方块的顺时针方向为基准，根据实体朝向决定优先侧，构造 10 个周围候选偏移与
     * 2 个"床上"候选偏移。先以安全模式（拒绝危险方块与悬空）搜索，全部失败后再允许
     * 不安全位置。双层床（下方一格仍是床）额外搜索下一层的同一组偏移集合。
     *
     * @param world 世界引用
     * @param bedPos 床方块位置（BED_RULE 记录的位置，通常是玩家点击的那个半床）
     * @param bedFacing 床的朝向
     * @param entityYaw 实体的偏航角（度），仅用于决定从哪一侧开始搜索
     * @param width 实体碰撞箱宽度
     * @param height 实体碰撞箱高度
     * @param isPlayer 是否按玩家规则检查（禁止在末地传送门/折跃门内部重生）
     * @param fireImmune 实体是否免疫火焰（安全模式不把火/岩浆当作危险方块）
     * @return 安全落点（实体脚底位置）；所有候选都不可用时返回 nullopt
     */
    [[nodiscard]] static std::optional<Vector3> findStandUpPosition(const IWorld& world,
        const BlockPos& bedPos,
        Direction bedFacing,
        f32 entityYaw,
        f32 width,
        f32 height,
        bool isPlayer,
        bool fireImmune);

    /**
     * @brief 右键交互 - 睡眠或爆炸
     *
     * 在主世界可以睡眠设置重生点，在下界和末地会爆炸。
     */
    [[nodiscard]] BlockActionResult useWithoutItem(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        const BlockRaycastResult& hit) override;

    /**
     * @brief 方块放置后处理
     *
     * 在脚部放置后，自动放置头部方块。
     *
     * @param stack 放置该方块的物品堆（可能携带自定义名称等组件）
     */
    void onBlockPlacedBy(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        const Entity* placer,
        const ItemStack& stack) override;

    /**
     * @brief 玩家破坏方块前的处理
     *
     * 当脚部被创造模式玩家破坏时，同时移除头部方块。
     */
    void playerWillDestroy(IWorld& world, const BlockPos& pos, const BlockState& state, Player& player) override;

protected:
    /// 床颜色
    DyeColor m_color;

    /// 各朝向的形状缓存
    std::array<CollisionShape, 8> m_shapesByFacing; // [0..3] = head(north/south/west/east)，[4..7] = foot(同序)
    /// 寻路通行判定。
    [[nodiscard]] bool isPathfindable(const BlockState& state, PathComputationType type) const override;
};

} // namespace blocks
} // namespace mc
