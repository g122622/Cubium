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

#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"

namespace mc {

class IWorld;

namespace blocks {

/**
 * @brief 湿润海绵方块
 *
 * 海绵吸水后的状态，需要烤干才能再次吸水。
 *
 * 特殊行为：
 * - 在下界放置时会变干（变成普通海绵）
 * - 产生蒸汽粒子效果和火焰熄灭音效
 */
class WetSpongeBlock : public Block {
public:
    explicit WetSpongeBlock(const BlockProperties& properties);
    ~WetSpongeBlock() override = default;

    // ========== 方块回调 ==========

    /**
     * @brief 方块被放置时的处理
     *
     * 在下界放置时会变干并产生蒸汽效果。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;
};

} // namespace blocks
} // namespace mc
