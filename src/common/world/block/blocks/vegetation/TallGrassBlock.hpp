#pragma once

#include "../agricultural/BushBlock.hpp"
#include "../../../../util/property/Properties.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 高草方块
 *
 * 单格高的草类植物，可被放置在草方块、泥土等上。
 * 可使用剪刀采集，否则掉落小麦种子（概率）。
 *
 * 参考: net.minecraft.block.TallGrassBlock
 */
class TallGrassBlock : public BushBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit TallGrassBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~TallGrassBlock() override = default;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

protected:
    /**
     * @brief 检查下方是否可支撑
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState,
        IWorld& world,
        const BlockPos& groundPos) const override;
};

/**
 * @brief 蕨类方块
 *
 * 类似高草，但使用不同的材质。
 *
 * 参考: net.minecraft.block.FernBlock
 */
class FernBlock : public TallGrassBlock {
public:
    explicit FernBlock(const BlockProperties& properties);
};

} // namespace blocks
} // namespace mc
