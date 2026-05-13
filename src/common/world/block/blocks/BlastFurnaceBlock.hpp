#pragma once

#include "AbstractFurnaceBlock.hpp"
#include "../../blockentity/BlockEntityType.hpp"
#include <memory>

namespace mc {
namespace blocks {

/**
 * @brief 高炉方块
 *
 * 实现100tick熔炼时间的熔炉方块。
 * 只能熔炼矿石和金属物品。
 *
 * 参考: net.minecraft.block.BlastFurnaceBlock
 */
class BlastFurnaceBlock : public AbstractFurnaceBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit BlastFurnaceBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~BlastFurnaceBlock() override = default;

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
    [[nodiscard]] BlockEntityType getBlockEntityType() const override {
        return BlockEntityType::BlastFurnace;
    }

protected:
    /**
     * @brief 与熔炉交互
     */
    [[nodiscard]] bool interactWith(IWorld& world, const BlockPos& pos, Player& player) override;
};

} // namespace blocks
} // namespace mc
