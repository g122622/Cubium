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
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/IWaterLoggable.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 屏障方块
 *
 * 不可见的不可破坏方块，用于地图制作。
 * 只有创造模式玩家可以看到边界轮廓。
 *
 * vanilla 1.21.11 barrier 持有 waterlogged 属性（实现 SimpleWaterloggedBlock）。
 */
class BarrierBlock : public Block, public IWaterLoggable {
public:
    explicit BarrierBlock(const BlockProperties& properties);
    ~BarrierBlock() override = default;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 屏障方块不产生 AO 阴影
     *
     * 屏障是不可见方块，不应影响环境光遮蔽计算。
     * 虽然默认实现基于碰撞形状判断（屏障有完整碰撞形状），
     * 但 MC 原版明确重写此方法返回 1.0F。
     */
    [[nodiscard]] f32 getShadeBrightness(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return 1.0f;
    }

    // ========== IWaterLoggable 接口实现 ==========

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    /**
     * @brief 屏障透传天空光（对齐 vanilla BarrierBlock#propagatesSkylightDown）
     *
     * vanilla: `return state.getFluidState().isEmpty()`。屏障 getShape 虽是完整立方体，
     * 但 vanilla 显式 override 使其透传天空光（无水时=true）。Cubium 默认公式会因
     * getShape=fullBlock 得 false（错误），故此处 override 对齐 vanilla：
     * 无水时透传（opacity=0），含水时阻断（opacity=1，对齐含水屏障）。
     */
    [[nodiscard]] bool propagatesSkylightDown(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;
};

} // namespace blocks
} // namespace mc
