#include "JukeboxBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../blockentity/interactive/JukeboxEntity.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== JukeboxBlock 实现 ==========

JukeboxBlock::JukeboxBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HAS_RECORD())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::HAS_RECORD(), false));

    // 唱片机形状是完整方块
    m_shape = CollisionShape::fullBlock();
}

BlockState JukeboxBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

const CollisionShape& JukeboxBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

std::unique_ptr<BlockEntity> JukeboxBlock::createBlockEntity(const BlockPos& pos) {
    return std::make_unique<blockentity::JukeboxEntity>(pos);
}

int JukeboxBlock::getComparatorInputOverride(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos) const {

    // 从唱片机方块实体获取比较器信号
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::Jukebox) {
        auto* jukebox = static_cast<blockentity::JukeboxEntity*>(entity);
        return jukebox->getComparatorSignal();
    }

    // 有唱片时输出1，无唱片时输出0
    return hasRecord(state) ? 1 : 0;
}

void JukeboxBlock::setRecord(IWorld& world, const BlockPos& pos, BlockState& state, bool hasRecord) {
    BlockState newState = state.with(BlockStateProperties::HAS_RECORD(), hasRecord);
    world.setBlockState(pos.x, pos.y, pos.z, &newState, 3);

    // 更新方块实体
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::Jukebox) {
        auto* jukebox = static_cast<blockentity::JukeboxEntity*>(entity);
        if (hasRecord) {
            jukebox->startPlaying(world);
        } else {
            jukebox->stopPlaying(world);
        }
    }
}

} // namespace blocks
} // namespace mc
