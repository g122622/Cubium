#pragma once

#include "../../blockentity/BlockEntityType.hpp"
#include "AbstractFurnaceBlock.hpp"
#include <memory>

namespace mc {
namespace blocks {

/**
 * @brief 普通熔炉方块
 *
 * 实现200tick熔炼时间的熔炉方块。
 *
 * 参考: net.minecraft.block.FurnaceBlock
 */
class FurnaceBlock : public AbstractFurnaceBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit FurnaceBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~FurnaceBlock() override = default;

    // ========== 方块实体 ==========

    /**
     * @brief 创建方块实体
     * @param pos 方块位置
     * @return 方块实体
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    /**
     * @brief 获取方块实体类型
     */
    [[nodiscard]] BlockEntityType getBlockEntityType() const override { return BlockEntityType::Furnace; }

protected:
    /**
     * @brief 与熔炉交互
     */
    [[nodiscard]] bool interactWith(IWorld& world, const BlockPos& pos, Player& player) override;
};

} // namespace blocks
} // namespace mc
