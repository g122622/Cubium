#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "CropBlock.hpp"
#include <array>

namespace mc {
namespace blocks {

/**
 * @brief 甜菜根作物
 *
 * 4个生长阶段（AGE_0_3），成熟时掉落甜菜根和甜菜根种子。
 * 形状高度：2, 4, 6, 8 像素。
 * 生长速度比其他作物慢（有 1/3 概率跳过生长）。
 *
 * 参考: net.minecraft.block.BeetrootBlock
 */
class BeetrootBlock : public CropBlock {
public:
    explicit BeetrootBlock(const BlockProperties& properties);
    ~BeetrootBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取年龄属性（甜菜根使用 AGE_0_3）
     */
    [[nodiscard]] const IntegerProperty& getAgeProperty() const override;

    /**
     * @brief 获取最大年龄（甜菜根为 3）
     */
    [[nodiscard]] int getMaxAge() const override { return 3; }

    // ========== 生长逻辑 ==========

    /**
     * @brief 随机刻（甜菜根生长较慢）
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 骨粉增加的年龄（甜菜根较少）
     */
    [[nodiscard]] int getBonemealAgeIncrease(IWorld& world, const BlockPos& pos) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 掉落物 ==========

    [[nodiscard]] u32 getCropItem() const override;
    [[nodiscard]] u32 getSeedItem() const override;

private:
    std::array<CollisionShape, 4> m_beetrootShapesByAge;
};

} // namespace blocks
} // namespace mc
