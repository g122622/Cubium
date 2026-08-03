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
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "world/blockentity/storage/ChestEntity.hpp"
#include <memory>
#include <string>

namespace mc {
namespace blockentity {

/**
 * @brief 陷阱箱方块实体
 *
 * 继承自箱子实体，额外提供红石信号输出功能。
 * 输出的红石信号强度等于打开箱子的玩家数量（最大15）。
 *
 * 参考: net.minecraft.tileentity.TrappedChestTileEntity
 */
class TrappedChestEntity : public ChestEntity {
public:
    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit TrappedChestEntity(const BlockPos& pos);

    /**
     * @brief 创建方块实体副本
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    /**
     * @brief 获取红石信号强度
     * @param world 世界引用
     * @return 信号强度 (0-15)，等于打开玩家数
     */
    [[nodiscard]] i32 getRedstoneSignal(IWorld& world) const;

    // ========== 重写打开/关闭方法 ==========

    void openContainer(Player* player) override;
    void closeContainer(Player* player) override;

protected:
    [[nodiscard]] std::string getDefaultName() const override { return "container.chestTrapped"; }

private:
    /**
     * @brief 通知邻居方块更新红石信号
     * @param world 世界引用
     */
    void _notifyNeighbors(IWorld& world);
};

} // namespace blockentity
} // namespace mc
