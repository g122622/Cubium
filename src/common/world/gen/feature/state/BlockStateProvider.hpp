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
#include "common/util/math/random/IRandom.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>

namespace mc {
class IWorld;
}

namespace mc::world::gen::feature::state {

/**
 * @brief 方块状态提供者基类
 *
 * 数据驱动方块状态提供者体系的统一多态基类。8 种子类（Simple/Weighted/RuleBased/
 * Rotated/NoiseThreshold/Noise/DualNoise/RandomizedInt）均继承本基类，由
 * BlockStateProviderParser 按 JSON 的 "type" 字段构造对应子类实例。
 *
 * getState 统一带 IWorld 参数：RuleBased 的谓词测试与 RandomizedInt 的属性查找
 * 需要世界上下文；其余子类忽略该参数。
 */
class BlockStateProvider {
public:
    virtual ~BlockStateProvider() = default;

    /**
     * @brief 获取方块状态
     * @param world 世界（RuleBased 谓词测试 / RandomizedInt 属性查找用，其余子类忽略）
     * @param random 随机数生成器
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 方块状态；提供者无可用状态时返回 nullptr
     */
    [[nodiscard]] virtual const BlockState* getState(
        const IWorld& world, math::IRandom& random, i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 取单一状态（解析期降级用）
     *
     * 仅 Simple 子类返回其固定状态；其余子类返回 nullptr（需随机源/世界才能采样）。
     */
    [[nodiscard]] virtual const BlockState* asSingleState() const noexcept { return nullptr; }

    /**
     * @brief 深拷贝当前提供者（含递归子提供者）
     *
     * 配置结构的拷贝构造/赋值通过 clone() 实现多态深拷贝，避免浅拷贝 double-free。
     */
    [[nodiscard]] virtual std::unique_ptr<BlockStateProvider> clone() const = 0;
};

/**
 * @brief 固定方块状态提供者
 *
 * 始终返回同一个方块状态。
 */
class SimpleBlockStateProvider : public BlockStateProvider {
public:
    explicit SimpleBlockStateProvider(const BlockState* state);

    [[nodiscard]] const BlockState* getState(
        const IWorld& world, math::IRandom& random, i32 x, i32 y, i32 z) const override;

    [[nodiscard]] const BlockState* asSingleState() const noexcept override;

    [[nodiscard]] std::unique_ptr<BlockStateProvider> clone() const override;

private:
    const BlockState* m_state;
};

} // namespace mc::world::gen::feature::state
