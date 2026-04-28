#include "CropBlocks.hpp"
#include "../../../IWorld.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// WheatBlock
// ============================================================================

WheatBlock::WheatBlock(const BlockProperties& properties)
    : CropBlock(properties) {
    // 小麦使用 CropBlock 的默认形状
}

u32 WheatBlock::getCropItem() const {
    // 返回小麦物品ID
    // 注：需要在 Items 初始化后才能访问
    return 0; // TODO: 返回 Items::WHEAT 的 ID
}

u32 WheatBlock::getSeedItem() const {
    // 返回小麦种子物品ID
    return 0; // TODO: 返回 Items::WHEAT_SEEDS 的 ID
}

// ============================================================================
// CarrotBlock
// ============================================================================

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
    return 0; // TODO: 返回 Items::CARROT 的 ID
}

u32 CarrotBlock::getSeedItem() const {
    // 胡萝卜的作物和种子是同一个物品
    return 0; // TODO: 返回 Items::CARROT 的 ID
}

const CollisionShape& CarrotBlock::getShape(const BlockState& state) const {
    int age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 7);
    return m_carrotShapesByAge[age];
}

// ============================================================================
// PotatoBlock
// ============================================================================

PotatoBlock::PotatoBlock(const BlockProperties& properties)
    : CropBlock(properties) {

    // 预计算马铃薯各生长阶段的形状
    // 高度与胡萝卜相同：2, 3, 4, 5, 6, 7, 8, 9 像素
    constexpr f32 P = 1.0f / 16.0f;
    constexpr f32 heights[] = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};

    for (int i = 0; i < 8; ++i) {
        m_potatoShapesByAge[i] = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f * P, heights[i] * P, 16.0f * P);
    }
}

u32 PotatoBlock::getCropItem() const {
    // 马铃薯的作物和种子是同一个物品
    return 0; // TODO: 返回 Items::POTATO 的 ID
}

u32 PotatoBlock::getSeedItem() const {
    // 马铃薯的作物和种子是同一个物品
    return 0; // TODO: 返回 Items::POTATO 的 ID
}

const CollisionShape& PotatoBlock::getShape(const BlockState& state) const {
    int age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 7);
    return m_potatoShapesByAge[age];
}

// ============================================================================
// BeetrootBlock
// ============================================================================

BeetrootBlock::BeetrootBlock(const BlockProperties& properties)
    : CropBlock(properties) {

    // 预计算甜菜根各生长阶段的形状
    // 只有 4 个阶段，高度：2, 4, 6, 8 像素
    constexpr f32 P = 1.0f / 16.0f;
    constexpr f32 heights[] = {2.0f, 4.0f, 6.0f, 8.0f};

    for (int i = 0; i < 4; ++i) {
        m_beetrootShapesByAge[i] = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f * P, heights[i] * P, 16.0f * P);
    }
}

const IntegerProperty& BeetrootBlock::getAgeProperty() const {
    // 甜菜根使用 AGE_0_3 属性
    return BlockStateProperties::AGE_0_3();
}

void BeetrootBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 参考: net.minecraft.block.BeetrootBlock#randomTick
    // 甜菜根有 1/3 概率跳过生长检查

    // 如果已经成熟，不需要生长
    if (isMaxAge(state)) {
        return;
    }

    // 甜菜根有 1/3 概率跳过
    // 参考: if (worldIn.getRandom().nextInt(3) != 0)
    if (random.nextInt(3) == 0) {
        return;
    }

    // 光照检查
    const BlockPos abovePos = pos.up();
    const i32 blockLight = static_cast<i32>(world.getBlockLight(abovePos));
    const i32 skyLight = static_cast<i32>(world.getSkyLight(abovePos));
    if (std::max(blockLight, skyLight) < 9) {
        return;
    }

    // 计算生长概率
    const f32 growthChance = std::max(1.0f, getGrowthChance(*this, static_cast<IBlockReader&>(world), pos));
    const i32 randomBound = static_cast<i32>(25.0f / growthChance) + 1;
    if (random.nextInt(randomBound) == 0) {
        world.setBlockState(pos, &withAge(getAge(state) + 1), 2);
    }
}

int BeetrootBlock::getBonemealAgeIncrease(IWorld& world, const BlockPos& pos) const {
    // 甜菜根骨粉增加的生长阶段较少
    // 参考: net.minecraft.block.BeetrootBlock#getBonemealAgeIncrease
    // 返回父类的 1/3（约 0-1，因为父类返回 2-5）
    // 实际上，甜菜根骨粉只增加 1 个阶段
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return 1;
}

const CollisionShape& BeetrootBlock::getShape(const BlockState& state) const {
    int age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 3);
    return m_beetrootShapesByAge[age];
}

u32 BeetrootBlock::getCropItem() const {
    // 返回甜菜根物品ID
    return 0; // TODO: 返回 Items::BEETROOT 的 ID
}

u32 BeetrootBlock::getSeedItem() const {
    // 返回甜菜根种子物品ID
    return 0; // TODO: 返回 Items::BEETROOT_SEEDS 的 ID
}

} // namespace blocks
} // namespace mc
