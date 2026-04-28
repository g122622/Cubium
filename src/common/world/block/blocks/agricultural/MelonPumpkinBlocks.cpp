#include "MelonPumpkinBlocks.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// MelonBlock
// ============================================================================

MelonBlock::MelonBlock(const Block* stem, const Block* attachedStem, const BlockProperties& properties)
    : StemGrownBlock(properties)
    , m_stem(stem)
    , m_attachedStem(attachedStem) {
    // 西瓜没有状态属性
}

// ============================================================================
// PumpkinBlock
// ============================================================================

PumpkinBlock::PumpkinBlock(const Block* stem, const Block* attachedStem, const Block* carvedPumpkin, const BlockProperties& properties)
    : StemGrownBlock(properties)
    , m_stem(stem)
    , m_attachedStem(attachedStem)
    , m_carvedPumpkin(carvedPumpkin) {
    // 南瓜没有状态属性
}

// TODO: 实现 onBlockActivated
// 需要检查剪刀物品类型

// ============================================================================
// CarvedPumpkinBlock
// ============================================================================

CarvedPumpkinBlock::CarvedPumpkinBlock(const BlockProperties& properties)
    : HorizontalBlock(properties) {
    // 创建状态容器，添加 HORIZONTAL_FACING 属性
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(FACING())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(FACING(), Direction::North));
}

BlockState CarvedPumpkinBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 放置时朝向玩家的反方向
    return defaultState().with(FACING(), Directions::opposite(context.horizontalDirection()));
}

void CarvedPumpkinBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(state);
    // 尝试生成傀儡（雪傀儡或铁傀儡）
    trySpawnGolem(world, pos);
}

void CarvedPumpkinBlock::trySpawnGolem(IWorld& world, const BlockPos& pos) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 实现傀儡生成逻辑
    // 检测下方两个铁块（铁傀儡）或两个雪块（雪傀儡）
    // 检测上方是否是南瓜/刻南瓜/南瓜灯
    // 如果条件满足，移除方块并生成傀儡实体
}

// ============================================================================
// JackOLanternBlock
// ============================================================================

JackOLanternBlock::JackOLanternBlock(const BlockProperties& properties)
    : HorizontalBlock(properties) {
    // 创建状态容器，添加 HORIZONTAL_FACING 属性
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(FACING())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(FACING(), Direction::North));
}

BlockState JackOLanternBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 放置时朝向玩家的反方向
    return defaultState().with(FACING(), Directions::opposite(context.horizontalDirection()));
}

} // namespace blocks
} // namespace mc
