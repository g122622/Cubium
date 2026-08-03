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

#include "DispenserBlockEntity.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include <memory>
#include <string>

namespace mc {
namespace blockentity {

/**
 * @brief 投掷器方块实体
 *
 * 继承自 DispenserBlockEntity，提供9格物品存储和随机选择物品投掷的功能。
 * 与发射器的区别：
 * - 投掷器只投掷物品，没有特殊行为
 * - 发射器对特定物品有特殊行为（如箭矢发射、火焰球等）
 * - 投掷器会尝试向相邻容器输出物品
 */
class DropperBlockEntity : public DispenserBlockEntity {
public:
    /**
     * @brief 构造函数
     * @param pos 位置
     */
    explicit DropperBlockEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~DropperBlockEntity() override = default;

    /**
     * @brief 创建方块实体副本
     * @return 克隆的投掷器方块实体
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

protected:
    /**
     * @brief 获取默认显示名称
     * @return 投掷器的显示名称
     */
    [[nodiscard]] std::string getDefaultName() const override { return "container.dropper"; }
};

} // namespace blockentity
} // namespace mc
