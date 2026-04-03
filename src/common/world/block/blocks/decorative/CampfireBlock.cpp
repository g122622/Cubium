#include "CampfireBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== CampfireBlock 实现 ==========

CampfireBlock::CampfireBlock(BlockProperties properties, u8 lightValue)
    : Block(std::move(properties))
    , m_lightValue(lightValue)
{
    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::LIT())
        .add(BlockStateProperties::SIGNAL_FIRE())
        .add(BlockStateProperties::WATERLOGGED())
        .add(BlockStateProperties::AGE_0_4())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::LIT(), true)
        .with(BlockStateProperties::SIGNAL_FIRE(), false)
        .with(BlockStateProperties::WATERLOGGED(), false)
        .with(BlockStateProperties::AGE_0_4(), 0));

    // 营火形状（略小于完整方块）
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f, 7.0f, 16.0f);
}

BlockState CampfireBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 默认点燃，非信号火，非水淹
    return defaultState()
        .with(BlockStateProperties::LIT(), true)
        .with(BlockStateProperties::SIGNAL_FIRE(), false)
        .with(BlockStateProperties::WATERLOGGED(), false);
}

void CampfireBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 如果被水淹没，熄灭
    if (isWaterlogged(state)) {
        extinguish(world, pos, state);
        return;
    }

    // TODO: 检查雨水（如果上方无遮挡，增加AGE）
    // 如果AGE达到4，熄灭
    // 需要天气系统支持

    // TODO: 烹饪食物逻辑
    // 需要方块实体支持
}

const CollisionShape& CampfireBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

u8 CampfireBlock::getLightLevel(
    const BlockState& state,
    IWorld* world,
    const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 点燃时发出光照，熄灭时不发光
    if (isLit(state)) {
        return m_lightValue;
    }
    return 0;
}

void CampfireBlock::light(IWorld& world, const BlockPos& pos, BlockState& state) {
    if (!isLit(state) && !isWaterlogged(state)) {
        BlockState newState = state.with(BlockStateProperties::LIT(), true);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 3);
        // TODO: 播放点燃音效和粒子效果
    }
}

void CampfireBlock::extinguish(IWorld& world, const BlockPos& pos, BlockState& state) {
    if (isLit(state)) {
        BlockState newState = state
            .with(BlockStateProperties::LIT(), false)
            .with(BlockStateProperties::AGE_0_4(), 0);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 3);
        // TODO: 播放熄灭音效和烟雾粒子效果
    }
}

// ========== SoulCampfireBlock 实现 ==========

SoulCampfireBlock::SoulCampfireBlock(BlockProperties properties)
    : CampfireBlock(std::move(properties), 10)  // 灵魂营火光照等级为10
{
}

} // namespace blocks
} // namespace mc
