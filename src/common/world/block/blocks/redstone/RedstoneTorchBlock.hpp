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
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/redstone/RedstonePower.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 红石火把方块
 *
 * 红石信号反转器，当下方有信号输入时熄灭。
 * 熄灭时输出0，点亮时输出15强度信号。
 *
 * ## 特性
 * - 信号反转：下方有信号时熄灭
 * - 输出强度：点亮时15，熄灭时0
 * - 烧毁机制：60 tick内翻转8次则烧毁，冷却160 tick
 * - 弱信号输出：从四周和上方输出
 *
 * ## 容易踩的坑
 * - 红石火把不向下输出信号
 * - 烧毁时需要记录历史翻转
 * - 更新时需要防止无限递归
 */
class RedstoneTorchBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RedstoneTorchBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 获取发光等级 - 点亮时为7，熄灭时为0
     *
     * 红石火把发光随 LIT 状态动态变化（被充能或烧毁时熄灭）。注册时移除静态 lightLevel(7)，
     * 此处按 LIT 属性返回，对齐 net.minecraft.block.RedstoneTorchBlock：lit 时发出 7 级方块光，
     * unlit（熄灭/烧毁）时不发光。此前注册期静态 lightLevel(7) 使熄灭火把仍发光 7，与原版不符。
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return isLit(state) ? 7 : 0;
    }

    // ========== 红石接口 ==========

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    /**
     * @brief 获取强信号强度
     *
     * 红石火把只在向下方向输出强信号。
     * 这使得红石火把可以充能其下方的方块。
     *
     * @param state 方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @param side 方向
     * @return i32 强信号强度
     */
    [[nodiscard]] i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 红石火把特有方法 ==========

    /**
     * @brief 检查火把是否应该熄灭
     *
     * 当下方方块被充能时，火把应该熄灭。
     *
     * @param world 世界引用
     * @param pos 火把位置
     * @return true 如果应该熄灭
     */
    [[nodiscard]] bool shouldBeOff(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 检查火把是否点亮
     *
     * @param state 方块状态
     * @return true 如果点亮
     */
    [[nodiscard]] static bool isLit(const BlockState& state);

protected:
    /**
     * @brief 更新火把状态
     *
     * 检查是否需要改变点亮/熄灭状态，
     * 如果需要则调度延迟更新。
     *
     * @param world 世界引用
     * @param pos 火把位置
     * @param state 当前方块状态
     */
    void updateState(IWorld& world, const BlockPos& pos, const BlockState& state);
};

} // namespace blocks
} // namespace mc
