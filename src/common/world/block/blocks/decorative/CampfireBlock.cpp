#include "CampfireBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../WaterLoggableHelpers.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../sound/SoundCategory.hpp"
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
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查是否含水
    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    // 如果在水中，默认不点燃
    bool lit = !waterlogged;

    // 默认点燃，非信号火
    return defaultState()
        .with(BlockStateProperties::LIT(), lit)
        .with(BlockStateProperties::SIGNAL_FIRE(), false)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState CampfireBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 处理含水状态
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

void CampfireBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    MC_UNUSED(random);
    // 如果被水淹没，熄灭
    if (state.get(BlockStateProperties::WATERLOGGED())) {
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
    if (!isLit(state) && !state.get(BlockStateProperties::WATERLOGGED())) {
        BlockState newState = state.with(BlockStateProperties::LIT(), true);
        world.setBlockState(pos, &newState, 3);
        // MC 1.16.5: 点燃音效
        // 参考: CampfireBlock.onBlockAdded 和 FireBlock.onBlockAdded
        // 注: 点燃音效使用通用的火焰点燃声，此处不播放特定音效
        // 粒子效果由客户端渲染器处理
    }
}

void CampfireBlock::extinguish(IWorld& world, const BlockPos& pos, BlockState& state) {
    if (isLit(state)) {
        BlockState newState = state
            .with(BlockStateProperties::LIT(), false)
            .with(BlockStateProperties::AGE_0_4(), 0);
        world.setBlockState(pos, &newState, 3);

        // MC 1.16.5: 熄灭时播放音效
        // 参考: CampfireBlock.receiveFluid 和 extinguish 方法
        // 注: 原版使用 ENTITY_GENERIC_EXTINGUISH_FIRE 音效
        if (!world.isClientSide()) {
            world.playSound(
                SoundEvents::ENTITY_GENERIC_EXTINGUISH_FIRE,
                sound::SoundCategory::Blocks,
                pos.center(),
                1.0f,
                1.0f
            );
        }
    }
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* CampfireBlock::getFluidState(const BlockState& state) const {
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

// ========== SoulCampfireBlock 实现 ==========

SoulCampfireBlock::SoulCampfireBlock(BlockProperties properties)
    : CampfireBlock(std::move(properties), 10)  // 灵魂营火光照等级为10
{
}

} // namespace blocks
} // namespace mc
