/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "TrailsBlocks.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntitySpawnPlacementRegistry.hpp"
#include "common/entity/entities/passive/special/SnifferEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/registry/TrailsBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/interactive/BrushableBlockEntity.hpp"
#include "common/world/blockentity/interactive/DecoratedPotBlockEntity.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include <memory>

namespace mc {
namespace blocks {

// ============================================================================
// ChiseledBookshelfBlock
// ============================================================================

ChiseledBookshelfBlock::ChiseledBookshelfBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    // HorizontalBlock 已添加 HORIZONTAL_FACING，需额外添加 SLOT 占用属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(FACING())
            .add(BlockStateProperties::SLOT_0_OCCUPIED())
            .add(BlockStateProperties::SLOT_1_OCCUPIED())
            .add(BlockStateProperties::SLOT_2_OCCUPIED())
            .add(BlockStateProperties::SLOT_3_OCCUPIED())
            .add(BlockStateProperties::SLOT_4_OCCUPIED())
            .add(BlockStateProperties::SLOT_5_OCCUPIED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(FACING(), Direction::North)
            .with(BlockStateProperties::SLOT_0_OCCUPIED(), false)
            .with(BlockStateProperties::SLOT_1_OCCUPIED(), false)
            .with(BlockStateProperties::SLOT_2_OCCUPIED(), false)
            .with(BlockStateProperties::SLOT_3_OCCUPIED(), false)
            .with(BlockStateProperties::SLOT_4_OCCUPIED(), false)
            .with(BlockStateProperties::SLOT_5_OCCUPIED(), false));
}

void ChiseledBookshelfBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState ChiseledBookshelfBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState().with(FACING(), Directions::opposite(context.horizontalDirection()));
}

i32 ChiseledBookshelfBlock::getComparatorInputOverride(
    const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 比较器输出 = 最后一个被占用的槽位索引 + 1
    i32 lastOccupied = -1;
    if (state.get(BlockStateProperties::SLOT_5_OCCUPIED()))
        lastOccupied = 5;
    else if (state.get(BlockStateProperties::SLOT_4_OCCUPIED()))
        lastOccupied = 4;
    else if (state.get(BlockStateProperties::SLOT_3_OCCUPIED()))
        lastOccupied = 3;
    else if (state.get(BlockStateProperties::SLOT_2_OCCUPIED()))
        lastOccupied = 2;
    else if (state.get(BlockStateProperties::SLOT_1_OCCUPIED()))
        lastOccupied = 1;
    else if (state.get(BlockStateProperties::SLOT_0_OCCUPIED()))
        lastOccupied = 0;

    return lastOccupied + 1;
}

// ============================================================================
// DecoratedPotBlock
// ============================================================================

DecoratedPotBlock::DecoratedPotBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(FACING())
            .add(BlockStateProperties::CRACKED())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(FACING(), Direction::North)
            .with(BlockStateProperties::CRACKED(), false)
            .with(BlockStateProperties::WATERLOGGED(), false));

    // 饰纹陶罐形状: 缩小为圆柱
    m_shape = CollisionShape::fromPixelBox(1, 0, 1, 15, 16, 15);
}

void DecoratedPotBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState DecoratedPotBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockState state = defaultState()
                           .with(FACING(), Directions::opposite(context.horizontalDirection()))
                           .with(BlockStateProperties::CRACKED(), false)
                           .with(BlockStateProperties::WATERLOGGED(), false);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState DecoratedPotBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

const CollisionShape& DecoratedPotBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const fluid::FluidState* DecoratedPotBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

std::unique_ptr<BlockEntity> DecoratedPotBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::DecoratedPotBlockEntity>(pos);
}

i32 DecoratedPotBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    BlockEntity* be = world.getBlockEntity(pos);
    if (be != nullptr && be->getType() == BlockEntityType::DecoratedPot) {
        auto* potEntity = static_cast<blockentity::DecoratedPotBlockEntity*>(be);
        return potEntity->getComparatorSignal();
    }
    return 0;
}

BlockActionResult DecoratedPotBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(hit);

    // 副手不处理
    if (hand == Hand::OffHand) {
        return ActionResultType::Pass;
    }

    // 获取方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::DecoratedPot) {
        return ActionResultType::Pass;
    }
    auto* potEntity = static_cast<blockentity::DecoratedPotBlockEntity*>(blockEntity);

    // 客户端：直接返回成功以播放动画
    if (world.isClientSide()) {
        ItemStack heldItem = player.getHeldItem(hand);
        return heldItem.isEmpty() ? ActionResultType::Success : ActionResultType::Success;
    }

    // === 服务端逻辑 ===
    ItemStack& heldItem = player.getHeldItem(hand);
    ItemStack potItem = potEntity->getItem();

    if (!heldItem.isEmpty()) {
        // 手持物品：尝试向陶罐中放入1个物品

        // 检查是否可以放入：陶罐为空，或罐内物品与手持物品相同且未达到最大堆叠
        bool canInsert = false;
        if (potItem.isEmpty()) {
            // 陶罐为空，可以放入
            canInsert = true;
        } else if (potItem.canMergeWith(heldItem) && potItem.getCount() < potItem.getMaxStackSize()) {
            // 罐内物品与手持物品相同且未满，可以叠加
            canInsert = true;
        }

        if (canInsert) {
            // 触发正摇晃动画
            potEntity->wobble(blockentity::DecoratedPotBlockEntity::WobbleStyle::Positive);

            // 分离1个物品（保留物品元数据如附魔、自定义名称等）
            ItemStack toInsert = heldItem.split(1);

            if (potItem.isEmpty()) {
                // 陶罐为空，直接设置
                potEntity->setItem(toInsert);
            } else {
                // 叠加到已有物品
                potItem.grow(1);
                potEntity->setItem(potItem);
            }

            // 创造模式不消耗手持物品
            if (player.isCreative()) {
                heldItem.grow(1);
            }

            // 播放插入音效
            // 音高随填充程度变化：0.7 + 0.5 * (count / maxStackSize)
            ItemStack newItem = potEntity->getItem();
            f32 pitch =
                0.7f + 0.5f * (static_cast<f32>(newItem.getCount()) / static_cast<f32>(newItem.getMaxStackSize()));
            world.playSound(
                SoundEvents::BLOCK_DECORATED_POT_INSERT, sound::SoundCategory::Blocks, pos.center(), 1.0f, pitch);

            // 触发方块变化游戏事件
            world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &state);

            // 标记方块实体已修改
            potEntity->setChanged();

            // 通知红石比较器更新信号
            world::redstone::RedstoneSystem::instance().updateComparators(world, pos);

            return ActionResultType::Success;
        }

        // 物品无法放入，回退到空手交互逻辑
    }

    // 空手交互或物品无法放入：触发负摇晃动画
    potEntity->wobble(blockentity::DecoratedPotBlockEntity::WobbleStyle::Negative);

    // 播放插入失败音效
    world.playSound(
        SoundEvents::BLOCK_DECORATED_POT_INSERT_FAIL, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);

    // 触发方块变化游戏事件
    world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &state);

    return ActionResultType::Success;
}

void DecoratedPotBlock::playerWillDestroy(IWorld& world, const BlockPos& pos, const BlockState& state, Player& player)
{
    // 如果玩家手持的物品具有 BREAKS_DECORATED_POTS 标签（剑、斧、镐、铲、锄、三叉戟、重锤），
    // 且没有精准采集附魔（精准采集可阻止陶罐碎裂），
    // 则将陶罐设为 CRACKED 状态，使其掉落4个单独的陶片而非陶罐物品。
    if (!state.get(BlockStateProperties::CRACKED())) {
        const ItemStack& mainHandItem = player.getMainHandItem();
        if (!mainHandItem.isEmpty() && mainHandItem.getItem()->isIn(item::tag::ItemTags::BREAKS_DECORATED_POTS())) {
            if (!item::enchant::EnchantmentHelper::hasSilkTouch(mainHandItem)) {
                BlockState crackedState = state.with(BlockStateProperties::CRACKED(), true);
                world.setBlockState(pos, &crackedState, 260);
            }
        }
    }

    Block::playerWillDestroy(world, pos, state, player);
}

void DecoratedPotBlock::onProjectileHit(
    IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile)
{
    MC_UNUSED(hitResult);

    if (world.isClientSide()) {
        return;
    }

    // MC Java: DecoratedPotBlock.onProjectileHit
    // 条件: projectile.mayInteract(serverlevel, blockpos) && projectile.mayBreak(serverlevel)
    // mayBreak 检查投射物是否属于 #minecraft:impact_projectiles 标签
    // 且 PROJECTILES_CAN_BREAK_BLOCKS 游戏规则为 true
    auto* projEntity = dynamic_cast<entity::ProjectileEntity*>(&projectile);
    if (projEntity != nullptr && projEntity->mayInteract(world, hitResult.blockPos()) && projEntity->mayBreak(world)) {
        BlockPos blockPos = hitResult.blockPos();
        if (!state.get(BlockStateProperties::CRACKED())) {
            BlockState crackedState = state.with(BlockStateProperties::CRACKED(), true);
            world.setBlockState(blockPos, &crackedState, 260);
        }
        // 将方块设为空气以触发 onBlockRemoved（掉落陶片）和方块移除
        world.setBlockState(blockPos, nullptr, 3);
    }
}

void DecoratedPotBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::DecoratedPot) {
        auto* potEntity = static_cast<blockentity::DecoratedPotBlockEntity*>(entity);

        if (!world.isClientSide()) {
            if (state.get(BlockStateProperties::CRACKED())) {
                // 碎裂状态：掉落4个独立的陶片/砖块物品
                const blockentity::PotDecorations& decorations = potEntity->getDecorations();
                const auto& patterns = decorations.ordered();
                math::Random rng;
                for (i32 i = 0; i < 4; ++i) {
                    const Item* sherdItem = blockentity::getItemFromPattern(patterns[static_cast<std::size_t>(i)]);
                    if (sherdItem != nullptr) {
                        ItemStack sherdStack(sherdItem, 1);
                        ItemDropHelper::spawnItemEntity(&world, sherdStack, pos.x + 0.5, pos.y + 0.5, pos.z + 0.5, rng);
                    }
                }
            } else {
                // 非碎裂状态：掉落陶罐内存储的物品（陶罐本身由战利品表系统处理）
                ItemStack storedItem = potEntity->getItem();
                if (!storedItem.isEmpty()) {
                    math::Random rng;
                    ItemDropHelper::spawnItemEntity(&world, storedItem, pos.x + 0.5, pos.y + 0.5, pos.z + 0.5, rng);
                }
            }

            // 清空容器以防止重复掉落
            potEntity->setItem(ItemStack());
        }
    }

    Block::onBlockRemoved(world, pos, state);
}

ItemStack DecoratedPotBlock::getCloneItemStack(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(state);

    // 中键选取：返回带有图案数据的陶罐物品
    if (world != nullptr && pos != nullptr) {
        BlockEntity* entity = world->getBlockEntity(*pos);
        if (entity != nullptr && entity->getType() == BlockEntityType::DecoratedPot) {
            auto* potEntity = static_cast<blockentity::DecoratedPotBlockEntity*>(entity);
            return blockentity::createDecoratedPotItem(potEntity->getDecorations());
        }
    }

    // 如果无法获取方块实体，返回默认的空陶罐物品
    return Block::getCloneItemStack(state, world, pos);
}

// ============================================================================
// BrushableBlock
// ============================================================================

BrushableBlock::BrushableBlock(const BlockProperties& properties,
    const Block* turnsInto,
    ResourceLocation brushSound,
    ResourceLocation brushCompletedSound)
    : FallingBlock(properties)
    , m_turnsInto(turnsInto)
    , m_brushSound(std::move(brushSound))
    , m_brushCompletedSound(std::move(brushCompletedSound))
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::DUSTED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::DUSTED(), 0));
}

void BrushableBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

std::unique_ptr<BlockEntity> BrushableBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::BrushableBlockEntity>(pos);
}

void BrushableBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 对齐 MC 1.21.11 BrushableBlock.tick：
    // 1. 获取 BrushableBlockEntity 并调用 checkReset()
    // 2. 执行 FallingBlock 的下落检测逻辑

    // 步骤 1：调用 checkReset
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::BrushableBlock) {
        auto* brushableEntity = static_cast<blockentity::BrushableBlockEntity*>(blockEntity);
        brushableEntity->checkReset(world);
    }

    // 步骤 2：委托基类执行下落检测
    FallingBlock::tick(world, pos, state, random);
}

// ============================================================================
// SnifferEggBlock
// ============================================================================

SnifferEggBlock::SnifferEggBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HATCH_0_2())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::HATCH_0_2(), 0));

    m_noCrackShape = CollisionShape::fromPixelBox(1, 0, 2, 15, 16, 14);
    m_crackedShape = CollisionShape::fromPixelBox(1, 0, 2, 15, 16, 14);
    m_hatchingShape = CollisionShape::fromPixelBox(1, 0, 2, 15, 16, 14);
}

void SnifferEggBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState SnifferEggBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    return defaultState().with(BlockStateProperties::HATCH_0_2(), 0);
}

const CollisionShape& SnifferEggBlock::getShape(const BlockState& state) const
{
    i32 hatch = state.get(BlockStateProperties::HATCH_0_2());
    if (hatch == 2) {
        return m_hatchingShape;
    }
    if (hatch == 1) {
        return m_crackedShape;
    }
    return m_noCrackShape;
}

void SnifferEggBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 对齐 MC SnifferEggBlock.onPlace：放置后调度首个孵化 tick。
    // MC 原版通过 ServerLevel.scheduleTick 调用，天然只在服务端执行。
    // 本项目 onBlockAdded 由 ServerWorld::setBlockState 在方块类型变化时触发，
    // 客户端路径同样会调用此回调，因此显式跳过客户端。
    if (world.isClientSide()) {
        return;
    }

    const bool flag = hatchBoost(world, pos);
    if (flag) {
        // 加速时播放蛋壳破裂粒子事件（MC levelEvent 3009）
        world.playEvent(world::WorldEvents::EGG_CRACK, pos, 0);
    }

    // 孵化总时长：加速 12000 tick，常规 24000 tick；分三阶段，每段 i/3 + [0, 300) tick
    constexpr i32 REGULAR_HATCH_TIME_TICKS = 24000;
    constexpr i32 BOOSTED_HATCH_TIME_TICKS = 12000;
    constexpr i32 RANDOM_HATCH_OFFSET_TICKS = 300;
    const i32 i = flag ? BOOSTED_HATCH_TIME_TICKS : REGULAR_HATCH_TIME_TICKS;
    const i32 j = i / 3;

    // 发出 BLOCK_PLACE 游戏事件（通知附近幽匿感测体）
    world.gameEvent(gameevent::GameEvents::BLOCK_PLACE, pos, &state);

    // 调度首个孵化 tick
    const i32 delay = j + world.getRandom().nextInt(RANDOM_HATCH_OFFSET_TICKS);
    world.tickManager().scheduleBlockTick(pos, *this, delay, world::tick::TickPriority::Normal);
}

void SnifferEggBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 对齐 MC SnifferEggBlock.tick：MC 原版通过 ServerLevel.scheduleTick 调用，
    // 天然只在服务端执行。本项目 tick 由 TickManager 调度，仅在服务端 TickManager::tick 中触发，
    // 但为防御性编程与测试友好，仍在此显式检查 isClientSide。
    if (world.isClientSide()) {
        return;
    }

    const i32 hatch = state.get(BlockStateProperties::HATCH_0_2());
    if (hatch < 2) {
        // 非孵化完成分支：播放 SNIFFER_EGG_CRACK 音效并增加孵化进度
        world.playSound(SoundEvents::SNIFFER_EGG_CRACK,
            sound::SoundCategory::Blocks,
            pos.center(),
            0.7f,
            0.9f + random.nextFloat() * 0.2f);
        BlockState newState = state.with(BlockStateProperties::HATCH_0_2(), hatch + 1);
        world.setBlockState(pos, &newState, 2);

        // 调度下一阶段 tick：MC 原版通过 onPlace 在 setBlock 时重新调度，
        // 本项目 onBlockAdded 仅在方块类型变化时触发，因此需在 tick 中显式调度下一阶段。
        // 每次重新查询 hatchBoost，因为下方方块可能在阶段间被替换。
        const bool flag = hatchBoost(world, pos);
        constexpr i32 REGULAR_HATCH_TIME_TICKS = 24000;
        constexpr i32 BOOSTED_HATCH_TIME_TICKS = 12000;
        constexpr i32 RANDOM_HATCH_OFFSET_TICKS = 300;
        const i32 i = flag ? BOOSTED_HATCH_TIME_TICKS : REGULAR_HATCH_TIME_TICKS;
        const i32 j = i / 3;
        const i32 delay = j + world.getRandom().nextInt(RANDOM_HATCH_OFFSET_TICKS);
        world.tickManager().scheduleBlockTick(pos, *this, delay, world::tick::TickPriority::Normal);
    } else {
        // 孵化完成 - 生成嗅探兽幼体
        //   1. 播放 SNIFFER_EGG_HATCH 音效
        //   2. 销毁蛋方块
        //   3. 创建 Sniffer 实体，setBaby(true)（年龄 -48000，40 分钟）
        //   4. snapTo(pos.center(), wrapDegrees(random * 360), 0)
        //   5. addFreshEntity
        world.playSound(SoundEvents::SNIFFER_EGG_HATCH,
            sound::SoundCategory::Blocks,
            pos.center(),
            0.7f,
            0.9f + random.nextFloat() * 0.2f);

        // 销毁蛋方块（对齐 MC level.destroyBlock(pos, false)）
        const BlockState* airState = BlockRegistry::instance().airState();
        if (airState != nullptr) {
            world.setBlockState(pos, airState, 2);
        }

        // 创建嗅探兽幼体
        auto sniffer = std::make_unique<SnifferEntity>(EntityId(0));
        if (sniffer) {
            // 设置为幼体（-48000 tick，40 分钟）
            // SnifferEntity::setChild 覆盖了 AgeableEntity::setChild，设置正确的嗅探兽幼年期
            sniffer->setChild(true);

            // 对齐 MC sniffer.snapTo(vec3.x, vec3.y, vec3.z, wrapDegrees(random * 360), 0)
            // pos.center() = (x+0.5, y+0.5, z+0.5)
            Vector3 center = pos.center();
            f32 yaw = math::wrapDegrees(random.nextFloat() * 360.0f);
            sniffer->setPosition(center.x, center.y, center.z);
            sniffer->setRotation(yaw, 0.0f);

            // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化
            // MC 原版使用 EntitySpawnReason.BREEDING（蛋由繁殖产生），这里使用 Natural
            // 因为 scheduleTick 孵化更接近自然生成场景；不影响功能
            entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(world, pos);
            sniffer->finalizeSpawn(world, difficultyInstance, world::spawn::SpawnReason::Natural);

            // 生成到世界
            world.spawnEntity(std::move(sniffer));
        }
    }
}

bool SnifferEggBlock::hatchBoost(IWorld& world, const BlockPos& pos)
{
    // 对齐 MC SnifferEggBlock.hatchBoost：检查下方方块是否在 SNIFFER_EGG_HATCH_BOOST 标签中
    const BlockState* belowState = world.getBlockState(pos.down());
    return belowState != nullptr && BlockTags::SNIFFER_EGG_HATCH_BOOST().contains(*belowState);
}

} // namespace blocks
} // namespace mc
