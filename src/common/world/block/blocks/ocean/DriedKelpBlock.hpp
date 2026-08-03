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
#include "common/world/block/Block.hpp"

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;

namespace blocks {

/**
 * @brief 干海带块
 *
 * 由9个干海带合成的方块，可以作为燃料使用。
 * 可以分解为9个干海带物品。
 *
 * MC ID: minecraft:dried_kelp_block
 */
class DriedKelpBlock : public Block {
public:
    /**
     * @brief 构造干海带块
     */
    explicit DriedKelpBlock(BlockProperties properties);

    /**
     * @brief 获取燃烧时间
     * 干海带块可以作为燃料，燃烧时间为200tick（10秒）
     * @return 燃烧时间（tick），0表示不可燃
     */
    [[nodiscard]] i32 getBurnTime() const { return 200; }

    // 注意: 可燃性参数 (flammability=60, encouragement=30) 已移至 FireInfoRegistry 统一管理
    // 参见 FireInfoRegistry::initializeVanillaFireInfos() 中的注册
};

} // namespace blocks
} // namespace mc
