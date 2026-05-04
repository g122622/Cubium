#include "RespawnAnchorBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../dimension/DimensionType.hpp"
#include "../../../explosion/ExplosionMode.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../sound/SoundCategory.hpp"

namespace mc {
namespace blocks {

// ========== RespawnAnchorBlock 实现 ==========

RespawnAnchorBlock::RespawnAnchorBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::CHARGES_0_4())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::CHARGES_0_4(), 0));

    // 重生锚形状是完整方块
    m_shape = CollisionShape::fullBlock();
}

BlockState RespawnAnchorBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

void RespawnAnchorBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 重生锚的tick处理
    // 当前没有特殊的tick逻辑
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
}

void RespawnAnchorBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 在下界之外，充能的重生锚可能会爆炸
    // TODO: 检查维度，如果不是下界则爆炸
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
}

const CollisionShape& RespawnAnchorBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

int RespawnAnchorBlock::getComparatorInputOverride(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos) const {

    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 比较器输出 = 充能等级
    return getCharges(state);
}

u8 RespawnAnchorBlock::getLightLevel(
    const BlockState& state,
    IWorld* world,
    const BlockPos* pos) const {

    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 光照等级 = charges * 3.75，向下取整
    // 0 -> 0, 1 -> 3, 2 -> 7, 3 -> 11, 4 -> 15
    int charges = getCharges(state);
    return static_cast<u8>(std::floor(charges * 3.75f));
}

BlockState RespawnAnchorBlock::charge(IWorld& world, const BlockPos& pos, BlockState& state) {
    int charges = getCharges(state);
    if (charges < 4) {
        BlockState newState = state.with(BlockStateProperties::CHARGES_0_4(), charges + 1);
        world.setBlockState(pos, &newState, 3);
        // TODO: 播放充能音效和粒子效果
        return newState;
    }
    return state;
}

void RespawnAnchorBlock::discharge(IWorld& world, const BlockPos& pos, BlockState& state) {
    int charges = getCharges(state);
    if (charges > 0) {
        BlockState newState = state.with(BlockStateProperties::CHARGES_0_4(), charges - 1);
        world.setBlockState(pos, &newState, 3);
    }
}

ActionResultType RespawnAnchorBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(hit);

    // 获取维度信息
    DimensionType dimType = DimensionType::fromId(world.dimension());

    // 获取手持物品
    ItemStack& heldItem = player.getHeldItem(hand);

    // 检查是否用萤石充能
    // MC Java: 检查物品是否对应 GLOWSTONE 方块
    bool hasGlowstone = false;
    if (!heldItem.isEmpty()) {
        const Item* item = heldItem.getItem();
        if (item != nullptr) {
            const Block* block = BlockItemRegistry::instance().getBlock(item->itemId());
            hasGlowstone = (block == VanillaBlocks::GLOWSTONE);
        }
    }

    if (hasGlowstone && getCharges(state) < 4) {
        // 充能
        BlockState newState = charge(world, pos, const_cast<BlockState&>(state));

        // 播放充能音效
        world.playSound(
            ResourceLocation("minecraft:block.respawn_anchor.charge"),
            sound::SoundCategory::Blocks,
            pos.center(),
            1.0f,
            1.0f
        );

        // 消耗萤石
        heldItem.shrink(1);

        return ActionResultType::Success;
    }

    // 检查重生锚是否在此维度可用
    if (!dimType.respawnAnchorWorks()) {
        // 在非下界使用重生锚会爆炸
        // 移除重生锚
        world.setBlockState(pos, nullptr, 11);

        // MC 1.16.5: 重生锚爆炸强度为 5.0，破坏方块但不生成火焰
        // 参考: net.minecraft.block.RespawnAnchorBlock.onBlockActivated
        world.createExplosion(
            pos.center(),
            5.0f,  // 爆炸半径
            world::explosion::ExplosionMode::Destroy,
            false   // 不生成火焰
        );

        return ActionResultType::Success;
    }

    // 在下界使用重生锚设置重生点
    if (getCharges(state) > 0) {
        // 消耗一次充能
        BlockState mutableState = state;
        discharge(world, pos, mutableState);

        // 设置玩家的重生点
        // TODO: player.setSpawnPoint(pos, true, dimId);

        // 播放设置重生点音效
        world.playSound(
            ResourceLocation("minecraft:block.respawn_anchor.set_spawn"),
            sound::SoundCategory::Blocks,
            pos.center(),
            1.0f,
            1.0f
        );

        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

} // namespace blocks
} // namespace mc
