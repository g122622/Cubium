#include "CarrotBlock.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../../item/Items.hpp"

namespace mc {
namespace blocks {

CarrotBlock::CarrotBlock(const BlockProperties& properties)
    : CropBlock(properties) {

    // 预计算胡萝卜各生长阶段的形状
    // 高度：2, 3, 4, 5, 6, 7, 8, 9 像素
    constexpr f32 P = 1.0f / 16.0f;
    constexpr f32 heights[] = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};

    for (int i = 0; i < 8; ++i) {
        m_carrotShapesByAge[i] = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f * P, heights[i] * P, 16.0f * P);
    }
}

u32 CarrotBlock::getCropItem() const {
    // 胡萝卜的作物和种子是同一个物品
    // 参考: net.minecraft.block.CarrotBlock#getCropItem
    return Items::CARROT->itemId();
}

u32 CarrotBlock::getSeedItem() const {
    // 胡萝卜的作物和种子是同一个物品
    return Items::CARROT->itemId();
}

const CollisionShape& CarrotBlock::getShape(const BlockState& state) const {
    int age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 7);
    return m_carrotShapesByAge[age];
}

} // namespace blocks
} // namespace mc
