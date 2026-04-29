#pragma once

#include "world/blockentity/storage/ChestEntity.hpp"

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
    [[nodiscard]] String getDefaultName() const override { return "container.chestTrapped"; }

private:
    /**
     * @brief 通知邻居方块更新红石信号
     * @param world 世界引用
     */
    void notifyNeighbors(IWorld& world);
};

} // namespace blockentity
} // namespace mc
