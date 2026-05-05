#pragma once

#include "DispenserBlockEntity.hpp"

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
 *
 * 参考: net.minecraft.tileentity.DropperTileEntity
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
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

protected:
    /**
     * @brief 获取默认显示名称
     * @return 投掷器的显示名称
     */
    [[nodiscard]] String getDefaultName() const override { return "container.dropper"; }
};

} // namespace blockentity
} // namespace mc
