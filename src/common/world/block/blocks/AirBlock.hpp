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

#include "../Block.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {

/**
 * @brief 空气方块
 *
 * 无碰撞、非固体、非不透明的方块。
 * 参考: net.minecraft.block.AirBlock
 */
class AirBlock : public Block {
public:
    /**
     * @brief 构造空气方块
     */
    explicit AirBlock(BlockProperties properties);

    /**
     * @brief 获取渲染形状（空）
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（空）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 是否为空气（始终返回true）
     */
    [[nodiscard]] bool isAir(const BlockState& state) const override;

    /**
     * @brief 是否不透明（始终返回false）
     */
    [[nodiscard]] bool isOpaque(const BlockState& state) const override;
};

} // namespace mc
