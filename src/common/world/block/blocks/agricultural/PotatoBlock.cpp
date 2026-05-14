#include "PotatoBlock.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

PotatoBlock::PotatoBlock(const BlockProperties& properties)
    : CropBlock(properties)
{

    // 预计算马铃薯各生长阶段的形状
    // 高度与胡萝卜相同：2, 3, 4, 5, 6, 7, 8, 9 像素
    constexpr f32 P = 1.0f / 16.0f;
    constexpr f32 heights[] = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};

    for (int i = 0; i < 8; ++i) {
        m_potatoShapesByAge[i] = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f * P, heights[i] * P, 16.0f * P);
    }
}

u32 PotatoBlock::getCropItem() const
{
    // 马铃薯的作物和种子是同一个物品
    // 参考: net.minecraft.block.PotatoBlock#getCropItem
    return Items::POTATO->itemId();
}

u32 PotatoBlock::getSeedItem() const
{
    // 马铃薯的作物和种子是同一个物品
    return Items::POTATO->itemId();
}

const CollisionShape& PotatoBlock::getShape(const BlockState& state) const
{
    int age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 7);
    return m_potatoShapesByAge[age];
}

} // namespace blocks
} // namespace mc
