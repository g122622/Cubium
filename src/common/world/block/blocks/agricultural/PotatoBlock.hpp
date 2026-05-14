#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "CropBlock.hpp"
#include <array>

namespace mc {
namespace blocks {

/**
 * @brief 马铃薯作物
 *
 * 8个生长阶段（AGE_0_7），成熟时掉落多个马铃薯，有几率掉落毒马铃薯。
 * 形状高度与胡萝卜相同。
 *
 * 参考: net.minecraft.block.PotatoBlock
 */
class PotatoBlock : public CropBlock {
public:
    explicit PotatoBlock(const BlockProperties& properties);
    ~PotatoBlock() override = default;

    [[nodiscard]] u32 getCropItem() const override;
    [[nodiscard]] u32 getSeedItem() const override;

    // 马铃薯形状与胡萝卜相同
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    std::array<CollisionShape, 8> m_potatoShapesByAge;
};

} // namespace blocks
} // namespace mc
