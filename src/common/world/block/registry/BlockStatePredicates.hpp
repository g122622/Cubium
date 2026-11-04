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
 */

#pragma once

#include "common/world/block/Block.hpp"

namespace mc {
namespace block_registry {

class BlockStatePredicates {
public:
    BlockStatePredicates() = delete;

    // ========================================================================
    // 通用常量谓词
    // ========================================================================

    /** @brief 恒为 false（原版 `Blocks.never`）。 */
    static const StatePredicate& never();

    /** @brief 恒为 true（原版 `Blocks.always`）。 */
    static const StatePredicate& always();

    // ========================================================================
    // 窒息 / 视线谓词
    // ========================================================================
    //
    // 原版把这两项挂在 `BlockBehaviour.Properties` 上，默认是同一个谓词实例
    // `blocksMotion() && isCollisionShapeFullBlock()`。下列常量与原版逐项对应，名字取自
    // 原版 `Blocks.java` 里注册点的实际写法。

    /**
     * @brief 活塞：未伸出时挡视线并造成窒息
     *
     * 原版 `Blocks.NOT_EXTENDED_PISTON`。伸出后活塞头占满整格反而不再遮住视线判定——
     * 这条看似反直觉，但与 `PistonHeadBlock` 的 `noOcclusion` 注册一致。
     */
    static const StatePredicate& notExtendedPiston();

    /** @brief 雪层：满层（`layers >= 8`）才挡视线，永不造成窒息。 */
    static const StatePredicate& snowFullLayersViewBlocking();

    /** @brief 耕地：恒挡视线、恒造成窒息（碰撞不满格，默认判据会给 false）。 */
    static const StatePredicate& alwaysBlocking();

    /** @brief 泥土路径：恒挡视线、恒造成窒息。 */
    static const StatePredicate& dirtPathBlocking();

    /** @brief 灵魂沙：恒挡视线、恒造成窒息（碰撞比满格矮，但遮挡形状满格）。 */
    static const StatePredicate& soulSandBlocking();

    /** @brief 泥巴：恒挡视线、恒造成窒息。 */
    static const StatePredicate& mudBlocking();

    /** @brief 移动中的活塞：恒定不挡视线、不造成窒息（原版 `Blocks::never`）。 */
    static const StatePredicate& movingPistonNever();

    /** @brief 奶昔状试炼刷怪笼/宝库：只造成窒息、不挡视线。 */
    static const StatePredicate& trialSpawnerNeverView();

    /** @brief 测试实例方块：只造成窒息、不挡视线（原版 `Blocks::always` 用于窒息、`never` 用于视线）。 */
    static const StatePredicate& testInstanceNeverView();
};

} // namespace block_registry
} // namespace mc
