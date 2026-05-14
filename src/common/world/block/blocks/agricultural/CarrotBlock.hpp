#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "CropBlock.hpp"
#include <array>

namespace mc {
namespace blocks {

/**
 * @brief 胡萝卜作物
 *
 * 8个生长阶段（AGE_0_7），成熟时掉落多个胡萝卜。
 * 形状高度比小麦低：2, 3, 4, 5, 6, 7, 8, 9 像素。
 *
 * 参考: net.minecraft.block.CarrotBlock
 */
class CarrotBlock : public CropBlock {
public:
    explicit CarrotBlock(const BlockProperties& properties);
    ~CarrotBlock() override = default;

    [[nodiscard]] u32 getCropItem() const override;
    [[nodiscard]] u32 getSeedItem() const override;

    // 胡萝卜形状不同，需要覆盖
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    std::array<CollisionShape, 8> m_carrotShapesByAge;
};

} // namespace blocks
} // namespace mc
