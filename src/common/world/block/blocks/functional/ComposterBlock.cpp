#include "ComposterBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== ComposterBlock 实现 ==========

ComposterBlock::ComposterBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::LEVEL_0_8())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::LEVEL_0_8(), 0));

    // 预计算各等级的形状
    // 堆肥桶内部填充高度随等级变化
    constexpr f32 P = 1.0f / 16.0f;

    // 外部形状是完整方块
    m_collisionShape = CollisionShape::fullBlock();

    // 内部空洞随等级变化
    // 等级0-7：内部空洞从下到上
    // 等级8：满的
    for (int i = 0; i < 8; ++i) {
        // 内部填充高度 = max(2, 1 + i * 2) 像素
        f32 fillHeight = static_cast<f32>(std::max(2, 1 + i * 2)) * P;
        // 形状 = 完整方块 - 内部空洞
        // 简化实现：只计算填充部分的形状
        m_shapesByLevel[i] = CollisionShape::box(
            2.0f * P, fillHeight, 2.0f * P,
            14.0f * P, 16.0f * P, 14.0f * P);
    }
    // 等级7和8形状相同
    m_shapesByLevel[8] = m_shapesByLevel[7];
}

BlockState ComposterBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

void ComposterBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    MC_UNUSED(random);
    int level = getLevel(state);
    if (level == 7) {
        // 等级7时，经过20 tick后变成等级8（可以收获骨粉）
        BlockState newState = state.with(BlockStateProperties::LEVEL_0_8(), 8);
        world.setBlockState(pos, &newState, 3);
        // TODO: 播放声音
    }
}

const CollisionShape& ComposterBlock::getShape(const BlockState& state) const {
    int level = getLevel(state);
    MC_ASSERT(level >= 0 && level <= 8);
    // 等级0时返回完整方块形状
    if (level == 0) {
        return m_collisionShape;
    }
    return m_shapesByLevel[level];
}

const CollisionShape& ComposterBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    // 碰撞箱始终是完整方块
    return m_collisionShape;
}

int ComposterBlock::getComparatorInputOverride(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos) const {

    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 比较器输出 = 等级
    return getLevel(state);
}

BlockState ComposterBlock::attemptCompost(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    u32 itemId) {

    int level = getLevel(state);
    if (level >= 7) {
        return state;  // 已满或正在完成
    }

    float chance = getCompostChance(itemId);
    if (chance <= 0.0f) {
        return state;  // 不可堆肥
    }

    // 概率性增加等级
    math::Random random;
    if (random.nextFloat() < chance) {
        int newLevel = level + 1;
        BlockState newState = state.with(BlockStateProperties::LEVEL_0_8(), newLevel);
        world.setBlockState(pos, &newState, 3);

        // 如果达到等级7，安排tick
        if (newLevel == 7) {
            // TODO: 安排tick
            // world.getPendingBlockTicks().scheduleTick(pos, this, 20);
        }

        return newState;
    }

    return state;
}

BlockState ComposterBlock::empty(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 生成骨粉物品
    // TODO: 掉落骨粉物品

    // 重置为等级0
    BlockState newState = state.with(BlockStateProperties::LEVEL_0_8(), 0);
    world.setBlockState(pos, &newState, 3);

    // TODO: 播放空声音

    return newState;
}

bool ComposterBlock::isCompostable(u32 itemId) {
    return getCompostChance(itemId) > 0.0f;
}

float ComposterBlock::getCompostChance(u32 itemId) {
    // TODO: 实现完整的堆肥概率表
    // 这里简化实现，返回-1表示不可堆肥
    // 实际应该检查物品类型：
    // - 树叶、树苗等: 0.3
    // - 甘蔗、藤蔓等: 0.5
    // - 苹果、蘑菇等: 0.65
    // - 干草块、面包等: 0.85
    // - 蛋糕、南瓜派: 1.0

    MC_UNUSED(itemId);
    return -1.0f;
}

} // namespace blocks
} // namespace mc
