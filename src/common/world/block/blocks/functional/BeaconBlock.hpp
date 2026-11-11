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
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Player;

namespace blocks {

/**
 * @brief 信标方块
 *
 * 提供增益效果的功能方块。
 * 需要金字塔基座才能激活，基座层数决定可用效果。
 *
 */
class BeaconBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit BeaconBlock(const BlockProperties& properties);
    ~BeaconBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 光照 ==========

    /**
     * @brief 获取光照等级
     *
     * 信标始终发出15级光照。
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 位置（可选）
     * @return 光照等级 (15)
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return 15;
    }

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 红石 ==========

    // 不重写比较器相关方法：vanilla BeaconBlock 既不是 isSignalSource，也不重写
    // hasAnalogOutputSignal——信标不向比较器输出读数（「输出金字塔等级」是常见误解）。

    // ========== 方块实体 ==========

    // 信标是方块实体方块：放置时需创建 BeaconEntity，承载金字塔等级与效果应用等状态。
    // 缺少此重写会落到基类 hasBlockEntity() 默认 false，ServerWorld::setBlockState 不会创建方块实体，
    // 导致 tick 不会执行、右键打不开 GUI。
    [[nodiscard]] bool hasBlockEntity() const noexcept override
    {
        return true;
    }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 交互 ==========

    /**
     * @brief 处理玩家右键交互
     *
     * 打开信标GUI并记录交互统计。
     * 参考: MC Java BeaconBlock.useWithoutItem()
     */
    [[nodiscard]] std::optional<ContainerType> getMenuProvider(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockActionResult useWithoutItem(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        const BlockRaycastResult& hit) override;

protected:
    /// 信标形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
