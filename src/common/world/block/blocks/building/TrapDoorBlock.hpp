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
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IWaterLoggable.hpp"
#include "common/world/block/Material.hpp"
#include <array>
#include <cstddef>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class BlockRaycastResult;

namespace blocks {

/**
 * @brief 活板门方块
 *
 * 可被玩家或红石控制开关，可以放置在方块的顶部或底部。
 * 实现 IWaterLoggable 接口支持含水功能。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 水平朝向 (NORTH, SOUTH, EAST, WEST)
 * - OPEN: 是否打开
 * - HALF: 上半/下半 (TOP, BOTTOM) - 对应MC的Half枚举
 * - POWERED: 是否被充能
 * - WATERLOGGED: 是否含水
 */
class TrapDoorBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param isIron 是否为铁活板门（只能红石控制）
     */
    TrapDoorBlock(const BlockProperties& properties, bool isIron = false);

    /**
     * @brief 析构函数
     */
    ~TrapDoorBlock() override = default;

    // ========== 放置和更新 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    // ========== 交互 ==========

    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 是否使用形状进行光照遮挡检测
     *
     * 活板门有复杂的形状（关闭时只有部分面），需要精确的形状遮挡检测。
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== 红石 ==========

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 静态方法 ==========

    /**
     * @brief 检查活板门是否打开
     * @param state 方块状态
     * @return 如果打开返回true
     */
    [[nodiscard]] static bool isOpen(const BlockState& state);

    /**
     * @brief 切换活板门开关状态
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前状态
     * @param open 是否打开
     */
    static void toggle(IWorld& world, const BlockPos& pos, const BlockState& state, bool open);

    /**
     * @brief 检查是否为铁活板门
     * @return 如果是铁活板门返回true
     */
    [[nodiscard]] bool isIronTrapdoor() const { return m_isIron; }

    // ========== 推动反应 ==========

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Destroy;
    }

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 获取流体状态
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 检查方块是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    // ========== 攀爬 ==========

    /**
     * @brief 检查方块是否可攀爬
     *
     * 打开的活板门可以攀爬。
     *
     * @param state 方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @param entity 实体（用于检查实体位置）
     * @return 如果活板门打开且实体在正确位置返回 true
     */
    [[nodiscard]] bool isLadder(const BlockState& state,
        IWorld* world = nullptr,
        const BlockPos* pos = nullptr,
        const Entity* entity = nullptr) const override;

private:
    /**
     * @brief 播放开关音效
     * @param world 世界
     * @param pos 方块位置
     * @param isOpening 是否正在打开
     */
    static void _playSound(IWorld& world, const BlockPos& pos, bool isOpening);

    /**
     * @brief 获取形状索引
     * @param facing 朝向
     * @param open 是否打开
     * @param half 上半/下半
     * @return 形状索引
     */
    [[nodiscard]] static size_t _getShapeIndex(Direction facing, bool open, BlockStateProperties::Half half);

    /// 是否为铁活板门
    bool m_isIron;

    /// 预计算的形状缓存 (4 facing * 2 open * 2 half = 16)
    std::array<CollisionShape, 16> m_shapes;
};

} // namespace blocks
} // namespace mc
