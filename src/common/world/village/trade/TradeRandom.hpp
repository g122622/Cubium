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

#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {
namespace world {
namespace village {
namespace trade {

/**
 * @brief 带派生能力的交易随机源
 *
 * vanilla 交易生成使用 `RandomSource` 的 `fork()`：每次为某个列表项抽取随机数前派生一条
 * 独立子流，使"某一项消耗了多少随机数"不影响其他项的随机序列（也保证同一实体重放时
 * 结果稳定）。本类把该语义映射到项目的 IRandom 之上：
 * - fromSeed：离线场景（无世界）按种子构造一棵确定性的随机树；
 * - nextChild：基于父流的下一个 64 位值派生同类型子流。
 */
class TradeRandom : public math::IRandom {
public:
    /**
     * @brief 以种子构造随机源
     * @param seed 种子
     */
    explicit TradeRandom(u64 seed);

    /**
     * @brief 拷贝构造（父流派生时复制一次引擎状态）
     */
    TradeRandom(const TradeRandom& other);

    /**
     * @brief 拷贝赋值
     */
    TradeRandom& operator=(const TradeRandom& other);

    /**
     * @brief 以种子重置状态
     * @param seed 种子
     */
    void setSeed(u64 seed) override;

    /**
     * @brief 返回 [0, UINT64_MAX] 范围的随机数
     */
    [[nodiscard]] u64 nextU64() override;

    /**
     * @brief 派生一个位置随机工厂
     */
    [[nodiscard]] math::PositionalRandomFactory forkPositional() override;

    /**
     * @brief 派生一条与父流独立、但可复现的子流
     * @return 新子流（调用方持有）
     */
    [[nodiscard]] TradeRandom nextChild();

private:
    /// 底层引擎（Xoroshiro128++，与 MC Java LogicalRandomSource 同属 xoroshiro 家族）
    math::Xoroshiro128ppRandom m_engine;
};

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc