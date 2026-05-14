#include "ComposterBlock.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "CompostableItems.hpp"

namespace mc {
namespace blocks {

// ========== ComposterBlock 实现 ==========

ComposterBlock::ComposterBlock(const BlockProperties& properties)
    : Block(properties)
{

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
        m_shapesByLevel[i] = CollisionShape::box(2.0f * P, fillHeight, 2.0f * P, 14.0f * P, 16.0f * P, 14.0f * P);
    }
    // 等级7和8形状相同
    m_shapesByLevel[8] = m_shapesByLevel[7];
}

BlockState ComposterBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

void ComposterBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    int level = getLevel(state);
    if (level == 7) {
        // 等级7时，经过20 tick后变成等级8（可以收获骨粉）
        BlockState newState = state.with(BlockStateProperties::LEVEL_0_8(), 8);
        world.setBlockState(pos, &newState, 3);

        // MC 1.16.5: 播放堆肥完成音效
        if (!world.isClientSide()) {
            world.playSound(SoundEvents::BLOCK_COMPOSTER_READY, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
        }
    }
}

const CollisionShape& ComposterBlock::getShape(const BlockState& state) const
{
    int level = getLevel(state);
    MC_ASSERT(level >= 0 && level <= 8);
    // 等级0时返回完整方块形状
    if (level == 0) {
        return m_collisionShape;
    }
    return m_shapesByLevel[level];
}

const CollisionShape& ComposterBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 碰撞箱始终是完整方块
    return m_collisionShape;
}

int ComposterBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{

    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 比较器输出 = 等级
    return getLevel(state);
}

BlockState ComposterBlock::attemptCompost(
    const BlockState& state, IWorld& world, const BlockPos& pos, Block& block, u32 itemId)
{

    int level = getLevel(state);
    if (level >= 7) {
        return state; // 已满或正在完成
    }

    // 从 CompostableItems 注册表获取堆肥概率
    const Item* item = Item::getItem(static_cast<ItemId>(itemId));
    if (item == nullptr) {
        return state;
    }

    float chance = CompostableItems::getCompostChance(item);
    if (chance <= 0.0f) {
        return state; // 不可堆肥
    }

    // 概率性增加等级
    math::Random random;
    if (random.nextFloat() < chance) {
        int newLevel = level + 1;
        BlockState newState = state.with(BlockStateProperties::LEVEL_0_8(), newLevel);
        world.setBlockState(pos, &newState, 3);

        // MC 1.16.5: 播放成功音效
        if (!world.isClientSide()) {
            world.playSound(
                SoundEvents::BLOCK_COMPOSTER_FILL_SUCCESS, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
        }

        // 如果达到等级7，调度 20 tick 后的转变
        if (newLevel == 7) {
            world.tickManager().scheduleBlockTick(pos, block, 20);
        }

        return newState;
    }

    // MC 1.16.5: 播放失败音效（尝试堆肥但没增加等级）
    if (!world.isClientSide()) {
        world.playSound(SoundEvents::BLOCK_COMPOSTER_FILL, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
    }

    return state;
}

BlockState ComposterBlock::empty(IWorld& world, const BlockPos& pos, BlockState& state)
{
    // MC 1.16.5: 生成骨粉物品
    // 只有等级为 8 时才能收获
    int level = getLevel(state);
    if (level != 8) {
        return state;
    }

    // 掉落骨粉物品
    if (!world.isClientSide() && Items::BONE_MEAL != nullptr) {
        // 创建骨粉物品堆
        ItemStack boneMealStack(Items::BONE_MEAL, 1);

        // 使用 ItemDropHelper 生成物品实体
        math::Random random;
        ItemDropHelper::spawnItemEntity(&world,
            boneMealStack,
            static_cast<f64>(pos.x) + 0.5,
            static_cast<f64>(pos.y) + 1.0, // 在堆肥桶上方生成
            static_cast<f64>(pos.z) + 0.5,
            random,
            ItemDropHelper::DEFAULT_PICKUP_DELAY,
            "" // 无所有者
        );
    }

    // 重置为等级0
    BlockState newState = state.with(BlockStateProperties::LEVEL_0_8(), 0);
    world.setBlockState(pos, &newState, 3);

    // MC 1.16.5: 播放清空音效
    if (!world.isClientSide()) {
        world.playSound(SoundEvents::BLOCK_COMPOSTER_EMPTY, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
    }

    return newState;
}

bool ComposterBlock::isCompostable(u32 itemId)
{
    const Item* item = Item::getItem(static_cast<ItemId>(itemId));
    return CompostableItems::isCompostable(item);
}

float ComposterBlock::getCompostChance(u32 itemId)
{
    const Item* item = Item::getItem(static_cast<ItemId>(itemId));
    return CompostableItems::getCompostChance(item);
}

ActionResultType ComposterBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(hand);
    MC_UNUSED(hit);
    int level = getLevel(state);

    // 如果等级为8，取出骨粉
    if (level == 8) {
        empty(world, pos, const_cast<BlockState&>(state));
        return ActionResultType::Success;
    }

    // 检查玩家手持物品
    ItemStack heldItem = player.inventory().getSelectedStack();
    if (heldItem.isEmpty()) {
        return ActionResultType::Pass;
    }

    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    // 检查物品是否可堆肥
    float chance = CompostableItems::getCompostChance(item);
    if (chance <= 0.0f) {
        return ActionResultType::Pass;
    }

    // 尝试堆肥
    BlockState newState = attemptCompost(state, world, pos, *this, static_cast<u32>(item->itemId()));

    // 如果堆肥成功（状态改变了），消耗物品
    if (newState.get(BlockStateProperties::LEVEL_0_8()) > level) {
        // 非创造模式消耗物品
        if (!player.abilities().creativeMode) {
            heldItem.shrink(1);
            player.inventory().setChanged();
        }
        return ActionResultType::Success;
    }

    // 堆肥失败但仍播放了音效
    return ActionResultType::Success;
}

} // namespace blocks
} // namespace mc
