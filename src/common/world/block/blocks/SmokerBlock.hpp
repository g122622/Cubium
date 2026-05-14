#pragma once

#include "../../blockentity/BlockEntityType.hpp"
#include "AbstractFurnaceBlock.hpp"
#include <memory>

namespace mc {
namespace blocks {

/**
 * @brief 烟熏炉方块
 *
 * 实现100tick熔炼时间的熔炉方块。
 * 只能烹饪食物。
 *
 * 参考: net.minecraft.block.SmokerBlock
 */
class SmokerBlock : public AbstractFurnaceBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit SmokerBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~SmokerBlock() override = default;

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
    [[nodiscard]] BlockEntityType getBlockEntityType() const override { return BlockEntityType::Smoker; }

protected:
    /**
     * @brief 与熔炉交互
     */
    [[nodiscard]] bool interactWith(IWorld& world, const BlockPos& pos, Player& player) override;
};

} // namespace blocks
} // namespace mc
